#include <Windows.h>
#include <Windowsx.h>
#include <CommCtrl.h>
#include <tchar.h>

#include "MciUtils.h"
#include "TrackBar.h"
#include "resource.h"

// TODO
// Volume
// Text time
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

#define MS (1000)
#define WM_AP_OPEN (WM_USER + 10)

LPCTSTR PathFindName(LPCTSTR pFullName)
{
    LPCTSTR pName = _tcsrchr(pFullName, _T('\\'));
    return pName ? pName + 1 : pFullName;
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

    GetWindowText(hDlg, pData->strTitle, ARRAYSIZE(pData->strTitle));

    HICON hIcon = (HICON) LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PLAYER), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE);
    SendMessageW(hDlg, WM_SETICON, ICON_BIG, (LPARAM) hIcon);

    pData->hIcoPlay = (HICON) LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PLAY), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE);
    pData->hIcoPause = (HICON) LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PAUSE), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE);

    SetTimer(hDlg, 123, MS / 4, nullptr);
    TrackBar_SetTicFreq(hPosition, 60);
    TrackBar_SetLineSize(hPosition, 10);
    TrackBar_SetPageSize(hPosition, 60);

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
        pData->dwLastMode = dwMode;
    }

    if (!pData->bTracking && dwMode == MCI_MODE_PLAY)
    {
        DWORD dwPosition = (DWORD) MciGetStatus(hDlg, pData->wDeviceID, MCI_STATUS_POSITION);
        TrackBar_SetPos(hPosition, dwPosition / MS);
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

    if (MciOpen(hDlg, pData->wDeviceID, pFileName) == MMSYSERR_NOERROR)
    {
        TCHAR buf[1024];
        _stprintf_s(buf, _T("%s - %s"), PathFindName(pFileName), pData->strTitle);
        SetWindowText(hDlg, buf);

        //DWORD_PTR dwTimeFormat = MciGetStatus(hDlg, MCI_STATUS_TIME_FORMAT);
        DWORD dwLength = (DWORD) MciGetStatus(hDlg, pData->wDeviceID, MCI_STATUS_LENGTH);
        TrackBar_SetRange(hPosition, 0, (WORD) (dwLength / MS));
        MciPlay(hDlg, pData->wDeviceID);
    }
    else
    {
        SetWindowText(hDlg, pData->strTitle);
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

    if (id == IDC_POSITION)
    {
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
                }
                else if (pPosChanging->nReason != TB_THUMBPOSITION && pPosChanging->nReason != TB_ENDTRACK)
                {
                    DWORD_PTR dwMode = MciGetStatus(hDlg, pData->wDeviceID, MCI_STATUS_MODE);
                    MciSeekTo(hDlg, pData->wDeviceID, pPosChanging->dwPos * MS);
                    if (dwMode == MCI_MODE_PLAY)
                        MciPlay(hDlg, pData->wDeviceID);
                }
            }
            break;
        }
        return TRUE;
    }
    else
        return FALSE;
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
