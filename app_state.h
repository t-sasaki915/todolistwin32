#ifndef APP_STATE_H
#define APP_STATE_H

#include <windows.h>

#include "task_list_file.h"

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
} AppState;

BOOL InitialiseAppState(HINSTANCE hInstance, HWND mainHWND);

TaskEntry *LookupTaskEntryByTextListUniqueId(int textListUniqueId);

void DeleteTaskEntriesFromAppState(void);

BOOL DeleteTaskEntryFromAppState(int textListUniqueId);

void UpdateCurrentTaskListFriendlyName(wchar_t *newName);

wchar_t *GetCurrentTaskListFriendlyName(void);

BOOL HasUnsavedChanges(void);

void SetUnsavedChangesStatus(BOOL status);

BOOL AddTaskEntryToAppState(int textListUniqueId, TaskEntry *newEntry);

TaskEntry *GetOrderedTaskEntryList(void);

int GetAppStateEntryCount(void);

void UpdateCurrentTaskListFilePath(wchar_t *newPath);

wchar_t *GetCurrentTaskListFilePath(void);

void UpdateTopMostEnabled(BOOL enabled);

BOOL GetTopMostEnabled(void);

void UpdateCustomTextListFont(HFONT newFont);

void SetTaskItemRightClickMenuTarget(int textListUniqueId);

int GetTaskItemRightClickMenuTarget(void);

void FreeAppState(void);

#endif

