#include "text_list.h"

#include <windows.h>
#include <windowsx.h>

#include "util.h"

#define TEXT_LIST_CLASS_NAME L"TEXTLIST"

typedef struct
{
    TextListEntry *textListEntries;

    int itemCount;
    int itemCapacity;

    int maxItemWidth;
    int itemHeight;

    int scrollX;
    int scrollY;

    HFONT baseFont;

    int nextUniqueId;
} TextListData;

typedef struct
{
    int clientWidth;
    int clientHeight;
    int contentWidth;
    int contentHeight;
    int maxScrollX;
    int maxScrollY;
} ScrollStruct;

TextListData *GetTextListData(HWND hwnd)
{
    return (TextListData *) GetWindowLongPtrW(hwnd, GWLP_USERDATA);
}

int LookupTextListIndexFromUniqueId(TextListData *data, int uniqueId)
{
    if (data == NULL || uniqueId < 0)
    {
        return -1;
    }

    for (int i = 0; i < data->itemCount; i ++)
    {
        if (data->textListEntries[i].uniqueId == uniqueId)
        {
            return i;
        }
    }

    return -1;
}

TextListEntry *LookupTextListEntryFromUniqueId(TextListData *data, int uniqueId)
{
    if (data == NULL || uniqueId < 0)
    {
        return NULL;
    }

    int index = LookupTextListIndexFromUniqueId(data, uniqueId);
    if (index < 0)
    {
        return NULL;
    }

    return &(data->textListEntries[index]);
}

void CalculateScrollStruct(HWND hwnd, TextListData *textListData, ScrollStruct *scrollStruct)
{
    if (hwnd == NULL || textListData == NULL || scrollStruct == NULL)
    {
        return;
    }

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);

    int clientWidth   = clientRect.right - clientRect.left;
    int clientHeight  = clientRect.bottom - clientRect.top;
    int contentWidth  = textListData->maxItemWidth;
    int contentHeight = textListData->itemCount * textListData->itemHeight;
    int maxScrollX    = contentWidth - clientWidth;
    int maxScrollY    = contentHeight - clientHeight;

    if (maxScrollX < 0)
    {
        maxScrollX = 0;
    }
    if (maxScrollY < 0)
    {
        maxScrollY = 0;
    }
  
    scrollStruct->clientWidth   = clientWidth;
    scrollStruct->clientHeight  = clientHeight;
    scrollStruct->contentWidth  = contentWidth;
    scrollStruct->contentHeight = contentHeight;
    scrollStruct->maxScrollX    = maxScrollX;
    scrollStruct->maxScrollY    = maxScrollY;
}

void ValidateScrollX(int *scrollX, ScrollStruct *scrollStruct)
{
    if (*scrollX < 0)
    {
        *scrollX = 0;
    }
    if (*scrollX > scrollStruct->maxScrollX)
    {
        *scrollX = scrollStruct->maxScrollX;
    }
}

void ValidateScrollY(int *scrollY, ScrollStruct *scrollStruct)
{
    if (*scrollY < 0)
    {
        *scrollY = 0;
    }
    if (*scrollY > scrollStruct->maxScrollY)
    {
        *scrollY = scrollStruct->maxScrollY;
    }
}

void UpdateScrollBars(HWND hwnd)
{
    TextListData *data = GetTextListData(hwnd);
    if (data == NULL)
    {
        return;
    }

    ScrollStruct ss = {0};
    CalculateScrollStruct(hwnd, data, &ss);

    ValidateScrollX(&(data->scrollX), &ss);
    ValidateScrollY(&(data->scrollY), &ss);

    SCROLLINFO siX = {0};
    siX.cbSize     = sizeof(SCROLLINFO);
    siX.fMask      = SIF_RANGE | SIF_PAGE | SIF_POS;
    siX.nMin       = 0;
    siX.nMax       = ss.contentWidth > 0 ? ss.contentWidth - 1
                                         : 0;
    siX.nPage      = ss.clientWidth;
    siX.nPos       = data->scrollX;

    SCROLLINFO siY = {0};
    siY.cbSize     = sizeof(SCROLLINFO);
    siY.fMask      = SIF_RANGE | SIF_PAGE | SIF_POS;
    siY.nMin       = 0;
    siY.nMax       = ss.contentHeight > 0 ? ss.contentHeight - 1
                                          : 0;
    siY.nPage      = ss.clientHeight;
    siY.nPos       = data->scrollY;

    SetScrollInfo(hwnd, SB_HORZ, &siX, TRUE);
    SetScrollInfo(hwnd, SB_VERT, &siY, TRUE);
}

void UpdateInternalFont(TextListData *data, TextListEntry *entry)
{
    if (data == NULL || entry == NULL)
    {
        return;
    }

    if (entry->internalFont != NULL)
    {
        DeleteObject(entry->internalFont);
        entry->internalFont = NULL;
    }

    if (entry->fontStyle != TEXTLIST_FONT_STYLE_NONE)
    {
        LOGFONTW lf = {0};
        GetObjectW(data->baseFont, sizeof(LOGFONTW), &lf);

        lf.lfItalic    = entry->fontStyle & TEXTLIST_FONT_STYLE_ITALIC;
        lf.lfStrikeOut = entry->fontStyle & TEXTLIST_FONT_STYLE_STRIKEOUT;

        HFONT internalFont = CreateFontIndirectW(&lf);
        if (internalFont == NULL)
        {
            return;
        }

        entry->internalFont = internalFont;
    }
}

void RecalculateMaxItemWidth(HWND hwnd, TextListData *data, const wchar_t *text)
{
    if (data == NULL || text == NULL)
    {
        return;
    }

    int textWidth = CalculateTextWidth(hwnd, data->baseFont, text);
    if (textWidth < 0)
    {
        return;
    }

    textWidth += 16;

    if (textWidth > data->maxItemWidth)
    {
        data->maxItemWidth = textWidth;
    }
}

void SetFont(HWND hwnd, HFONT newBaseFont)
{
    TextListData *data = GetTextListData(hwnd);
    if (data == NULL || newBaseFont == NULL)
    {
        return;
    }

    int fontHeight = CalculateFontHeight(hwnd, newBaseFont);
    if (fontHeight < 0)
    {
        return;
    }

    data->baseFont     = newBaseFont;
    data->itemHeight   = fontHeight + 12;
    data->maxItemWidth = 0;

    if (data->textListEntries != NULL && data->itemCount > 0)
    {
        for (int i = 0; i < data->itemCount; i ++)
        {
            UpdateInternalFont(data, &(data->textListEntries[i]));
            RecalculateMaxItemWidth(hwnd, data, data->textListEntries[i].textContent);
        }
    }

    UpdateScrollBars(hwnd);

    InvalidateRect(hwnd, NULL, TRUE);
}

int CalculateEntryIndexFromCursorYPos(HWND hwnd, int cursorYPos)
{
    TextListData *data = GetTextListData(hwnd);
    if (data == NULL)
    {
        return -1;
    }

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);

    if (cursorYPos < 0 || cursorYPos >= clientRect.bottom)
    {
        return -1;
    }

    if (data->itemHeight == 0)
    {
        return -1;
    }

    int index = (cursorYPos + data->scrollY) / (data->itemHeight);
    if (index < 0 || index >= data->itemCount)
    {
        return -1;
    }

    return index;
}

void DeleteTextListEntries(TextListData *data)
{
    if (data == NULL)
    {
        return;
    }

    if (data->textListEntries != NULL)
    {
        for (int i = 0; i < data->itemCount; i ++)
        {
            if (data->textListEntries[i].textContent != NULL)
            {
                free(data->textListEntries[i].textContent);
                data->textListEntries[i].textContent = NULL;
            }
            if (data->textListEntries[i].internalFont != NULL)
            {
                DeleteObject(data->textListEntries[i].internalFont);
                data->textListEntries[i].internalFont = NULL;
            }
        }

        free(data->textListEntries);
        data->textListEntries = NULL;
    }
}

void DeleteTextListEntry(HWND hwnd, int uniqueId)
{
    TextListData *data = GetTextListData(hwnd);
    if (data == NULL || uniqueId < 0)
    {
        return;
    }

    int index = LookupTextListIndexFromUniqueId(data, uniqueId);
    if (index < 0)
    {
        return;
    }

    TextListEntry *entry = &(data->textListEntries[index]);
    if (entry->textContent != NULL)
    {
        free(entry->textContent);
        entry->textContent = NULL;
    }
    if (entry->internalFont != NULL)
    {
        DeleteObject(entry->internalFont);
        entry->internalFont = NULL;
    }

    int elementsToMoveCount = data->itemCount - index - 1;
    if (elementsToMoveCount > 0)
    {
        memmove(
            &(data->textListEntries[index]),
            &(data->textListEntries[index + 1]),
            elementsToMoveCount * sizeof(TextListEntry)
        );
    }

    (data->itemCount) --;

    UpdateScrollBars(hwnd);

    InvalidateRect(hwnd, NULL, TRUE);
}

LRESULT TextListWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg)
    {
        case WM_CREATE: {
            TextListData *data = (TextListData *) malloc(sizeof(TextListData));
            if (data == NULL)
            {
                return -1;
            }

            data->textListEntries = NULL;
            data->itemCount       = 0;
            data->itemCapacity    = 0;
            data->maxItemWidth    = 0;
            data->itemHeight      = 0;
            data->scrollX         = 0;
            data->scrollY         = 0;
            data->baseFont        = NULL;
            data->nextUniqueId    = 0;

            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) data);

            SetFont(hwnd, (HFONT) GetStockObject(DEFAULT_GUI_FONT));

            return 0;
        }
        case WM_NCDESTROY: {
            TextListData *data = GetTextListData(hwnd);
            if (data == NULL)
            {
                return DefWindowProcW(hwnd, wMsg, wParam, lParam);
            }

            DeleteTextListEntries(data);

            free(data);
            data = NULL;

            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) 0);

            return DefWindowProcW(hwnd, wMsg, wParam, lParam);
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT clientRect;
            GetClientRect(hwnd, &clientRect);

            FillRect(
                hdc,
                &clientRect,
                GetSysColorBrush(COLOR_WINDOW)
            );

            TextListData *data = GetTextListData(hwnd);
            if (data == NULL)
            {
                EndPaint(hwnd, &ps);

                return 0;
            }

            SetBkMode(hdc, TRANSPARENT);

            for (int i = 0; i < data->itemCount; i ++)
            {
                TextListEntry entry = data->textListEntries[i];

                HFONT fontToUse = entry.internalFont != NULL ? entry.internalFont
                                                             : data->baseFont;

                SetTextColor(hdc, entry.textColour);

                HFONT oldFont = (HFONT) SelectObject(hdc, fontToUse);
                
                RECT itemRect;
                itemRect.left   = 8 - data->scrollX;
                itemRect.top    = i * data->itemHeight - data->scrollY;
                itemRect.right  = 8 + data->maxItemWidth - data->scrollX;
                itemRect.bottom = itemRect.top + data->itemHeight;

                DrawTextW(
                    hdc,
                    entry.textContent,
                    -1,
                    &itemRect,
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE
                );

                SelectObject(hdc, oldFont);
            }

            EndPaint(hwnd, &ps);

            return 0;
        }
        case WM_HSCROLL: {
            TextListData *data = GetTextListData(hwnd);
            if (data == NULL)
            {
                return 0;
            }

            int averageCharWidth = CalculateAverageCharWidth(hwnd, data->baseFont);
            if (averageCharWidth < 0)
            {
                return 0;
            }

            ScrollStruct ss;
            CalculateScrollStruct(hwnd, data, &ss);
            int newScrollX = data->scrollX;

            switch (LOWORD(wParam))
            {
                case SB_LINELEFT: {
                    newScrollX -= averageCharWidth * 2;
                    break;
                }
                case SB_LINERIGHT: {
                    newScrollX += averageCharWidth * 2;
                    break;
                }
                case SB_PAGELEFT: {
                    newScrollX -= ss.clientWidth;
                    break;
                }
                case SB_PAGERIGHT: {
                    newScrollX += ss.clientWidth;
                    break;
                }
            
                case SB_THUMBTRACK:
                case SB_THUMBPOSITION: {
                    SCROLLINFO si = {0};
                    si.cbSize     = sizeof(SCROLLINFO);
                    si.fMask      = SIF_TRACKPOS;
            
                    GetScrollInfo(hwnd, SB_HORZ, &si);
            
                    newScrollX = si.nTrackPos;
            
                    break;
                }
            
                case SB_LEFT: {
                    newScrollX = 0;
                    break;
                }
                case SB_RIGHT: {
                    newScrollX = ss.maxScrollX;
                    break;
                }
            }
            
            ValidateScrollX(&newScrollX, &ss);
            
            SCROLLINFO si = {0};
            si.cbSize     = sizeof(SCROLLINFO);
            si.fMask      = SIF_POS;
            si.nPos       = newScrollX;
            
            SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
            
            data->scrollX = newScrollX;
            
            InvalidateRect(hwnd, NULL, TRUE);
            
            return 0;
        }
        case WM_VSCROLL: {
            TextListData *data = GetTextListData(hwnd);
            if (data == NULL)
            {
                return 0;
            }

            ScrollStruct ss = {0};
            CalculateScrollStruct(hwnd, data, &ss);

            int newScrollY = data->scrollY;

            switch (LOWORD(wParam))
            {
                case SB_LINEUP: {
                    newScrollY -= data->itemHeight;
                    break;
                }
                case SB_LINEDOWN: {
                    newScrollY += data->itemHeight;
                    break;
                }
                case SB_PAGEUP: {
                    newScrollY -= ss.clientHeight;
                    break;
                }
                case SB_PAGEDOWN: {
                    newScrollY += ss.clientHeight;
                    break;
                }

                case SB_THUMBTRACK:
                case SB_THUMBPOSITION: {
                    SCROLLINFO si = {0};
                    si.cbSize     = sizeof(SCROLLINFO);
                    si.fMask      = SIF_TRACKPOS;

                    GetScrollInfo(hwnd, SB_VERT, &si);

                    newScrollY = si.nTrackPos;
                    
                    break;
                }

                case SB_TOP: {
                    newScrollY = 0;
                    break;
                }
                case SB_BOTTOM: {
                    newScrollY = ss.maxScrollY;
                    break;
                }
            }

            ValidateScrollY(&newScrollY, &ss);

            SCROLLINFO si = {0};
            si.cbSize     = sizeof(SCROLLINFO);
            si.fMask      = SIF_POS;
            si.nPos       = newScrollY;
            
            SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

            data->scrollY = newScrollY;

            InvalidateRect(hwnd, NULL, TRUE);

            return 0;
        }
        case WM_MOUSEWHEEL: {
            TextListData *data = GetTextListData(hwnd);
            if (data == NULL)
            {
                return 0;
            }

            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            int lines = 3;

            int newScrollY = data->scrollY
                           - ((delta / WHEEL_DELTA) * lines * data->itemHeight);

            ScrollStruct ss;
            CalculateScrollStruct(hwnd, data, &ss);

            ValidateScrollY(&newScrollY, &ss);

            SCROLLINFO si = {0};
            si.cbSize     = sizeof(SCROLLINFO);
            si.fMask      = SIF_POS;
            si.nPos       = newScrollY;

            SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

            data->scrollY = newScrollY;

            InvalidateRect(hwnd, NULL, TRUE);

            return 0;
        }
        case WM_SIZE: {
            InvalidateRect(hwnd, NULL, TRUE);

            return 0;
        }
        case WM_ERASEBKGND: {
            return 1;
        }
        case WM_SETFONT: {
            SetFont(hwnd, (HFONT) wParam);

            return 0;
        }
        case WM_GETFONT: {
            TextListData *data = GetTextListData(hwnd);
            if (data == NULL)
            {
                return (LRESULT) NULL;
            }

            return (LRESULT) data->baseFont;
        }
        case WM_LBUTTONDOWN: {
            if (wParam != MK_LBUTTON)
            {
                return 0;
            }

            TextListData *data = GetTextListData(hwnd);
            if (data == NULL)
            {
                return 0;
            }

            int index = CalculateEntryIndexFromCursorYPos(hwnd, GET_Y_LPARAM(lParam));
            if (index < 0)
            {
                return 0;
            }

            HWND hwndParent = GetParent(hwnd);
            if (hwndParent == NULL)
            {
                return 0;
            }

            TextListClickDetail *detail = malloc(sizeof(TextListClickDetail));
            if (detail == NULL)
            {
                return 0;
            }
            detail->textListUniqueId     = data->textListEntries[index].uniqueId;
            detail->clickedClientPoint.x = GET_X_LPARAM(lParam);
            detail->clickedClientPoint.y = GET_Y_LPARAM(lParam);

            SendMessageW(
                hwndParent,
                WM_TEXTLIST_ENTRY_CLICKED,
                (WPARAM) detail,
                (LPARAM) hwnd
            );

            return 0;
        }
        case WM_RBUTTONDOWN: {
            if (wParam != MK_RBUTTON)
            {
                return 0;
            }

            TextListData *data = GetTextListData(hwnd);
            if (data == NULL)
            {
                return 0;
            }

            int index = CalculateEntryIndexFromCursorYPos(hwnd, GET_Y_LPARAM(lParam));
            if (index < 0)
            {
                return 0;
            }

            HWND hwndParent = GetParent(hwnd);
            if (hwndParent == NULL)
            {
                return 0;
            }

            TextListClickDetail *detail = malloc(sizeof(TextListClickDetail));
            if (detail == NULL)
            {
                return 0;
            }
            detail->textListUniqueId     = data->textListEntries[index].uniqueId;
            detail->clickedClientPoint.x = GET_X_LPARAM(lParam);
            detail->clickedClientPoint.y = GET_Y_LPARAM(lParam);

            SendMessageW(
                hwndParent,
                WM_TEXTLIST_ENTRY_RIGHTCLICKED,
                (WPARAM) detail,
                (LPARAM) hwnd
            );

            return 0;
        }
        case WM_TEXTLIST_ADD_ENTRY: {
            TextListData *data = GetTextListData(hwnd);
            if (data == NULL)
            {
                return -1;
            }

            if (data->itemCount >= data->itemCapacity)
            {
                int newCapacity = data->itemCapacity == 0 ? 8
                                                          : data->itemCapacity * 2;

                TextListEntry *newEntries = realloc(
                    data->textListEntries,
                    newCapacity * sizeof(TextListEntry)
                );
                if (newEntries == NULL)
                {
                    return -1;
                }

                data->textListEntries = newEntries;
                data->itemCapacity    = newCapacity;
            }

            TextListEntry *newEntry = (TextListEntry *) lParam;
            if (newEntry == NULL)
            {
                return -1;
            }

            wchar_t *copiedText = _wcsdup(newEntry->textContent);
            if (copiedText == NULL)
            {
                return -1;
            }

            int index    = data->itemCount;
            int uniqueId = data->nextUniqueId;
            data->textListEntries[index].textContent  = copiedText;
            data->textListEntries[index].fontStyle    = newEntry->fontStyle;
            data->textListEntries[index].textColour   = newEntry->textColour;
            data->textListEntries[index].internalFont = NULL;
            data->textListEntries[index].uniqueId     = uniqueId;

            (data->itemCount) ++;
            (data->nextUniqueId) ++;

            RecalculateMaxItemWidth(hwnd, data, copiedText);
            UpdateScrollBars(hwnd);

            UpdateInternalFont(data, &data->textListEntries[index]);

            InvalidateRect(hwnd, NULL, TRUE);

            return uniqueId;
        }
        case WM_TEXTLIST_SET_ENTRY_TEXT: {
            TextListData *data = GetTextListData(hwnd);
            if (data == NULL)
            {
                return -1;
            }

            int uniqueId = (int) wParam;
            if (uniqueId < 0)
            {
                return -1;
            }
            TextListEntry *entry = LookupTextListEntryFromUniqueId(data, uniqueId);
            if (entry == NULL)
            {
                return -1;
            }

            const wchar_t *newText = (const wchar_t *) lParam;
            if (newText == NULL)
            {
                return -1;
            }

            wchar_t *copiedText = _wcsdup(newText);
            if (copiedText == NULL)
            {
                return -1;
            }

            if (entry->textContent != NULL)
            {
                free(entry->textContent);
                entry->textContent = NULL;
            }
            entry->textContent = copiedText;

            UpdateScrollBars(hwnd);

            InvalidateRect(hwnd, NULL, TRUE);

            return 0;
        }
        case WM_TEXTLIST_SET_ENTRY_FONTSTYLE: {
            TextListData *data = GetTextListData(hwnd);
            if (data == NULL)
            {
                return -1;
            }

            int uniqueId = (int) wParam;
            if (uniqueId < 0)
            {
                return -1;
            }
            TextListEntry *entry = LookupTextListEntryFromUniqueId(data, uniqueId);
            if (entry == NULL)
            {
                return -1;
            }

            TextListFontStyle newFontStyle = (TextListFontStyle) lParam;

            entry->fontStyle = newFontStyle;

            UpdateInternalFont(data, entry);

            InvalidateRect(hwnd, NULL, TRUE);

            return 0;
        }
        case WM_TEXTLIST_SET_ENTRY_TEXTCOLOUR: {
            TextListData *data = GetTextListData(hwnd);
            if (data == NULL)
            {
                return -1;
            }

            int uniqueId = (int) wParam;
            if (uniqueId < 0)
            {
                return -1;
            }
            TextListEntry *entry = LookupTextListEntryFromUniqueId(data, uniqueId);
            if (entry == NULL)
            {
                return -1;
            }

            COLORREF newColour = (COLORREF) lParam;

            entry->textColour = newColour;

            InvalidateRect(hwnd, NULL, TRUE);

            return 0;
        }
        case WM_TEXTLIST_REMOVE_ENTRY: {
            int uniqueId = (int) lParam;
            if (uniqueId < 0)
            {
                return -1;
            }

            DeleteTextListEntry(hwnd, uniqueId);

            return 0;
        }
        case WM_TEXTLIST_CLEAR: {
            TextListData *data = GetTextListData(hwnd);
            if (data == NULL)
            {
                return -1;
            }

            DeleteTextListEntries(data);

            data->itemCount    = 0;
            data->itemCapacity = 0;
            data->maxItemWidth = 0;
            data->scrollX      = 0;
            data->scrollY      = 0;

            UpdateScrollBars(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);

            return 0;
        }
        default: {
            return DefWindowProcW(hwnd, wMsg, wParam, lParam);
        }
    }
}

BOOL InitialiseTextListClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wndClass   = {0};
    wndClass.cbSize        = sizeof(WNDCLASSEXW);
    wndClass.lpszClassName = TEXT_LIST_CLASS_NAME;
    wndClass.hCursor       = LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_ARROW));
    wndClass.hInstance     = hInstance;
    wndClass.style         = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc   = TextListWndProc;

    return RegisterClassExW(&wndClass);
}

