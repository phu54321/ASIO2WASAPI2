// Copyright (C) 2023 Hyun Woo Park
//
// This file is part of ASIO2WASAPI2.
//
// ASIO2WASAPI2 is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// ASIO2WASAPI2 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with ASIO2WASAPI2.  If not, see <http://www.gnu.org/licenses/>.


#ifndef ASIO2WASAPI2_CREATEIAUDIOCLIENT_H
#define ASIO2WASAPI2_CREATEIAUDIOCLIENT_H

#include <memory>
#include <mmdeviceapi.h>
#include <Audioclient.h>

const int BUFFER_SIZE_REQUEST_USEDEFAULT = -1;

enum class WASAPIMode {
    Pull,
    Event
};

bool FindStreamFormat(
        const std::shared_ptr<IMMDevice> &pDevice,
        int channelCount,
        int sampleCount,
        int bufferSizeRequest = BUFFER_SIZE_REQUEST_USEDEFAULT,
        WASAPIMode mode = WASAPIMode::Event,
        WAVEFORMATEXTENSIBLE *pwfxt = nullptr,
        std::shared_ptr<IAudioClient> *ppAudioClient = nullptr);

#endif //ASIO2WASAPI2_CREATEIAUDIOCLIENT_H
