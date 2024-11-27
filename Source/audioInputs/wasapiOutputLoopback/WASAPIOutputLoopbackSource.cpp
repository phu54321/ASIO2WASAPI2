// Copyright (C) 2024 Hyunwoo Park
//
// This file is part of trgkASIO.
//
// trgkASIO is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// trgkASIO is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with ASIO2WASAPI2.  If not, see <http://www.gnu.org/licenses/>.
//

//
// Created by whyask37 on 2024-11-27.
//

#include <Audioclient.h>
#include "WASAPIOutputLoopbackSource.h"
#include "../../utils/logger.h"
#include "../../utils/raiiUtils.h"
#include <algorithm>

#define REFTIMES_PER_SEC  10000000

WASAPIOutputLoopbackSource::WASAPIOutputLoopbackSource(const IMMDevicePtr &pDevice, int channelCount, int sampleRate,
                                                       bool interceptDefaultOutput)
        : _channelCount(channelCount), _sampleRate(sampleRate), _pDevice(pDevice), _pDeviceId(getDeviceId(pDevice)) {


    IAudioClient *pAudioClient_ = nullptr;

    HRESULT hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, (void **) &pAudioClient_);
    if (FAILED(hr) || !pAudioClient_) {
        mainlog->error(L"{} WASAPIOutputLoopbackSource: pAudioClient->Activate failed: 0x{:08X}", _pDeviceId,
                       (uint32_t) hr);
        throw AppException("Failed to create loopback device");
    }
    _pAudioClient = make_autorelease(pAudioClient_);

    hr = _pAudioClient->GetMixFormat(&_pwfx);
    if (FAILED(hr)) {
        throw AppException("GetMixFormat failed");
    }

    WAVEFORMATEXTENSIBLE waveFormat = {0};
    waveFormat.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    waveFormat.Format.nChannels = channelCount;
    waveFormat.Format.nSamplesPerSec = _pwfx->nSamplesPerSec;
    waveFormat.Format.wBitsPerSample = 32;
    waveFormat.Format.nBlockAlign = waveFormat.Format.wBitsPerSample * waveFormat.Format.nChannels / 8;
    waveFormat.Format.nAvgBytesPerSec = waveFormat.Format.nSamplesPerSec * waveFormat.Format.nBlockAlign;
    waveFormat.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    waveFormat.Samples.wValidBitsPerSample = waveFormat.Format.wBitsPerSample;
    waveFormat.dwChannelMask = (1 << channelCount) - 1;
    waveFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

    CoTaskMemFree(_pwfx);

    hr = _pAudioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_LOOPBACK,
            REFTIMES_PER_SEC,
            0,
            (WAVEFORMATEX *) &waveFormat,
            nullptr);

    if (FAILED(hr)) {
        mainlog->error(L"{} WASAPIOutputLoopbackSource: pAudioClient->Initialize failed: 0x{:08X}", _pDeviceId,
                       (uint32_t) hr);
        throw AppException("IsFormatSupported failed");
    }
    _pAudioClient->GetBufferSize(&_bufferSize);
    _tempBuffer.resize(_bufferSize);

    // Create capture clinet
    IAudioCaptureClient *pCaptureClient_ = nullptr;
    hr = _pAudioClient->GetService(
            IID_IAudioCaptureClient,
            (void **) &pCaptureClient_);
    if (FAILED(hr)) {
        mainlog->error(L"{} WASAPIOutputLoopbackSource: pAudioClient->GetService failed: 0x{:08X}", _pDeviceId,
                       (uint32_t) hr);
        throw AppException("GetService failed");
    }
    _pAudioCaptureClient = make_autorelease(pCaptureClient_);

    mainlog->info(L"{} WASAPIOutputLoopbackSource resample: {} -> {}", _pDeviceId, waveFormat.Format.nSamplesPerSec,
                  _sampleRate);
    for (int ch = 0; ch < _channelCount; ch++) {
        _audioBuffer.emplace_back((_bufferSize + 1024) * 2);
        _resamplers.push_back(
                std::make_unique<r8b::CDSPResampler24>(waveFormat.Format.nSamplesPerSec, _sampleRate,
                                                       (int) _bufferSize));
    }

    if (interceptDefaultOutput) {
        auto prevOutputDevice = getDefaultOutputDevice();
        _prevOutputDeviceId = getDeviceId(prevOutputDevice);
        setDefaultOutputDeviceId(_pDeviceId);
    }

    _pAudioClient->Start();
}

WASAPIOutputLoopbackSource::~WASAPIOutputLoopbackSource() {
    _pAudioClient->Stop();
    if (!_prevOutputDeviceId.empty()) {
        setDefaultOutputDeviceId(_prevOutputDeviceId);
    }
}

void WASAPIOutputLoopbackSource::_feedResampledData(int ch, double *input, UINT32 size) {
    while (size > 0) {
        auto packetSize = std::min(size, _bufferSize);
        double *outputSamples;
        auto outputLength = _resamplers[ch]->process(input, packetSize, outputSamples);
        if (!_audioBuffer[ch].push(outputSamples, outputLength)) {
            mainlog->debug(
                    L"{} WASAPIOutputLoopbackSource::render: ringbuffer[{}] overflow - capacity {}, size {}, new {}",
                    _pDeviceId, ch, _audioBuffer[0].capacity(), _audioBuffer[0].size(), outputLength);
        }
        input += packetSize;
        size -= packetSize;
    }
}

void WASAPIOutputLoopbackSource::render(int64_t currentFrame, std::vector<std::vector<int32_t>> *outputBuffer) {
    UINT32 numFramesAvailable;
    UINT32 packetLength = 0;
    BYTE *pData;
    DWORD flags;

    // Fetch all
    _pAudioCaptureClient->GetNextPacketSize(&packetLength);
    mainlog->debug(L"{} WASAPIOutputLoopbackSource::render: GetNextPacketSize {}", _pDeviceId, packetLength);
    while (packetLength != 0) {
        _pAudioCaptureClient->GetBuffer(
                &pData,
                &numFramesAvailable,
                &flags, nullptr, nullptr);

        // Buffer upgrade..
        if (_tempBuffer.size() < numFramesAvailable) {
            mainlog->warn(
                    L"{} WASAPIOutputLoopbackSource::render: buffer size inadequate: tempBuffer.size({}) < numFramesAvailable({})",
                    _pDeviceId, _tempBuffer.size(), numFramesAvailable);
            _tempBuffer.resize(numFramesAvailable);
        }


        if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
            mainlog->debug(L"{} WASAPIOutputLoopbackSource::render fetched buffer size {} [silent]", _pDeviceId,
                           numFramesAvailable);
            for (int ch = 0; ch < _channelCount; ch++) {
                std::fill(_tempBuffer.data(), _tempBuffer.data() + numFramesAvailable, 0.0);
                _feedResampledData(ch, _tempBuffer.data(), numFramesAvailable);
            }
        } else {
            for (int ch = 0; ch < _channelCount; ch++) {
                auto in = reinterpret_cast<int32_t *>(pData) + ch;
                auto pStart = _tempBuffer.data();
                auto pEnd = _tempBuffer.data() + numFramesAvailable;
                for (auto p = pStart; p != pEnd; p++) {
                    *p = *in / 2147483648.0;
                    in += _channelCount;
                }
                _feedResampledData(ch, _tempBuffer.data(), numFramesAvailable);
            }
            mainlog->debug(L"{} WASAPIOutputLoopbackSource::render fetched buffer size {}", _pDeviceId,
                           numFramesAvailable);
        }
        _pAudioCaptureClient->ReleaseBuffer(numFramesAvailable);
        _pAudioCaptureClient->GetNextPacketSize(&packetLength);
    }

    // Render
    auto outputSize = outputBuffer->at(0).size();
    if (_audioBuffer[0].size() < outputSize) {
        mainlog->warn(
                L"{} WASAPIOutputLoopbackSource::render: Capture not yet filled: audioBuffer.size({}), outputBuffer.size({})",
                _pDeviceId, _audioBuffer[0].size(), outputSize);
    }
    if (outputSize > _tempBuffer.size()) {
        _tempBuffer.resize(outputSize);
    }
    mainlog->debug(L"{} WASAPIOutputLoopbackSource::render: ringbuffer, size {}", _pDeviceId,
                   _audioBuffer[0].size());
    for (unsigned ch = 0; ch < _channelCount; ch++) {
        _audioBuffer[ch].get(_tempBuffer.data(), outputSize);
        auto &outputBufferChannel = outputBuffer->at(ch);
        for (int i = 0; i < outputSize; i++) {
            outputBufferChannel[i] += (int) round(_tempBuffer[i] * (1 << 23));
        }
    }
}
