#include "layout.h"

#include <windows.h>

#define ORIGINAL_SIZE_PROP_NAME L"ORIGINAL_SIZE"

void StoreOriginalSize(HWND hwnd)
{
    OriginalComponentSize *originalSize =
        (OriginalComponentSize *) malloc(sizeof(OriginalComponentSize));
    if (originalSize == NULL)
    {
        return;
    }
    
    RECT rect;
    GetWindowRect(hwnd, &rect);

    MapWindowPoints(HWND_DESKTOP, GetParent(hwnd), (POINT *)&rect, 2);

    originalSize->x      = rect.left;
    originalSize->y      = rect.top;
    originalSize->width  = rect.right - rect.left;
    originalSize->height = rect.bottom - rect.top;

    SetPropW(hwnd, ORIGINAL_SIZE_PROP_NAME, (HANDLE) originalSize);
}

OriginalComponentSize *GetOriginalSize(HWND hwnd)
{
    return (OriginalComponentSize *) GetPropW(hwnd, ORIGINAL_SIZE_PROP_NAME);
}

void RemoveOriginalSizeProp(HWND hwnd)
{
    OriginalComponentSize *originalSize = GetOriginalSize(hwnd);
    if (originalSize != NULL)
    {
        free(originalSize);
    }

    RemovePropW(hwnd, ORIGINAL_SIZE_PROP_NAME);
}

void ApplySizeDifference(HWND hwnd, ApplySizeDiffFlags sizeFlags, ApplyPosDiffFlags posFlags, SizeDifference *ss)
{
    OriginalComponentSize *origSize = GetOriginalSize(hwnd);
    if (origSize == NULL)
    {
        return;
    }

    int newWidth = origSize->width;
    int newHeight = origSize->height;
    int newX = origSize->x;
    int newY = origSize->y;

    if (sizeFlags & APPLY_SIZE_DIFF_WIDTH)
    {
        newWidth += ss->widthDifference;
    }
    if (sizeFlags & APPLY_SIZE_DIFF_HEIGHT)
    {
        newHeight += ss->heightDifference;
    }
    if (posFlags & APPLY_POS_DIFF_X)
    {
        newX += ss->widthDifference;
    }
    if (posFlags & APPLY_POS_DIFF_Y)
    {
        newY += ss->heightDifference;
    }

    SetWindowPos(hwnd, NULL, newX, newY, newWidth, newHeight, SWP_NOZORDER);
}

BOOL CalculateSizeDifference(HWND hwnd, SizeDifference *sd)
{
    OriginalComponentSize *origSize = GetOriginalSize(hwnd);
    if (origSize == NULL)
    {
        return FALSE;
    }

    RECT rect;
    GetWindowRect(hwnd, &rect);
    LONG currentWidth = rect.right - rect.left;
    LONG currentHeight = rect.bottom - rect.top;

    LONG widthDiff = currentWidth - origSize->width;
    LONG heightDiff = currentHeight - origSize->height;

    sd->widthDifference = widthDiff;
    sd->heightDifference = heightDiff;

    return TRUE;
}

