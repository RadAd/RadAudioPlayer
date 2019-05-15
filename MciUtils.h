#pragma once

#define WM_MCI_ERROR (WM_USER + 11)

void MciClose(HWND hDlg, MCIDEVICEID& wDeviceID);
MCIERROR MciOpen(HWND hDlg, MCIDEVICEID& wDeviceID, LPCTSTR pFileName);
DWORD_PTR MciGetStatus(HWND hDlg, MCIDEVICEID wDeviceID, DWORD dwItem);
void MciPlay(HWND hDlg, MCIDEVICEID wDeviceID);
void MciPause(HWND hDlg, MCIDEVICEID wDeviceID);
void MciSeekTo(HWND hDlg, MCIDEVICEID wDeviceID, DWORD dwTo);
void MciSetVolume(HWND hDlg, MCIDEVICEID wDeviceID, DWORD dwVolume);
