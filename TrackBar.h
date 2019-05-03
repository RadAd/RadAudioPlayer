#pragma once

DWORD TrackBar_GetPos(HWND hWnd)
{
    return (DWORD) SendMessage(hWnd, TBM_GETPOS, 0, 0);
}

void TrackBar_SetTicFreq(HWND hWnd, DWORD n)
{
    SendMessage(hWnd, TBM_SETTICFREQ, n, 0);
}

void TrackBar_SetLineSize(HWND hWnd, DWORD n)
{
    SendMessage(hWnd, TBM_SETLINESIZE, 0, n);
}

void TrackBar_SetPageSize(HWND hWnd, DWORD n)
{
    SendMessage(hWnd, TBM_SETPAGESIZE, 0, n);
}

void TrackBar_SetPos(HWND hWnd, DWORD n, BOOL bRedraw = TRUE)
{
    SendMessage(hWnd, TBM_SETPOS, bRedraw, n);
}

void TrackBar_SetRange(HWND hWnd, WORD min, WORD max, BOOL bRedraw = TRUE)
{
    SendMessage(hWnd, TBM_SETRANGE, bRedraw, MAKELONG(min, max));
}
