#include <Windows.h>
#include "MciUtils.h"

static void MciShowError(HWND hDlg, MCIERROR e)
{
    TCHAR buf[1024];
    mciGetErrorString(e, buf, ARRAYSIZE(buf));
    SendMessage(hDlg, WM_MCI_ERROR, 0, (LPARAM) buf);
}

static MCIERROR MciGeneric(HWND hDlg, MCIDEVICEID wDeviceID, UINT uMsg)
{
    MCI_GENERIC_PARMS gp = {};
    gp.dwCallback = MAKELONG(hDlg, 0);
    MCIERROR e = mciSendCommand(wDeviceID, uMsg, MCI_WAIT, (DWORD_PTR) &gp);
    if (e != MMSYSERR_NOERROR)
        MciShowError(hDlg, e);
    return e;
}

void MciClose(HWND hDlg, MCIDEVICEID& wDeviceID)
{
    if (wDeviceID != 0)
        MciGeneric(hDlg, wDeviceID, MCI_CLOSE);
    wDeviceID = 0;
}

MCIERROR MciOpen(HWND hDlg, MCIDEVICEID& wDeviceID, LPCTSTR pFileName)
{
    MciClose(hDlg, wDeviceID);

    MCI_OPEN_PARMS op = {};
    op.dwCallback = MAKELONG(hDlg, 0);
    op.lpstrElementName = pFileName;
    MCIERROR e = mciSendCommand(wDeviceID, MCI_OPEN, MCI_WAIT | MCI_OPEN_ELEMENT, (DWORD_PTR) &op);
    if (e != MMSYSERR_NOERROR)
    {
        //MciClose(hDlg, op.wDeviceID);
        MciShowError(hDlg, e);
    }
    else
        wDeviceID = op.wDeviceID;
    return e;
}

DWORD_PTR MciGetStatus(HWND hDlg, MCIDEVICEID wDeviceID, DWORD dwItem)
{
    MCI_STATUS_PARMS sp = {};
    sp.dwCallback = MAKELONG(hDlg, 0);
    sp.dwItem = dwItem;
    MCIERROR e = mciSendCommand(wDeviceID, MCI_STATUS, MCI_WAIT | MCI_STATUS_ITEM, (DWORD_PTR) &sp);
    if (e != MMSYSERR_NOERROR)
        MciShowError(hDlg, e);
    return sp.dwReturn;
}

void MciPlay(HWND hDlg, MCIDEVICEID wDeviceID)
{
    MCI_PLAY_PARMS pp = {};
    pp.dwCallback = MAKELONG(hDlg, 0);
    MCIERROR e = mciSendCommand(wDeviceID, MCI_PLAY, 0, (DWORD_PTR) &pp);
    if (e != MMSYSERR_NOERROR)
        MciShowError(hDlg, e);
}

void MciPause(HWND hDlg, MCIDEVICEID wDeviceID)
{
    MciGeneric(hDlg, wDeviceID, MCI_PAUSE);
}

void MciSeekTo(HWND hDlg, MCIDEVICEID wDeviceID, DWORD dwTo)
{
    MCI_SEEK_PARMS sp = {};
    sp.dwCallback = MAKELONG(hDlg, 0);
    sp.dwTo = dwTo;
    MCIERROR e = mciSendCommand(wDeviceID, MCI_SEEK, MCI_WAIT | MCI_TO, (DWORD_PTR) &sp);
    if (e != MMSYSERR_NOERROR)
        MciShowError(hDlg, e);
}
