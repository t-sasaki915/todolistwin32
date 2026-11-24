#ifndef TASK_LIST_FILE_H
#define TASK_LIST_FILE_H

#include <windows.h>

typedef struct
{
    wchar_t *taskDetail;
    BOOL isDone;
} TaskEntry;

typedef struct
{
    wchar_t *friendlyName;

    TaskEntry *taskEntries;
    int taskItemCount;
} TaskListFile;

void InitialiseTaskListFile(HINSTANCE hInstance, HWND hwnd);

BOOL ReadTaskListFile(wchar_t *filePath, TaskListFile *taskListFile);

BOOL WriteTaskListFile(wchar_t *filePath, TaskListFile *taskListFile);

#endif

