#ifndef LAYOUT_H
#define LAYOUT_H

#include <windows.h>

typedef struct
{
    LONG x;
    LONG y;
    LONG width;
    LONG height;
} OriginalComponentSize;

typedef enum
{
    APPLY_SIZE_DIFF_NONE   = 0,
    APPLY_SIZE_DIFF_WIDTH  = 1 << 0,
    APPLY_SIZE_DIFF_HEIGHT = 1 << 1,
    APPLY_SIZE_DIFF_BOTH   = APPLY_SIZE_DIFF_WIDTH | APPLY_SIZE_DIFF_HEIGHT
} ApplySizeDiffFlags;

typedef enum
{
    APPLY_POS_DIFF_NONE = 0,
    APPLY_POS_DIFF_X    = 1 << 0,
    APPLY_POS_DIFF_Y    = 1 << 1,
    APPLY_POS_DIFF_BOTH = APPLY_POS_DIFF_X | APPLY_POS_DIFF_Y
} ApplyPosDiffFlags;

typedef struct
{
    LONG widthDifference;
    LONG heightDifference;
} SizeDifference;

void StoreOriginalSize(HWND hwnd);

void RemoveOriginalSizeProp(HWND hwnd);

OriginalComponentSize *GetOriginalSize(HWND hwnd);

void ApplySizeDifference(HWND hwnd, ApplySizeDiffFlags sizeFlags, ApplyPosDiffFlags posFlags, SizeDifference *ss);

BOOL CalculateSizeDifference(HWND hwnd, SizeDifference *sd);

#endif

