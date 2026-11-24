#ifndef TEXT_LIST_H
#define TEXT_LIST_H

#include <windows.h>

#define WM_TEXTLIST_ADD_ENTRY            (WM_APP + 1001)
#define WM_TEXTLIST_SET_ENTRY_TEXT       (WM_APP + 1002)
#define WM_TEXTLIST_SET_ENTRY_FONTSTYLE  (WM_APP + 1003)
#define WM_TEXTLIST_SET_ENTRY_TEXTCOLOUR (WM_APP + 1004)
#define WM_TEXTLIST_ENTRY_CLICKED        (WM_APP + 1005)
#define WM_TEXTLIST_ENTRY_RIGHTCLICKED   (WM_APP + 1006)
#define WM_TEXTLIST_REMOVE_ENTRY         (WM_APP + 1007)
#define WM_TEXTLIST_CLEAR                (WM_APP + 1008)

typedef enum
{
    TEXTLIST_FONT_STYLE_NONE = 0,
    TEXTLIST_FONT_STYLE_ITALIC = 1 << 0,
    TEXTLIST_FONT_STYLE_STRIKEOUT = 1 << 1
} TextListFontStyle;

typedef struct
{
    int textListUniqueId;
    POINT clickedClientPoint;
} TextListClickDetail;

typedef struct
{
    int uniqueId;
    wchar_t *textContent;
    TextListFontStyle fontStyle;
    COLORREF textColour;

    HFONT internalFont;
} TextListEntry;

BOOL InitialiseTextListClass(HINSTANCE hInstance);

#endif

