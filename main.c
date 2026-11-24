#include <windows.h>
#include <wchar.h>

#include "text_list.h"
#include "task_list_file.h"
#include "layout.h"
#include "simple_ini.h"
#include "util.h"
#include "resource.h"

typedef struct
{
    int textListUniqueId;
    TaskEntry taskEntry;
} TextListUniqueIdAndTaskEntry;

typedef struct
{
    TextListUniqueIdAndTaskEntry *entries;
    wchar_t *currentTaskListFilePath;
    wchar_t *currentTaskListFriendlyName;
    BOOL hasUnsavedChanges;
    
    int entryCount;
    int entryCapacity;

    HFONT customTextListFont;

    BOOL topMostEnabled;

    int taskItemRightClickMenuTarget;
} TaskListState;

static HINSTANCE g_hInstance;

static HWND  g_mainDialog             = NULL;
static HWND  g_addButton              = NULL;
static HWND  g_entry                  = NULL;
static HWND  g_taskFrame              = NULL;
static HMENU g_mainDialogMenu         = NULL;
static HMENU g_taskItemRightClickMenu = NULL;

static TaskListState *g_appState = NULL;

int LookupTaskEntryIndexByTextListUniqueId(int textListUniqueId)
{
    if (textListUniqueId < 0)
    {
        return -1;
    }

    for (int i = 0; i < g_appState->entryCount; i ++)
    {
        if (g_appState->entries[i].textListUniqueId == textListUniqueId)
        {
            return i;
        }
    }

    return -1;
}

TaskEntry *LookupTaskEntryByTextListUniqueId(int textListUniqueId)
{
    if (textListUniqueId < 0)
    {
        return NULL;
    }

    int index = LookupTaskEntryIndexByTextListUniqueId(textListUniqueId);
    if (index < 0)
    {
        return NULL;
    }

    return &(g_appState->entries[index].taskEntry);
}

void DeleteTaskEntries(void)
{
    if (g_appState->entries != NULL)
    {
        for (int i = 0; i < g_appState->entryCount; i ++)
        {
            if (g_appState->entries[i].taskEntry.taskDetail != NULL)
            {
                free(g_appState->entries[i].taskEntry.taskDetail);
                g_appState->entries[i].taskEntry.taskDetail = NULL;
            }
        }

        free(g_appState->entries);
        g_appState->entries = NULL;
    }

    g_appState->entryCount    = 0;
    g_appState->entryCapacity = 0;

    SendMessageW(g_taskFrame, WM_TEXTLIST_CLEAR, 0, 0);
}

void DeleteTaskEntry(int textListUniqueId)
{
    if (textListUniqueId < 0)
    {
        return;
    }

    int index = LookupTaskEntryIndexByTextListUniqueId(textListUniqueId);
    if (index < 0)
    {
        return;
    }

    TaskEntry *taskEntry = &(g_appState->entries[index].taskEntry);
    if (taskEntry->taskDetail != NULL)
    {
        free(taskEntry->taskDetail);
        taskEntry->taskDetail = NULL;
    }

    int elementsToMoveCount = g_appState->entryCount - index - 1;
    if (elementsToMoveCount > 0)
    {
        memmove(
            &(g_appState->entries[index]),
            &(g_appState->entries[index + 1]),
            elementsToMoveCount * sizeof(TextListUniqueIdAndTaskEntry)
        );
    }

    (g_appState->entryCount) --;

    SendMessageW(
        g_taskFrame,
        WM_TEXTLIST_REMOVE_ENTRY,
        (WPARAM) 0,
        (LPARAM) textListUniqueId
    );
}

void UpdateMainDialogTitle(void)
{
    wchar_t *friendlyName = NULL;
    if (g_appState->currentTaskListFriendlyName != NULL)
    {
        friendlyName = _wcsdup(g_appState->currentTaskListFriendlyName);
    }
    else
    {
        if (LoadStringAndValidate(g_hInstance, IDS_UNTITLED, &friendlyName) < 0)
        {
            return;
        }
    }

    UINT titleFormatID = g_appState->hasUnsavedChanges ? IDS_MAINDIALOG_TITLE_EDITED
                                                       : IDS_MAINDIALOG_TITLE;

    wchar_t *titleFormat = NULL;
    if (LoadStringAndValidate(g_hInstance, titleFormatID, &titleFormat) < 0)
    {
        free(friendlyName);

        return;
    }

    wchar_t title[1024];
    swprintf(
        title,
        1024,
        titleFormat,
        friendlyName
    );

    SetWindowTextW(g_mainDialog, title);

    free(friendlyName);
    free(titleFormat);
}

void GetFontStyleAndColour(TaskEntry *entry, TextListFontStyle *fontStyle, COLORREF *textColour)
{
    if (entry->isDone)
    {
        *fontStyle  = TEXTLIST_FONT_STYLE_STRIKEOUT;
        *textColour = RGB(150, 150, 150);
    }
    else
    {
        *fontStyle  = TEXTLIST_FONT_STYLE_NONE;
        *textColour = GetSysColor(COLOR_WINDOWTEXT);
    }
}

void ThereAreUnsavedChanges(void)
{
    g_appState->hasUnsavedChanges = TRUE;
    UpdateMainDialogTitle();
}

void ThereAreNoUnsavedChanges(void)
{
    g_appState->hasUnsavedChanges = FALSE;
    UpdateMainDialogTitle();
}

void AddTask(TaskEntry *entry)
{
    if (g_appState->entryCount >= g_appState->entryCapacity)
    {
        int newCapacity = g_appState->entryCapacity == 0 ? 8
                                                         : g_appState->entryCapacity * 2;

        TextListUniqueIdAndTaskEntry *newEntries = realloc(
            g_appState->entries,
            newCapacity * sizeof(TextListUniqueIdAndTaskEntry)
        );
        if (newEntries == NULL)
        {
            MessageBoxUsingResourceString(
                g_hInstance,
                g_mainDialog,
                MB_ICONERROR | MB_OK,
                IDS_MEMORY_ERROR_TITLE,
                IDS_FAILED_TO_ALLOCATE_MEMORY_MSG
            );

            return;
        }

        g_appState->entries       = newEntries;
        g_appState->entryCapacity = newCapacity;
    }

    TextListFontStyle fontStyle;
    COLORREF textColour;
    GetFontStyleAndColour(entry, &fontStyle, &textColour);

    TextListEntry newEntry = {0};
    newEntry.textContent   = entry->taskDetail;
    newEntry.fontStyle     = fontStyle;
    newEntry.textColour    = textColour;

    int textListUniqueId = (int) SendMessageW(
        g_taskFrame,
        WM_TEXTLIST_ADD_ENTRY,
        (WPARAM) 0,
        (LPARAM) &newEntry
    );
    if (textListUniqueId < 0)
    {
        return;
    }

    int index = g_appState->entryCount;
    g_appState->entries[index].textListUniqueId     = textListUniqueId;
    g_appState->entries[index].taskEntry.taskDetail = _wcsdup(entry->taskDetail);
    g_appState->entries[index].taskEntry.isDone     = entry->isDone;

    (g_appState->entryCount) ++;
    
    SetWindowTextW(g_entry, L"");

    ThereAreUnsavedChanges();
}

void GetEntryAndAddTask(void)
{
    wchar_t entryContent[1024];
    GetWindowTextW(g_entry, entryContent, 1024);

    if (wcslen(entryContent) == 0 || IsWhiteSpaceOnly(entryContent))
    {
        MessageBoxUsingResourceString(
            g_hInstance,
            g_mainDialog,
            MB_ICONINFORMATION | MB_OK,
            IDS_INFORMATION_TITLE,
            IDS_PLEASE_ENTER_TASK_NAME_MSG
        );

        SetWindowTextW(g_entry, L"");

        return;
    }

    TaskEntry entry  = {0};
    entry.taskDetail = entryContent;
    entry.isDone     = FALSE;
    AddTask(&entry);
}

void SaveTaskListFile(wchar_t *filePath)
{
    TaskEntry *taskEntries = calloc(g_appState->entryCount, sizeof(TaskEntry));
    if (taskEntries == NULL)
    {
        MessageBoxUsingResourceString(
            g_hInstance,
            g_mainDialog,
            MB_ICONERROR | MB_OK,
            IDS_MEMORY_ERROR_TITLE,
            IDS_FAILED_TO_ALLOCATE_MEMORY_MSG
        );

        return;
    }

    for (int i = 0; i < g_appState->entryCount; i ++)
    {
        taskEntries[i] = g_appState->entries[i].taskEntry;
    }

    TaskListFile taskListFile  = {0};
    taskListFile.friendlyName  = g_appState->currentTaskListFriendlyName;
    taskListFile.taskEntries   = taskEntries;
    taskListFile.taskItemCount = g_appState->entryCount;

    if (!WriteTaskListFile(filePath, &taskListFile))
    {
        return;
    }

    if (g_appState->currentTaskListFilePath != NULL)
    {
        free(g_appState->currentTaskListFilePath);
        g_appState->currentTaskListFilePath = NULL;
    }
    g_appState->currentTaskListFilePath = _wcsdup(filePath);

    ThereAreNoUnsavedChanges();
}

BOOL SaveTaskListAs(void)
{
    wchar_t szFile[1024];

    OPENFILENAMEW ofn   = {0};
    ofn.lStructSize     = sizeof(OPENFILENAMEW);
    ofn.hwndOwner       = g_mainDialog;
    ofn.lpstrFile       = szFile;
    ofn.lpstrFile[0]    = L'\0';
    ofn.nMaxFile        = sizeof(szFile);
    ofn.lpstrFilter     = L"Todo List Files (*.tdl)\0*.tdl\0\0";
    ofn.lpstrDefExt     = L"tdl";
    ofn.nFilterIndex    = 1;
    ofn.lpstrFileTitle  = NULL;
    ofn.nMaxFileTitle   = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags           = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (!GetSaveFileNameW(&ofn))
    {
        return FALSE;
    }

    SaveTaskListFile(ofn.lpstrFile);

    return TRUE;
}

BOOL SaveTaskList(void)
{
    if (g_appState->currentTaskListFilePath != NULL)
    {
        SaveTaskListFile(g_appState->currentTaskListFilePath);

        return TRUE;
    }
    else
    {
        return SaveTaskListAs();
    }
}

BOOL ConfirmSaveChangesBeforeContinuing(void)
{
    if (g_appState->hasUnsavedChanges)
    {
        int promptResult = MessageBoxUsingResourceString(
            g_hInstance,
            g_mainDialog,
            MB_ICONQUESTION | MB_YESNOCANCEL,
            IDS_CONFIRM_DIALOG_TITLE,
            IDS_SAVE_CHANGES_PROMPT_MSG
        );

        if (promptResult == IDYES)
        {
            return SaveTaskList();
        }
        else if (promptResult == IDCANCEL)
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    else
    {
        return TRUE;
    }
}

void NewFile(void)
{
    if (!ConfirmSaveChangesBeforeContinuing())
    {
        return;
    }

    DeleteTaskEntries();

    if (g_appState->currentTaskListFilePath != NULL)
    {
        free(g_appState->currentTaskListFilePath);
        g_appState->currentTaskListFilePath = NULL;
    }

    if (g_appState->currentTaskListFriendlyName != NULL)
    {
        free(g_appState->currentTaskListFriendlyName);
        g_appState->currentTaskListFriendlyName = NULL;
    }

    ThereAreNoUnsavedChanges();
}

void OpenTaskListFile(void)
{
    wchar_t szFile[1024];

    OPENFILENAMEW ofn   = {0};
    ofn.lStructSize     = sizeof(OPENFILENAMEW);
    ofn.hwndOwner       = g_mainDialog;
    ofn.lpstrFile       = szFile;
    ofn.lpstrFile[0]    = L'\0';
    ofn.nMaxFile        = sizeof(szFile);
    ofn.lpstrFilter     = L"Todo List Files (*.tdl)\0*.tdl\0\0";
    ofn.lpstrDefExt     = L"tdl";
    ofn.nFilterIndex    = 1;
    ofn.lpstrFileTitle  = NULL;
    ofn.nMaxFileTitle   = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags           = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (!GetOpenFileNameW(&ofn))
    {
        return;
    }

    TaskListFile taskListFile = {0};
    if (!ReadTaskListFile(ofn.lpstrFile, &taskListFile))
    {
        return;
    }

    if (!ConfirmSaveChangesBeforeContinuing())
    {
        return;
    }

    DeleteTaskEntries();

    for (int i = 0; i < taskListFile.taskItemCount; i ++)
    {
        AddTask(&taskListFile.taskEntries[i]);
    }

    if (g_appState->currentTaskListFilePath != NULL)
    {
        free(g_appState->currentTaskListFilePath);
        g_appState->currentTaskListFilePath = NULL;
    }
    g_appState->currentTaskListFilePath = _wcsdup(ofn.lpstrFile);

    if (g_appState->currentTaskListFriendlyName != NULL)
    {
        free(g_appState->currentTaskListFriendlyName);
        g_appState->currentTaskListFriendlyName = NULL;
    }
    g_appState->currentTaskListFriendlyName = _wcsdup(taskListFile.friendlyName);

    for (int i = 0; i < taskListFile.taskItemCount; i ++)
    {
        free(taskListFile.taskEntries[i].taskDetail);
        taskListFile.taskEntries[i].taskDetail = NULL;
    }
    free(taskListFile.taskEntries);
    taskListFile.taskEntries = NULL;

    free(taskListFile.friendlyName);
    taskListFile.friendlyName = NULL;

    ThereAreNoUnsavedChanges();
}

void ToggleAlwaysOnTop(void)
{
    g_appState->topMostEnabled = !g_appState->topMostEnabled;

    MENUITEMINFOW itemInfo = {0};
    itemInfo.cbSize        = sizeof(MENUITEMINFOW);
    itemInfo.fMask         = MIIM_STATE;
    GetMenuItemInfoW(g_mainDialogMenu, ID_VIEW_ALWAYSONTOP, FALSE, &itemInfo);

    itemInfo.fState = g_appState->topMostEnabled ? MFS_CHECKED : MFS_UNCHECKED;

    SetMenuItemInfoW(g_mainDialogMenu, ID_VIEW_ALWAYSONTOP, FALSE, &itemInfo);

    if (g_appState->topMostEnabled)
    {
        SetWindowPos(g_mainDialog, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    else
    {
        SetWindowPos(g_mainDialog, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
}

void SelectTaskAreaFont(void)
{
    HFONT currentFont = (HFONT) SendMessageW(g_taskFrame, WM_GETFONT, 0, 0);
    if (currentFont == NULL)
    {
        return;
    }

    LOGFONTW lf = {0};
    GetObjectW(currentFont, sizeof(LOGFONTW), &lf);

    CHOOSEFONTW cf = {0};
    cf.lStructSize = sizeof(CHOOSEFONTW);
    cf.hwndOwner   = g_mainDialog;
    cf.lpLogFont   = &lf;
    cf.rgbColors   = RGB(0, 0, 0);
    cf.Flags       = CF_SCREENFONTS | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT;

    if (!ChooseFontW(&cf))
    {
        return;
    }

    HFONT newFont = CreateFontIndirectW(cf.lpLogFont);
    if (newFont == NULL)
    {
        return;
    }

    if (g_appState->customTextListFont != NULL)
    {
        DeleteObject(g_appState->customTextListFont);
        g_appState->customTextListFont = NULL;
    }
    g_appState->customTextListFont = newFont;

    SendMessageW(
        g_taskFrame,
        WM_SETFONT,
        (WPARAM) newFont,
        (LPARAM) TRUE
    );
}

void ToggleTaskStatus(int textListUniqueId)
{
    if (textListUniqueId < 0)
    {
        return;
    }

    TaskEntry *taskEntry = LookupTaskEntryByTextListUniqueId(textListUniqueId);
    if (taskEntry == NULL)
    {
        return;
    }

    taskEntry->isDone = !taskEntry->isDone;

    TextListFontStyle newFontStyle;
    COLORREF newTextColour;
    GetFontStyleAndColour(taskEntry, &newFontStyle, &newTextColour);

    SendMessageW(
        g_taskFrame,
        WM_TEXTLIST_SET_ENTRY_FONTSTYLE,
        (WPARAM) textListUniqueId,
        (LPARAM) newFontStyle
    );
    SendMessageW(
        g_taskFrame,
        WM_TEXTLIST_SET_ENTRY_TEXTCOLOUR,
        (WPARAM) textListUniqueId,
        (LPARAM) newTextColour
    );

    ThereAreUnsavedChanges();
}

void AskAndDeleteTaskEntry(int textListUniqueId)
{
    if (textListUniqueId < 0)
    {
        return;
    }

    int msgBoxResult = MessageBoxUsingResourceString(
        g_hInstance,
        g_mainDialog,
        MB_ICONQUESTION | MB_YESNO,
        IDS_CONFIRM_DIALOG_TITLE,
        IDS_DELETE_ITEM_PROMPT_MSG
    );

    if (msgBoxResult == IDYES)
    {
        DeleteTaskEntry(textListUniqueId);

        ThereAreUnsavedChanges();
    }
}

INT_PTR CALLBACK VersionDialogDlgProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    (void) lParam;

    if (DIALOG_CANCEL_COND(wMsg, wParam))
    {
        EndDialog(hwnd, 0);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

INT_PTR CALLBACK FriendlyNameChangeDialogDlgProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    (void) lParam;

    if (DIALOG_CANCEL_COND(wMsg, wParam))
    {
        EndDialog(hwnd, 0);

        return TRUE;
    }
    else if (DIALOG_INIT_COND(wMsg))
    {
        if (g_appState->currentTaskListFilePath != NULL)
        {
            SetDlgItemTextW(hwnd, IDC_NEWTASKLISTNAME, g_appState->currentTaskListFriendlyName);
        }

        SetFocus(GetDlgItem(hwnd, IDC_NEWTASKLISTNAME));

        return FALSE;
    }
    else if (DIALOG_OK_COND(wMsg, wParam))
    {
        wchar_t newName[1024];
        GetDlgItemTextW(hwnd, IDC_NEWTASKLISTNAME, newName, 1024);

        if (g_appState->currentTaskListFriendlyName != NULL)
        {
            free(g_appState->currentTaskListFriendlyName);
            g_appState->currentTaskListFriendlyName = NULL;
        }

        if (wcslen(newName) != 0 && !IsWhiteSpaceOnly(newName))
        {
            g_appState->currentTaskListFriendlyName = _wcsdup(newName);
        }

        ThereAreUnsavedChanges();

        EndDialog(hwnd, 0);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

INT_PTR CALLBACK TaskDetailEditDialogDlgProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    if (DIALOG_CANCEL_COND(wMsg, wParam))
    {
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) 0);

        EndDialog(hwnd, 0);

        return TRUE;
    }
    else if (DIALOG_INIT_COND(wMsg))
    {
        int textListUniqueId = (int) lParam;
        if (textListUniqueId < 0)
        {
            return TRUE;
        }

        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) textListUniqueId);

        TaskEntry *taskEntry = LookupTaskEntryByTextListUniqueId(textListUniqueId);
        if (taskEntry == NULL)
        {
            return TRUE;
        }

        SetDlgItemTextW(hwnd, IDC_NEWTASKDETAIL, taskEntry->taskDetail);

        SetFocus(GetDlgItem(hwnd, IDC_NEWTASKDETAIL));

        return FALSE;
    }
    else if (DIALOG_OK_COND(wMsg, wParam))
    {
        wchar_t newDetail[1024];
        GetDlgItemTextW(hwnd, IDC_NEWTASKDETAIL, newDetail, 1024);

        if (wcslen(newDetail) == 0 || IsWhiteSpaceOnly(newDetail))
        {
            MessageBoxUsingResourceString(
                g_hInstance,
                hwnd,
                MB_ICONINFORMATION | MB_OK,
                IDS_INFORMATION_TITLE,
                IDS_PLEASE_ENTER_TASK_NAME_MSG
            );

            SetDlgItemTextW(hwnd, IDC_NEWTASKDETAIL, L"");

            return TRUE;
        }

        int textListUniqueId = (int) GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (textListUniqueId < 0)
        {
            return TRUE;
        }

        TaskEntry *taskEntry = LookupTaskEntryByTextListUniqueId(textListUniqueId);
        if (taskEntry == NULL)
        {
            return TRUE;
        }

        if (taskEntry->taskDetail != NULL)
        {
            free(taskEntry->taskDetail);
            taskEntry->taskDetail = NULL;
        }
        taskEntry->taskDetail = _wcsdup(newDetail);

        SendMessageW(
            g_taskFrame,
            WM_TEXTLIST_SET_ENTRY_TEXT,
            (WPARAM) textListUniqueId,
            (LPARAM) newDetail
        );

        ThereAreUnsavedChanges();

        EndDialog(hwnd, 0);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

INT_PTR CALLBACK MainDialogDlgProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg)
    {
        case WM_INITDIALOG: {
            InitialiseTaskListFile(g_hInstance, hwnd);

            g_mainDialog     = hwnd;
            g_addButton      = GetDlgItem(g_mainDialog, IDC_ADDBUTTON);
            g_entry          = GetDlgItem(g_mainDialog, IDC_ENTRY);
            g_taskFrame      = GetDlgItem(g_mainDialog, IDC_TASKFRAME);
            g_mainDialogMenu = GetMenu(g_mainDialog);

            StoreOriginalSize(g_mainDialog);
            StoreOriginalSize(g_addButton);
            StoreOriginalSize(g_entry);
            StoreOriginalSize(g_taskFrame);

            g_taskItemRightClickMenu =
                LoadMenuW(g_hInstance, MAKEINTRESOURCEW(IDR_TASKITEMRIGHTCLICKMENU));
            if (g_taskItemRightClickMenu == NULL)
            {
                EndDialog(g_mainDialog, 1);
                PostQuitMessage(1);

                return TRUE;
            }

            g_appState = malloc(sizeof(TaskListState));
            if (g_appState == NULL)
            {
                MessageBoxUsingResourceString(
                    g_hInstance,
                    g_mainDialog,
                    MB_ICONERROR | MB_OK,
                    IDS_MEMORY_ERROR_TITLE,
                    IDS_FAILED_TO_ALLOCATE_MEMORY_MSG
                );

                EndDialog(g_mainDialog, 1);
                PostQuitMessage(1);

                return TRUE;
            }

            g_appState->entries                      = NULL;
            g_appState->currentTaskListFilePath      = NULL;
            g_appState->currentTaskListFriendlyName  = NULL;
            g_appState->hasUnsavedChanges            = FALSE;
            g_appState->entryCount                   = 0;
            g_appState->entryCapacity                = 0;
            g_appState->customTextListFont           = NULL;
            g_appState->topMostEnabled               = FALSE;
            g_appState->taskItemRightClickMenuTarget = -1;

            NewFile();
            ToggleAlwaysOnTop();

            SetFocus(g_entry);

            return FALSE;
        }
        case WM_GETMINMAXINFO: {
            MINMAXINFO *minMaxInfo = (MINMAXINFO *)lParam;

            OriginalComponentSize *originalSize = GetOriginalSize(g_mainDialog);
            if (originalSize != NULL)
            {
                minMaxInfo->ptMinTrackSize.x = originalSize->width;
                minMaxInfo->ptMinTrackSize.y = originalSize->height;
            }

            return TRUE;
        }
        case WM_SIZE: {
            SizeDifference sd = {0};
            if (!CalculateSizeDifference(g_mainDialog, &sd))
            {
                return TRUE;
            }

            ApplySizeDifference(
                g_addButton,
                APPLY_SIZE_DIFF_NONE,
                APPLY_POS_DIFF_X,
                &sd
            );
            ApplySizeDifference(
                g_entry,
                APPLY_SIZE_DIFF_WIDTH,
                APPLY_POS_DIFF_NONE,
                &sd
            );
            ApplySizeDifference(
                g_taskFrame,
                APPLY_SIZE_DIFF_BOTH,
                APPLY_POS_DIFF_NONE,
                &sd
            );

            return TRUE;
        }
        case WM_COMMAND: {
            switch (LOWORD(wParam))
            {
                case IDCANCEL: {
                    SendMessageW(hwnd, WM_CLOSE, (WPARAM) 0, (LPARAM) 0);

                    return TRUE;
                }
                case IDOK: {
                    GetEntryAndAddTask();

                    return TRUE;
                }
                case IDC_ADDBUTTON: {
                    GetEntryAndAddTask();

                    return TRUE;
                }
                case ID_FILE_NEW: {
                    NewFile();

                    return TRUE;
                }
                case ID_FILE_OPEN: {
                    OpenTaskListFile();

                    return TRUE;
                }
                case ID_FILE_SAVE: {
                    SaveTaskList();

                    return TRUE;
                }
                case ID_FILE_SAVEAS: {
                    SaveTaskListAs();

                    return TRUE;
                }
                case ID_FILE_RENAMETASKLIST: {
                    DialogBoxW(
                        g_hInstance,
                        MAKEINTRESOURCEW(IDD_FRIENDLYNAMECHANGEDIALOG),
                        g_mainDialog,
                        FriendlyNameChangeDialogDlgProc
                    );

                    return TRUE;
                }
                case ID_FILE_EXIT: {
                    SendMessageW(hwnd, WM_CLOSE, (WPARAM) 0, (LPARAM) 0);

                    return TRUE;
                }
                case ID_VIEW_ALWAYSONTOP: {
                    ToggleAlwaysOnTop();

                    return TRUE;
                }
                case ID_VIEW_FONT: {
                    SelectTaskAreaFont();

                    return TRUE;
                }
                case ID_HELP_ABOUTSIMPLETASKLIST: {
                    DialogBoxW(
                        g_hInstance,
                        MAKEINTRESOURCEW(IDD_VERSIONDIALOG),
                        g_mainDialog,
                        VersionDialogDlgProc
                    );

                    return TRUE;
                }
                case ID_TASKITEMRIGHTCLICKMENU_MARKASCOMPLETED: {
                    ToggleTaskStatus(g_appState->taskItemRightClickMenuTarget);

                    return TRUE;
                }
                case ID_TASKITEMRIGHTCLICKMENU_EDIT: {
                    DialogBoxParamW(
                        g_hInstance,
                        MAKEINTRESOURCEW(IDD_TASKDETAILEDITDIALOG),
                        g_mainDialog,
                        TaskDetailEditDialogDlgProc,
                        (LPARAM) g_appState->taskItemRightClickMenuTarget
                    );

                    return TRUE;
                }
                case ID_TASKITEMRIGHTCLICKMENU_DELETE: {
                    AskAndDeleteTaskEntry(g_appState->taskItemRightClickMenuTarget);

                    return TRUE;
                }
                default: {
                    return FALSE;
                }
            }
        }
        case WM_TEXTLIST_ENTRY_CLICKED: {
            if (((HWND) lParam) != g_taskFrame)
            {
                return TRUE;
            }

            TextListClickDetail *detail = (TextListClickDetail *) wParam;
            int textListUniqueId = detail->textListUniqueId;

            free(detail);

            ToggleTaskStatus(textListUniqueId);

            return TRUE;
        }
        case WM_TEXTLIST_ENTRY_RIGHTCLICKED: {
            if (((HWND) lParam) != g_taskFrame)
            {
                return TRUE;
            }

            TextListClickDetail *detail = (TextListClickDetail *) wParam;
            int textListUniqueId = detail->textListUniqueId;
            POINT point = detail->clickedClientPoint;

            free(detail);

            TaskEntry *taskEntry = LookupTaskEntryByTextListUniqueId(textListUniqueId);
            if (taskEntry == NULL)
            {
                return TRUE;
            }

            HMENU rightClickMenu = GetSubMenu(g_taskItemRightClickMenu, 0);
            if (rightClickMenu == NULL)
            {
                return TRUE;
            }

            MENUITEMINFOW markAsCompletedInfo = {0};
            markAsCompletedInfo.cbSize = sizeof(MENUITEMINFOW);
            markAsCompletedInfo.fMask  = MIIM_STATE;
            GetMenuItemInfoW(
                rightClickMenu,
                ID_TASKITEMRIGHTCLICKMENU_MARKASCOMPLETED,
                FALSE,
                &markAsCompletedInfo
            );

            markAsCompletedInfo.fState = taskEntry->isDone ? MFS_CHECKED : MFS_UNCHECKED;

            SetMenuItemInfoW(
                rightClickMenu,
                ID_TASKITEMRIGHTCLICKMENU_MARKASCOMPLETED,
                FALSE,
                &markAsCompletedInfo
            );

            g_appState->taskItemRightClickMenuTarget = textListUniqueId;

            ClientToScreen(g_taskFrame, &point);
            TrackPopupMenu(
                rightClickMenu,
                TPM_RIGHTBUTTON,
                point.x,
                point.y,
                0,
                g_mainDialog,
                NULL
            );

            return TRUE;
        }
        case WM_CLOSE: {
            if (!ConfirmSaveChangesBeforeContinuing())
            {
                return TRUE;
            }

            EndDialog(hwnd, 0);
            PostQuitMessage(0);

            return TRUE;
        }
        case WM_DESTROY: {
            RemoveOriginalSizeProp(g_mainDialog);
            RemoveOriginalSizeProp(g_addButton);
            RemoveOriginalSizeProp(g_entry);
            RemoveOriginalSizeProp(g_taskFrame);

            if (g_taskItemRightClickMenu != NULL)
            {
                DestroyMenu(g_taskItemRightClickMenu);
                g_taskItemRightClickMenu = NULL;
            }
            
            DeleteTaskEntries();

            if (g_appState->currentTaskListFilePath != NULL)
            {
                free(g_appState->currentTaskListFilePath);
                g_appState->currentTaskListFilePath = NULL;
            }
            if (g_appState->currentTaskListFriendlyName != NULL)
            {
                free(g_appState->currentTaskListFriendlyName);
                g_appState->currentTaskListFriendlyName = NULL;
            }
            if (g_appState->customTextListFont != NULL)
            {
                DeleteObject(g_appState->customTextListFont);
                g_appState->customTextListFont = NULL;
            }

            return TRUE;
        }
        default: {
            return FALSE;
        }
    }
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    (void) hPrevInstance;
    (void) lpCmdLine;
    (void) nShowCmd;

    g_hInstance = hInstance;

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    IniFile iniFile;
    if (!ReadIniFile(L"C:\\Users\\tsasaki\\Desktop\\test.ini", &iniFile))
    {
        wchar_t msg[5];
        swprintf(msg, 5, L"%d", GetLastIniError());
        MessageBoxW(NULL, msg, L"INI TEST", MB_OK);
        return 0;
    }
    for (int i = 0; i < iniFile.numberOfSections; i ++)
    {
        IniSection iniSection = iniFile.iniSections[i];

        wchar_t aa1[1024];
        swprintf(aa1, 1024, L"Section Start: %ls", iniSection.sectionName);
        MessageBoxW(NULL, aa1, L"", MB_OK);

        for (int j = 0; j < iniSection.numberOfEntries; j ++)
        {
            IniEntry iniEntry = iniSection.iniEntries[j];
            wchar_t aa3[1024];
            swprintf(aa3, 1024, L"Key: \"%ls\", Value: \"%ls\"", iniEntry.key, iniEntry.value);
            MessageBoxW(NULL, aa3, L"", MB_OK);
        }

        wchar_t aa2[1024];
        swprintf(aa2, 1024, L"Section End: %ls", iniSection.sectionName);
        MessageBoxW(NULL, aa2, L"", MB_OK);
    }

    FreeIniFile(&iniFile);

    InitialiseTextListClass(hInstance);

    HACCEL mainDialogAccel = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(IDR_MAINACCELERATOR));
    if (mainDialogAccel == NULL)
    {
        return 1;
    }

    HWND mainDialog = CreateDialogW(
        hInstance,
        MAKEINTRESOURCEW(IDD_MAINDIALOG),
        NULL,
        MainDialogDlgProc
    );
    if (mainDialog == NULL)
    {
        return 1;
    }

    ShowWindow(mainDialog, SW_SHOW);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        if (!TranslateAcceleratorW(mainDialog, mainDialogAccel, &msg) &&
            !IsDialogMessageW(mainDialog, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}

