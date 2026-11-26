#include "app_state.h"

#include <windows.h>

#include "resource.h"
#include "util.h"

static AppState *g_appState = NULL;

static HINSTANCE g_hInstance;
static HWND g_mainHWND;

BOOL InitialiseAppState(HINSTANCE hInstance, HWND mainHWND)
{
    g_hInstance = hInstance;
    g_mainHWND = mainHWND;

    g_appState = malloc(sizeof(AppState));
    if (g_appState == NULL)
    {
        MessageBoxUsingResourceString(
            g_hInstance,
            g_mainHWND,
            MB_ICONERROR | MB_OK,
            IDS_MEMORY_ERROR_TITLE,
            IDS_FAILED_TO_ALLOCATE_MEMORY_MSG
        );

        return FALSE;
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

    return TRUE;
}

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

void DeleteTaskEntriesFromAppState(void)
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
}

BOOL DeleteTaskEntryFromAppState(int textListUniqueId)
{
    if (textListUniqueId < 0)
    {
        return FALSE;
    }

    int index = LookupTaskEntryIndexByTextListUniqueId(textListUniqueId);
    if (index < 0)
    {
        return FALSE;
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

    return TRUE;
}

void UpdateCurrentTaskListFriendlyName(wchar_t *newName)
{
    if (g_appState->currentTaskListFriendlyName != NULL)
    {
        free(g_appState->currentTaskListFriendlyName);
        g_appState->currentTaskListFriendlyName = NULL;
    }

    if (newName != NULL)
    {
        g_appState->currentTaskListFriendlyName = _wcsdup(newName);
    }
}

wchar_t *GetCurrentTaskListFriendlyName(void)
{
    return g_appState->currentTaskListFriendlyName;
}

BOOL HasUnsavedChanges(void)
{
    return g_appState->hasUnsavedChanges;
}

void SetUnsavedChangesStatus(BOOL status)
{
    g_appState->hasUnsavedChanges = status;
}

BOOL AddTaskEntryToAppState(int textListUniqueId, TaskEntry *newEntry)
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
                g_mainHWND,
                MB_ICONERROR | MB_OK,
                IDS_MEMORY_ERROR_TITLE,
                IDS_FAILED_TO_ALLOCATE_MEMORY_MSG
            );

            return FALSE;
        }

        g_appState->entries       = newEntries;
        g_appState->entryCapacity = newCapacity;
    }

    int index = g_appState->entryCount;
    g_appState->entries[index].textListUniqueId     = textListUniqueId;
    g_appState->entries[index].taskEntry.taskDetail = _wcsdup(newEntry->taskDetail);
    g_appState->entries[index].taskEntry.isDone     = newEntry->isDone;

    (g_appState->entryCount) ++;

    return TRUE;
}

TaskEntry *GetOrderedTaskEntryList(void)
{
    TaskEntry *taskEntries = calloc(g_appState->entryCount, sizeof(TaskEntry));
    if (taskEntries == NULL)
    {
        MessageBoxUsingResourceString(
            g_hInstance,
            g_mainHWND,
            MB_ICONERROR | MB_OK,
            IDS_MEMORY_ERROR_TITLE,
            IDS_FAILED_TO_ALLOCATE_MEMORY_MSG
        );

        return NULL;
    }

    for (int i = 0; i < g_appState->entryCount; i ++)
    {
        taskEntries[i] = g_appState->entries[i].taskEntry;
    }

    return taskEntries;
}

int GetAppStateEntryCount(void)
{
    return g_appState->entryCount;
}

void UpdateCurrentTaskListFilePath(wchar_t *newPath)
{
    if (g_appState->currentTaskListFilePath != NULL)
    {
        free(g_appState->currentTaskListFilePath);
        g_appState->currentTaskListFilePath = NULL;
    }

    if (newPath != NULL)
    {
        g_appState->currentTaskListFilePath = _wcsdup(newPath);
    }
}

wchar_t *GetCurrentTaskListFilePath(void)
{
    return g_appState->currentTaskListFilePath;
}

void UpdateTopMostEnabled(BOOL enabled)
{
    g_appState->topMostEnabled = enabled;
}

BOOL GetTopMostEnabled(void)
{
    return g_appState->topMostEnabled;
}

void UpdateCustomTextListFont(HFONT newFont)
{
    if (g_appState->customTextListFont != NULL)
    {
        DeleteObject(g_appState->customTextListFont);
        g_appState->customTextListFont = NULL;
    }
    g_appState->customTextListFont = newFont;
}

void SetTaskItemRightClickMenuTarget(int textListUniqueId)
{
    g_appState->taskItemRightClickMenuTarget = textListUniqueId;
}

int GetTaskItemRightClickMenuTarget(void)
{
    return g_appState->taskItemRightClickMenuTarget;
}

void FreeAppState(void)
{
    if (g_appState != NULL)
    {
        DeleteTaskEntriesFromAppState();

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

        free(g_appState);
        g_appState = NULL;
    }
}

