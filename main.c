#include <windows.h>
#include <wchar.h>

#include "app_state.h"
#include "text_list.h"
#include "task_list_file.h"
#include "layout.h"
#include "simple_ini.h"
#include "util.h"
#include "resource.h"

static HINSTANCE g_hInstance;

static HWND  g_mainDialog             = NULL;
static HWND  g_addButton              = NULL;
static HWND  g_entry                  = NULL;
static HWND  g_taskFrame              = NULL;
static HMENU g_mainDialogMenu         = NULL;
static HMENU g_taskItemRightClickMenu = NULL;

void DeleteTaskEntries(void)
{
    DeleteTaskEntriesFromAppState();
    SendMessageW(g_taskFrame, WM_TEXTLIST_CLEAR, 0, 0);
}

void DeleteTaskEntry(int textListUniqueId)
{
    if (!DeleteTaskEntryFromAppState(textListUniqueId))
    {
        return;
    }

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
    if (GetCurrentTaskListFriendlyName() != NULL)
    {
        friendlyName = _wcsdup(GetCurrentTaskListFriendlyName());
        if (friendlyName == NULL)
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
    }
    else
    {
        if (LoadStringAndValidate(g_hInstance, IDS_UNTITLED, &friendlyName) < 0)
        {
            return;
        }
    }

    UINT titleFormatID = HasUnsavedChanges() ? IDS_MAINDIALOG_TITLE_EDITED
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
    SetUnsavedChangesStatus(TRUE);
    UpdateMainDialogTitle();
}

void ThereAreNoUnsavedChanges(void)
{
    SetUnsavedChangesStatus(FALSE);
    UpdateMainDialogTitle();
}

void AddTaskEntry(TaskEntry *entry)
{
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

    if (!AddTaskEntryToAppState(textListUniqueId, entry))
    {
        SendMessageW(g_taskFrame, WM_TEXTLIST_REMOVE_ENTRY, 0, (LPARAM) textListUniqueId);

        return;
    }

    SetWindowTextW(g_entry, L"");

    ThereAreUnsavedChanges();
}

void GetEntryContentAndAddTask(void)
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
    AddTaskEntry(&entry);
}

void SaveTaskListFile(wchar_t *filePath)
{
    TaskEntry *taskEntries = GetOrderedTaskEntryList();
    if (taskEntries == NULL)
    {
        return;
    }

    TaskListFile taskListFile  = {0};
    taskListFile.friendlyName  = GetCurrentTaskListFriendlyName();
    taskListFile.taskEntries   = taskEntries;
    taskListFile.taskItemCount = GetAppStateEntryCount();

    if (!WriteTaskListFile(filePath, &taskListFile))
    {
        return;
    }

    free(taskEntries);

    UpdateCurrentTaskListFilePath(filePath);

    ThereAreNoUnsavedChanges();
}

BOOL CreateTodoListFileOFN(OPENFILENAMEW *ofn, DWORD flags)
{
    wchar_t *szFile = calloc(1024, sizeof(wchar_t));
    if (szFile == NULL)
    {
        MessageBoxUsingResourceString(
            g_hInstance,
            g_mainDialog,
            MB_ICONERROR | MB_OK,
            IDS_MEMORY_ERROR_TITLE,
            IDS_FAILED_TO_ALLOCATE_MEMORY_MSG
        );

        return FALSE;
    }

    ofn->lStructSize     = sizeof(OPENFILENAMEW);
    ofn->hwndOwner       = g_mainDialog;
    ofn->lpstrFile       = szFile;
    ofn->lpstrFile[0]    = L'\0';
    ofn->nMaxFile        = sizeof(szFile);
    ofn->lpstrFilter     = L"Todo List Files (*.tdl)\0*.tdl\0\0";
    ofn->lpstrDefExt     = L"tdl";
    ofn->nFilterIndex    = 1;
    ofn->lpstrFileTitle  = NULL;
    ofn->nMaxFileTitle   = 0;
    ofn->lpstrInitialDir = NULL;
    ofn->Flags           = flags;

    return TRUE;
}

BOOL SaveTaskListAs(void)
{
    OPENFILENAMEW ofn = {0};
    if (!CreateTodoListFileOFN(&ofn, OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT))
    {
        return FALSE;
    }

    if (!GetSaveFileNameW(&ofn))
    {
        free(ofn.lpstrFile);

        return FALSE;
    }

    SaveTaskListFile(ofn.lpstrFile);

    free(ofn.lpstrFile);

    return TRUE;
}

BOOL SaveTaskList(void)
{
    if (GetCurrentTaskListFilePath() != NULL)
    {
        SaveTaskListFile(GetCurrentTaskListFilePath());

        return TRUE;
    }
    else
    {
        return SaveTaskListAs();
    }
}

BOOL ConfirmSaveChangesBeforeContinuing(void)
{
    if (HasUnsavedChanges())
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

    UpdateCurrentTaskListFilePath(NULL);
    UpdateCurrentTaskListFriendlyName(NULL);

    ThereAreNoUnsavedChanges();
}

void OpenTaskListFile(void)
{
    OPENFILENAMEW ofn = {0};
    TaskListFile taskListFile = {0};

    if (!CreateTodoListFileOFN(&ofn, OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST))
    {
        return;
    }

    if (!GetOpenFileNameW(&ofn))
    {
        goto CleanUp;
    }

    if (!ReadTaskListFile(ofn.lpstrFile, &taskListFile))
    {
        goto CleanUp;
    }

    if (!ConfirmSaveChangesBeforeContinuing())
    {
        goto CleanUp;
    }

    DeleteTaskEntries();

    for (int i = 0; i < taskListFile.taskItemCount; i ++)
    {
        AddTaskEntry(&taskListFile.taskEntries[i]);
    }

    UpdateCurrentTaskListFilePath(ofn.lpstrFile);
    UpdateCurrentTaskListFriendlyName(taskListFile.friendlyName);

    ThereAreNoUnsavedChanges();

CleanUp:
    if (ofn.lpstrFile != NULL)
    {
        free(ofn.lpstrFile);
        ofn.lpstrFile = NULL;
    }
    if (taskListFile.taskEntries != NULL)
    {
        for (int i = 0; i < taskListFile.taskItemCount; i ++)
        {
            if (taskListFile.taskEntries[i].taskDetail != NULL)
            {
                free(taskListFile.taskEntries[i].taskDetail);
                taskListFile.taskEntries[i].taskDetail = NULL;
            }
        }
        free(taskListFile.taskEntries);
        taskListFile.taskEntries = NULL;
    }
    if (taskListFile.friendlyName != NULL)
    {
        free(taskListFile.friendlyName);
        taskListFile.friendlyName = NULL;
    }
}

void ToggleAlwaysOnTop(void)
{
    UpdateTopMostEnabled(!GetTopMostEnabled());

    MENUITEMINFOW itemInfo = {0};
    itemInfo.cbSize        = sizeof(MENUITEMINFOW);
    itemInfo.fMask         = MIIM_STATE;
    GetMenuItemInfoW(g_mainDialogMenu, ID_VIEW_ALWAYSONTOP, FALSE, &itemInfo);

    itemInfo.fState = GetTopMostEnabled() ? MFS_CHECKED : MFS_UNCHECKED;

    SetMenuItemInfoW(g_mainDialogMenu, ID_VIEW_ALWAYSONTOP, FALSE, &itemInfo);

    if (GetTopMostEnabled())
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
    cf.Flags       = CF_SCREENFONTS | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL;

    if (!ChooseFontW(&cf))
    {
        return;
    }

    HFONT newFont = CreateFontIndirectW(cf.lpLogFont);
    if (newFont == NULL)
    {
        return;
    }

    UpdateCustomTextListFont(newFont);

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
        if (GetCurrentTaskListFriendlyName() != NULL)
        {
            SetDlgItemTextW(hwnd, IDC_NEWTASKLISTNAME, GetCurrentTaskListFriendlyName());
        }

        SetFocus(GetDlgItem(hwnd, IDC_NEWTASKLISTNAME));

        return FALSE;
    }
    else if (DIALOG_OK_COND(wMsg, wParam))
    {
        wchar_t newName[1024];
        GetDlgItemTextW(hwnd, IDC_NEWTASKLISTNAME, newName, 1024);

        if (wcslen(newName) != 0 && !IsWhiteSpaceOnly(newName))
        {
            UpdateCurrentTaskListFriendlyName(newName);
        }
        else
        {
            UpdateCurrentTaskListFriendlyName(NULL);
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
                goto Failed;
            }

            if (!InitialiseAppState(g_hInstance, g_mainDialog))
            {
                goto Failed;
            }

            IniFile iniFile;
            if (!ReadIniFile(L"C:\\Users\\tsasaki\\Desktop\\test.ini", &iniFile))
            {
                MessageBoxUsingResourceString(
                    g_hInstance,
                    g_mainDialog,
                    MB_ICONERROR | MB_OK,
                    IDS_CONFIG_ERROR_TITLE,
                    IDS_FAILED_TO_READ_CONFIG_FILE_MSG,
                    GetLastIniError()
                );
        
                goto Failed;
            }
        
            BOOL topMostEnabled;
            BOOL defaultTopMostEnabled = TRUE;
            if (!GetBoolFromIni(&iniFile, L"General", L"AlwaysOnTop", &topMostEnabled))
            {
                if (!RecoverFromIniErrorUsingDefaultValue((void *) &topMostEnabled, (void *) &defaultTopMostEnabled, TRUE))
                {
                    MessageBoxUsingResourceString(
                        g_hInstance,
                        g_mainDialog,
                        MB_ICONERROR | MB_OK,
                        IDS_CONFIG_ERROR_TITLE,
                        IDS_INI_ERROR_MSG,
                        L"General.AlwaysOnTop",
                        GetLastIniError()
                    );
        
                    goto Failed;
                }
            }
        
            wchar_t *fontName;
            wchar_t *defaultFontName = L"Tahoma";
            if (!GetStringFromIni(&iniFile, L"Font", L"FontName", &fontName))
            {
                if (!RecoverFromIniErrorUsingDefaultValue((void *) &fontName, (void *) defaultFontName, TRUE))
                {
                    MessageBoxUsingResourceString(
                        g_hInstance,
                        g_mainDialog,
                        MB_ICONERROR | MB_OK,
                        IDS_CONFIG_ERROR_TITLE,
                        IDS_INI_ERROR_MSG,
                        L"Font.FontName",
                        GetLastIniError()
                    );
        
                    goto Failed;
                }
            }
        
            LONG fontSize;
            LONG defaultFontSize = 8;
            if (!GetLongFromIni(&iniFile, L"Font", L"FontSize", &fontSize))
            {
                if (!RecoverFromIniErrorUsingDefaultValue((void *) &fontSize, (void *) &defaultFontSize, TRUE))
                {
                    MessageBoxUsingResourceString(
                        g_hInstance,
                        g_mainDialog,
                        MB_ICONERROR | MB_OK,
                        IDS_CONFIG_ERROR_TITLE,
                        IDS_INI_ERROR_MSG,
                        L"Font.FontSize",
                        GetLastIniError()
                    );

                    goto Failed;
                }
            }
      
            UpdateTopMostEnabled(!topMostEnabled);
            ToggleAlwaysOnTop();
        
            FreeIniFile(&iniFile);

            NewFile();

            SetFocus(g_entry);

            return FALSE;

Failed:
            if (g_taskItemRightClickMenu != NULL)
            {
                DestroyMenu(g_taskItemRightClickMenu);
                g_taskItemRightClickMenu = NULL;
            }

            RemoveOriginalSizeProp(g_mainDialog);
            RemoveOriginalSizeProp(g_addButton);
            RemoveOriginalSizeProp(g_entry);
            RemoveOriginalSizeProp(g_taskFrame);

            EndDialog(g_mainDialog, 1);
            PostQuitMessage(1);

            return TRUE;
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
                    GetEntryContentAndAddTask();

                    return TRUE;
                }
                case IDC_ADDBUTTON: {
                    GetEntryContentAndAddTask();

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
                    ToggleTaskStatus(GetTaskItemRightClickMenuTarget());

                    return TRUE;
                }
                case ID_TASKITEMRIGHTCLICKMENU_EDIT: {
                    DialogBoxParamW(
                        g_hInstance,
                        MAKEINTRESOURCEW(IDD_TASKDETAILEDITDIALOG),
                        g_mainDialog,
                        TaskDetailEditDialogDlgProc,
                        (LPARAM) GetTaskItemRightClickMenuTarget()
                    );

                    return TRUE;
                }
                case ID_TASKITEMRIGHTCLICKMENU_DELETE: {
                    AskAndDeleteTaskEntry(GetTaskItemRightClickMenuTarget());

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

            SetTaskItemRightClickMenuTarget(textListUniqueId);

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
           
            FreeAppState();

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

