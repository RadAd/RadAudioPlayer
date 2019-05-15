#include <Windows.h>
#include <Windowsx.h>
#include <CommCtrl.h>
#include <tchar.h>

#include "MciUtils.h"
#include "TrackBar.h"
#include "resource.h"

// TODO
// Volume
// Format trackbar tooltip
// Playlist
// Tags
// Mouse wheel on slider doesn't seem to generate notifications

#define HANDLE_DLGMSG(hwnd, message, fn) \
    case (message): \
        return (SetDlgMsgResult(hwnd, message, \
            HANDLE_##message((hwnd), (wParam), (lParam), (fn))))

/* BOOL Cls_OnSysCommand(HWND hwnd, UINT cmd, int x, int y) */
#undef HANDLE_WM_SYSCOMMAND
#define HANDLE_WM_SYSCOMMAND(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (UINT)(wParam), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam)))

/* void Cls_OnAppCommand(HWND hwnd, HWND hchild, UINT cmd, UINT uDevice, DWORD dwKeys) */
#define HANDLE_WM_APPCOMMAND(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HWND)(wParam), GET_APPCOMMAND_LPARAM(lParam), GET_DEVICE_LPARAM(lParam), GET_KEYSTATE_LPARAM(lParam)), TRUE)

#define MS (1000)
#define WM_AP_OPEN (WM_USER + 10)

LPCTSTR PathFindName(LPCTSTR pFullName)
{
    LPCTSTR pName = _tcsrchr(pFullName, _T('\\'));
    return pName ? pName + 1 : pFullName;
}

void FormatTime(DWORD t, LPTSTR buf, DWORD len)
{
    DWORD h = t / (60 * 60);
    t = t % (60 * 60);
    DWORD m = t / 60;
    DWORD s = t % 60;
    if (h > 0)
        _stprintf_s(buf, len, _T("%d:%02d:%02d"), h, m, s);
    else
        _stprintf_s(buf, len, _T("%d:%02d"), m, s);
}

struct PlayerDlgData
{
    MCIDEVICEID wDeviceID = 0;
    TCHAR strTitle[1024];
    bool bTracking = false;
    MCIDEVICEID wLastDeviceID = -1;
    DWORD_PTR dwLastMode = 0;
    HICON hIcoPlay = NULL;
    HICON hIcoPause = NULL;
};

BOOL PlayerDlgOnInitDialog(HWND hDlg, HWND hWndFocus, LPARAM lParam)
{
    PlayerDlgData* pData = new PlayerDlgData;
    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) pData);

    const HWND hPosition = GetDlgItem(hDlg, IDC_POSITION);
    const HWND hVolume = GetDlgItem(hDlg, IDC_VOLUME);

    GetWindowText(hDlg, pData->strTitle, ARRAYSIZE(pData->strTitle));

    HICON hIcon = (HICON) LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PLAYER), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE);
    SendMessageW(hDlg, WM_SETICON, ICON_BIG, (LPARAM) hIcon);

    pData->hIcoPlay = (HICON) LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PLAY), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE);
    pData->hIcoPause = (HICON) LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PAUSE), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE);

    SetTimer(hDlg, 123, MS / 4, nullptr);
    SendMessage(hDlg, WM_TIMER, 0 , 0);
    TrackBar_SetTicFreq(hPosition, 60);
    TrackBar_SetLineSize(hPosition, 10);
    TrackBar_SetPageSize(hPosition, 60);
    TrackBar_SetTicFreq(hVolume, 10);
    TrackBar_SetLineSize(hVolume, 1);
    TrackBar_SetPageSize(hVolume, 10);
    TrackBar_SetRange(hVolume, 0, 100);
    TrackBar_SetPos(hVolume, 100 - 100);

    for (int i = 1; i < __argc; ++i)
    {
        SendMessage(hDlg, WM_AP_OPEN, 0, (LPARAM) __targv[i]);
    }

    return TRUE;
}

void PlayerDlgOnDestroy(HWND hDlg)
{
    PlayerDlgData* pData = (PlayerDlgData*) GetWindowLongPtr(hDlg, DWLP_USER);

    MciClose(hDlg, pData->wDeviceID);

    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) nullptr);
    delete pData;
    pData = nullptr;
}

void PlayerDlgOnCommand(HWND hDlg, int id, HWND hWndCtl, UINT codeNotify)
{
    const PlayerDlgData* pData = (PlayerDlgData*) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (id)
    {
    case IDC_PLAY:
        {
            DWORD_PTR dwMode = MciGetStatus(hDlg, pData->wDeviceID, MCI_STATUS_MODE);
            if (dwMode == MCI_MODE_PLAY)
                MciPause(hDlg, pData->wDeviceID);
            else
                MciPlay(hDlg, pData->wDeviceID);
        }
        break;

    case IDCANCEL:
        //EndDialog(hDlg, IDCANCEL);
        break;
    }
}

BOOL PlayerDlgOnSysCommand(HWND hDlg, UINT cmd, int x, int y)
{
    if (cmd == SC_CLOSE)
    {
        EndDialog(hDlg, ERROR_SUCCESS);
        return TRUE;
    }
    else
        return FALSE;
}

void PlayerDlgOnTimer(HWND hDlg, UINT id)
{
    static bool bRecursive = false;
    if (bRecursive)
        return;

    bRecursive = true;

    PlayerDlgData* pData = (PlayerDlgData*) GetWindowLongPtr(hDlg, DWLP_USER);
    const HWND hPosition = GetDlgItem(hDlg, IDC_POSITION);
    const HWND hPlay = GetDlgItem(hDlg, IDC_PLAY);
    const HWND hBegin = GetDlgItem(hDlg, IDC_BEGIN);

    if (pData->wLastDeviceID != pData->wDeviceID)
    {
        EnableWindow(hPlay, pData->wDeviceID != 0);
        EnableWindow(hPosition, pData->wDeviceID != 0);
        pData->wLastDeviceID = pData->wDeviceID;
    }

    DWORD_PTR dwMode = pData->wDeviceID != 0 ? MciGetStatus(hDlg, pData->wDeviceID, MCI_STATUS_MODE) : MCI_MODE_NOT_READY;
    if (pData->dwLastMode != dwMode)
    {
        //SetWindowText(hPlay, dwMode == MCI_MODE_PLAY ? _T("Pause") : _T("Play"));
        SendMessage(hPlay, BM_SETIMAGE, IMAGE_ICON, (LPARAM) (dwMode == MCI_MODE_PLAY ? pData->hIcoPause : pData->hIcoPlay));

        if (pData->dwLastMode == MCI_MODE_PLAY && dwMode == MCI_MODE_STOP)
        {
            MciSeekTo(hDlg, pData->wDeviceID, 0);
            TrackBar_SetPos(hPosition, 0);
            TCHAR buf[100];
            FormatTime(0, buf, ARRAYSIZE(buf));
            SetWindowText(hBegin, buf);
        }

        pData->dwLastMode = dwMode;
    }

    if (!pData->bTracking && dwMode == MCI_MODE_PLAY)
    {
        DWORD dwPosition = (DWORD) MciGetStatus(hDlg, pData->wDeviceID, MCI_STATUS_POSITION) / MS;
        TrackBar_SetPos(hPosition, dwPosition);
        TCHAR buf[100];
        FormatTime(dwPosition, buf, ARRAYSIZE(buf));
        SetWindowText(hBegin, buf);
    }

    bRecursive = false;
}

void PlayerDlgOnDropFiles(HWND hDlg, HDROP hDrop)
{
    UINT count = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);
    for (UINT i = 0; i < count; ++i)
    {
        TCHAR buf[MAX_PATH];
        DragQueryFile(hDrop, i, buf, ARRAYSIZE(buf));

        SendMessage(hDlg, WM_AP_OPEN, 0, (LPARAM) buf);
    }
}

void PlayerDlgOnAPOpen(HWND hDlg, LPCTSTR pFileName)
{
    PlayerDlgData* pData = (PlayerDlgData*) GetWindowLongPtr(hDlg, DWLP_USER);
    const HWND hPosition = GetDlgItem(hDlg, IDC_POSITION);
    const HWND hVolume = GetDlgItem(hDlg, IDC_VOLUME);
    const HWND hBegin = GetDlgItem(hDlg, IDC_BEGIN);
    const HWND hEnd = GetDlgItem(hDlg, IDC_END);

    if (MciOpen(hDlg, pData->wDeviceID, pFileName) == MMSYSERR_NOERROR)
    {
        TCHAR buf[1024];
        _stprintf_s(buf, _T("%s - %s"), PathFindName(pFileName), pData->strTitle);
        SetWindowText(hDlg, buf);

        //DWORD_PTR dwTimeFormat = MciGetStatus(hDlg, MCI_STATUS_TIME_FORMAT);
        DWORD dwLength = (DWORD) MciGetStatus(hDlg, pData->wDeviceID, MCI_STATUS_LENGTH) / MS;
        TrackBar_SetRange(hPosition, 0, (WORD) dwLength);

        FormatTime(dwLength, buf, ARRAYSIZE(buf));
        SetWindowText(hEnd, buf);

        DWORD dwVolume = 100 - TrackBar_GetPos(hVolume);
        MciSetVolume(hDlg, pData->wDeviceID, dwVolume * 10);

        MciPlay(hDlg, pData->wDeviceID);
    }
    else
    {
        SetWindowText(hDlg, pData->strTitle);
        SetWindowText(hBegin, _T(""));
        SetWindowText(hEnd, _T(""));
    }
}

void PlayerDlgOnMciError(HWND hDlg, LPCTSTR pMessage)
{
    PlayerDlgData* pData = (PlayerDlgData*) GetWindowLongPtr(hDlg, DWLP_USER);
    MessageBox(hDlg, pMessage, pData->strTitle, MB_ICONERROR | MB_OK);
}

BOOL PlayerDlgOnNotify(HWND hDlg, int id, LPNMHDR pNmHdr)
{
    PlayerDlgData* pData = (PlayerDlgData*) GetWindowLongPtr(hDlg, DWLP_USER);
    const HWND hPosition = GetDlgItem(hDlg, IDC_POSITION);
    const HWND hVolume = GetDlgItem(hDlg, IDC_VOLUME);
    const HWND hBegin = GetDlgItem(hDlg, IDC_BEGIN);

    switch (id)
    {
    case IDC_POSITION:
        switch (pNmHdr->code)
        {
        case NM_RELEASEDCAPTURE:
            {
                pData->bTracking = false;
                DWORD dwTo = TrackBar_GetPos(hPosition);
                DWORD_PTR dwMode = MciGetStatus(hDlg, pData->wDeviceID, MCI_STATUS_MODE);
                MciSeekTo(hDlg, pData->wDeviceID, dwTo * MS);
                if (dwMode == MCI_MODE_PLAY)
                    MciPlay(hDlg, pData->wDeviceID);
            }
            break;

        case TRBN_THUMBPOSCHANGING:
            {
                const NMTRBTHUMBPOSCHANGING* pPosChanging = (NMTRBTHUMBPOSCHANGING*) pNmHdr;
                if (pPosChanging->nReason == TB_THUMBTRACK)
                {
                    pData->bTracking = true;
                    DWORD dwTo = TrackBar_GetPos(hPosition);
                    TCHAR buf[100];
                    FormatTime(dwTo, buf, ARRAYSIZE(buf));
                    SetWindowText(hBegin, buf);
                }
                else if (pPosChanging->nReason != TB_THUMBPOSITION && pPosChanging->nReason != TB_ENDTRACK)
                {
                    DWORD_PTR dwMode = MciGetStatus(hDlg, pData->wDeviceID, MCI_STATUS_MODE);
                    DWORD dwLength = (DWORD) MciGetStatus(hDlg, pData->wDeviceID, MCI_STATUS_LENGTH);
                    DWORD dwPos = pPosChanging->dwPos * MS;
                    if (dwPos > dwLength)
                        dwPos = dwLength;
                    MciSeekTo(hDlg, pData->wDeviceID, dwPos);
                    if (dwMode == MCI_MODE_PLAY)
                        MciPlay(hDlg, pData->wDeviceID);
                    TCHAR buf[100];
                    FormatTime(dwPos / MS, buf, ARRAYSIZE(buf));
                    SetWindowText(hBegin, buf);
                }
            }
            break;
        }
        return TRUE;

    case IDC_VOLUME:
        switch (pNmHdr->code)
        {
        case NM_RELEASEDCAPTURE:
            if (pData->wDeviceID != 0)
            {
                DWORD dwVolume = 100 - TrackBar_GetPos(hVolume);
                MciSetVolume(hDlg, pData->wDeviceID, dwVolume * 10);
            }
            break;

        case TRBN_THUMBPOSCHANGING:
            if (pData->wDeviceID != 0)
            {
                const NMTRBTHUMBPOSCHANGING* pPosChanging = (NMTRBTHUMBPOSCHANGING*) pNmHdr;
                DWORD dwPos = 100 - pPosChanging->dwPos;
                if (dwPos > (MAXDWORD / 2)) dwPos = 0;
                if (dwPos > 100) dwPos = 100;
                if (pPosChanging->nReason == TB_THUMBTRACK)
                {
                    MciSetVolume(hDlg, pData->wDeviceID, dwPos * 10);
                }
                else if (pPosChanging->nReason != TB_THUMBPOSITION && pPosChanging->nReason != TB_ENDTRACK)
                {
                    MciSetVolume(hDlg, pData->wDeviceID, dwPos * 10);
                }
            }
            break;
        }
        return TRUE;

    default:
        return FALSE;
    }
}

void PlayerDlgOnAppCommand(HWND hDlg, HWND hChild, UINT cmd, UINT uDevice, DWORD dwKeys)
{
    PlayerDlgData* pData = (PlayerDlgData*) GetWindowLongPtr(hDlg, DWLP_USER);
    const HWND hVolume = GetDlgItem(hDlg, IDC_VOLUME);
    switch (cmd)
    {
    case APPCOMMAND_MEDIA_PLAY:
        MciPlay(hDlg, pData->wDeviceID);
        break;
    case APPCOMMAND_MEDIA_PAUSE:
        MciPause(hDlg, pData->wDeviceID);
        break;
    case APPCOMMAND_MEDIA_PLAY_PAUSE:
        {
            DWORD_PTR dwMode = MciGetStatus(hDlg, pData->wDeviceID, MCI_STATUS_MODE);
            if (dwMode == MCI_MODE_PLAY)
                MciPause(hDlg, pData->wDeviceID);
            else
                MciPlay(hDlg, pData->wDeviceID);
        }
        break;
    case APPCOMMAND_VOLUME_DOWN:
        {
            DWORD dwVolume = 100 - TrackBar_GetPos(hVolume);
            dwVolume -= 10;
            if (dwVolume > (DWORD) -100) dwVolume = 0;
            MciSetVolume(hDlg, pData->wDeviceID, dwVolume * 10);
            TrackBar_SetPos(hVolume, 100 - dwVolume);
        }
        break;
    case APPCOMMAND_VOLUME_UP:
        {
            DWORD dwVolume = 100 - TrackBar_GetPos(hVolume);
            dwVolume += 10;
            if (dwVolume > 100) dwVolume = 100;
            MciSetVolume(hDlg, pData->wDeviceID, dwVolume * 10);
            TrackBar_SetPos(hVolume, 100 - dwVolume);
        }
        break;
    }
}

INT_PTR PlayerDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (uMsg)
    {
        HANDLE_MSG(hDlg, WM_INITDIALOG, PlayerDlgOnInitDialog);
        HANDLE_DLGMSG(hDlg, WM_DESTROY, PlayerDlgOnDestroy);
        HANDLE_DLGMSG(hDlg, WM_COMMAND, PlayerDlgOnCommand);
        HANDLE_MSG(hDlg, WM_SYSCOMMAND, PlayerDlgOnSysCommand);
        HANDLE_MSG(hDlg, WM_NOTIFY, PlayerDlgOnNotify);
        HANDLE_DLGMSG(hDlg, WM_TIMER, PlayerDlgOnTimer);
        HANDLE_DLGMSG(hDlg, WM_DROPFILES, PlayerDlgOnDropFiles);
        HANDLE_MSG(hDlg, WM_APPCOMMAND, PlayerDlgOnAppCommand);

    case WM_AP_OPEN: PlayerDlgOnAPOpen(hDlg, (LPCTSTR) lParam); return TRUE;
    case WM_MCI_ERROR: PlayerDlgOnMciError(hDlg, (LPCTSTR) lParam); return TRUE;
    default: return FALSE;
    }
}

int WINAPI _tWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR     lpCmdLine,
    int       nShowCmd
)
{
    InitCommonControls();
    INT_PTR ret = DialogBox(hInstance,
        MAKEINTRESOURCE(IDD_PLAYER),
        NULL,
        PlayerDlgProc);
    return (int) ret;
}
