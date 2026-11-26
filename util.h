#ifndef UTIL_H
#define UTIL_H

#include <windows.h>

#define DIALOG_CANCEL_COND(wMsg, wParam) (wMsg == WM_COMMAND && LOWORD(wParam) == IDCANCEL)
#define DIALOG_INIT_COND(wMsg)           (wMsg == WM_INITDIALOG)
#define DIALOG_OK_COND(wMsg, wParam)     (wMsg == WM_COMMAND && LOWORD(wParam) == IDOK)

int LoadStringAndValidate(HINSTANCE hInstance, UINT uID, wchar_t **bufferPtr);

int MessageBoxUsingResourceString(HINSTANCE hInstance, HWND parentHWND, UINT uType, UINT titleID, UINT formatID, ...);

BOOL IsWhiteSpaceOnly(const wchar_t *text);

int CalculateTextWidth(HWND hwnd, HFONT font, const wchar_t *text);

int CalculateFontHeight(HWND hwnd, HFONT font);

int CalculateAverageCharWidth(HWND hwnd, HFONT font);

int PointSizeToHeight(HWND hwnd, int pointSize);

int HeightToPointSize(HWND hwnd, int height);

#endif

