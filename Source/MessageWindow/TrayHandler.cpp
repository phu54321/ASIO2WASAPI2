// Copyright (C) 2023 Hyunwoo Park
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
// along with trgkASIO.  If not, see <http://www.gnu.org/licenses/>.


#include <Windows.h>
#include "TrayHandler.h"
#include <shellapi.h>

void createTrayIcon(HWND hWnd, HICON hIcon, const TCHAR* szTip) {
    NOTIFYICONDATA nid = {0};

    nid.cbSize = sizeof(nid);
    nid.hWnd = hWnd;
    nid.uID = trayIconID;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;
    nid.uCallbackMessage = WM_TRAYICON_MSG;
    nid.hIcon = hIcon;
    nid.uTimeout = 1000;
    lstrcpy(nid.szTip, szTip);
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void setTooltip(HWND hWnd, const TCHAR* szTip) {
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(nid);
    nid.uID = trayIconID;
    nid.hWnd = hWnd;
    nid.uFlags = NIF_TIP;
    lstrcpy(nid.szTip, szTip);
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void removeTrayIcon(HWND hWnd) {
    NOTIFYICONDATA nid = {0};

    nid.cbSize = sizeof(nid);
    nid.uID = trayIconID;
    nid.hWnd = hWnd;
    Shell_NotifyIcon(NIM_DELETE, &nid);
}
