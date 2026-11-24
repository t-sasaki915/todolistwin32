#include "util.h"

#include <windows.h>
#include <wchar.h>

int LoadStringAndValidate(HINSTANCE hInstance, UINT uID, wchar_t **bufferPtr)
{
    const wchar_t *messagePointer = NULL;

    int length = LoadStringW(
        hInstance,
        uID,
        (LPWSTR) &messagePointer,
        0
    );

    if (length < 0 || messagePointer == NULL)
    {
        wchar_t message[512];
        swprintf(message, 512, L"Failed to load string %d.", uID);
        MessageBoxW(NULL, message, L"Resource Error", MB_ICONERROR | MB_OK);

        return -1;
    }

    wchar_t *buffer = (wchar_t *) calloc(length + 1, sizeof(wchar_t));
    if (buffer == NULL)
    {
        MessageBoxW(
            NULL,
            L"Failed to alocate memory.",
            L"Memory Error",
            MB_ICONERROR | MB_OK
        );

        return -1;
    }

    memcpy(buffer, messagePointer, length * sizeof(wchar_t));

    buffer[length] = L'\0';

    *bufferPtr = buffer;

    return length;
}

int MessageBoxUsingResourceString(HINSTANCE hInstance, HWND parentHWND, UINT uType, UINT titleID, UINT formatID, ...)
{
    va_list args;
    va_start(args, formatID);

    wchar_t *title = NULL;
    int titleLength = LoadStringAndValidate(hInstance, titleID, &title);
    if (titleLength < 0)
    {
        return 0;
    }

    wchar_t *format = NULL;
    int formatLength = LoadStringAndValidate(hInstance, formatID, &format);
    if (formatLength < 0)
    {
        if (title != NULL)
        {
            free(title);
        }

        return 0;
    }

    wchar_t message[1024];
    vswprintf(message, 1024, format, args);

    int result = MessageBoxW(parentHWND, message, title, uType);

    va_end(args);

    free(title);
    free(format);

    return result;
}

BOOL IsWhiteSpaceOnly(const wchar_t *text)
{
    BOOL result = TRUE;

    if (text == NULL)
    {
        return TRUE;
    }

    for (int i = 0; i < (int) wcslen(text); i ++)
    {
        if (text[i] != L' ' && text[i] != L'\t' && text[i] != L'\r' && text[i] != L'\n')
        {
            result = FALSE;
            break;
        }
    }

    return result;
}

int CalculateTextWidth(HWND hwnd, HFONT font, const wchar_t *text)
{
    HDC hdc = GetDC(hwnd);
    if (hdc == NULL)
    {
        return -1;
    }

    HFONT oldFont = (HFONT) SelectObject(hdc, font);

    SIZE size;
    GetTextExtentPoint32W(hdc, text, (int) wcslen(text), &size);

    SelectObject(hdc, oldFont);

    ReleaseDC(hwnd, hdc);

    return size.cx;
}

int CalculateFontHeight(HWND hwnd, HFONT font)
{
    HDC hdc = GetDC(hwnd);
    if (hdc == NULL)
    {
        return -1;
    }

    HFONT oldFont = (HFONT) SelectObject(hdc, font);

    TEXTMETRICW tm;
    GetTextMetricsW(hdc, &tm);

    SelectObject(hdc, oldFont);

    ReleaseDC(hwnd, hdc);

    return tm.tmHeight;
}

int CalculateAverageCharWidth(HWND hwnd, HFONT font)
{
    HDC hdc = GetDC(hwnd);
    if (hdc == NULL)
    {
        return -1;
    }

    HFONT oldFont = (HFONT) SelectObject(hdc, font);

    TEXTMETRICW tm;
    GetTextMetricsW(hdc, &tm);

    SelectObject(hdc, oldFont);

    ReleaseDC(hwnd, hdc);

    return tm.tmAveCharWidth;
}

