#include "task_list_file.h"

#include <windows.h>
#include <wchar.h>
#include <stdint.h>

#include "util.h"
#include "resource.h"

typedef enum
{
    CREATE_FILE_MODE_OPEN           = 0,
    CREATE_FILE_MODE_READ_OVERWRITE = 1
} CreateFileMode;

static const int  g_taskListFileSignatureLength = 12;
static const char g_taskListFileSignature[]     = "TASKLISTFILE";

static HINSTANCE g_hInstance = NULL;
static HWND g_mainHWND = NULL;

void InitialiseTaskListFile(HINSTANCE hInstance, HWND hwnd)
{
    g_hInstance = hInstance;
    g_mainHWND = hwnd;
}

BOOL CreateFileAndValidate(wchar_t *filePath, HANDLE *handlePtr, CreateFileMode createFileMode)
{
    DWORD access = 0;
    DWORD mode = 0;
    switch (createFileMode)
    {
        case CREATE_FILE_MODE_OPEN: {
            access = GENERIC_READ;
            mode   = OPEN_EXISTING;
            break;
        }
        case CREATE_FILE_MODE_READ_OVERWRITE: {
            access = GENERIC_WRITE;
            mode   = CREATE_ALWAYS;
            break;
        }
    }

    HANDLE fileHandle = CreateFileW(
        filePath,
        access,
        0,
        NULL,
        mode,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        MessageBoxUsingResourceString(
            g_hInstance,
            g_mainHWND,
            MB_ICONERROR | MB_OK,
            IDS_OPEN_FILE_ERROR_TITLE,
            IDS_FAILED_TO_OPEN_FILE_MSG,
            filePath,
            GetLastError()
        );

        return FALSE;
    }

    *handlePtr = fileHandle;

    return TRUE;
}

BOOL ReadFileAndValidate(wchar_t *name, HANDLE fileHandle, LPVOID buffer, DWORD size)
{
    DWORD bytesRead;
    if (!ReadFile(fileHandle, buffer, size, &bytesRead, NULL))
    {
        MessageBoxUsingResourceString(
            g_hInstance,
            g_mainHWND,
            MB_ICONERROR | MB_OK,
            IDS_READ_FILE_ERROR_TITLE,
            IDS_READFILE_RETURNED_FALSE_MSG,
            name,
            GetLastError()
        );

        return FALSE;
    }

    if (bytesRead != size)
    {
        MessageBoxUsingResourceString(
            g_hInstance,
            g_mainHWND,
            MB_ICONERROR | MB_OK,
            IDS_READ_FILE_ERROR_TITLE,
            IDS_BYTE_READ_MISMATCH_MSG,
            name,
            size,
            bytesRead
        );

        return FALSE;
    }

    return TRUE;
}

BOOL WriteFileAndValidate(wchar_t *name, HANDLE fileHandle, LPVOID buffer, DWORD size)
{
    DWORD bytesWritten;
    if (!WriteFile(fileHandle, buffer, size, &bytesWritten, NULL))
    {
        MessageBoxUsingResourceString(
            g_hInstance,
            g_mainHWND,
            MB_ICONERROR | MB_OK,
            IDS_WRITE_FILE_ERROR_TITLE,
            IDS_WRITEFILE_RETURNED_FALSE_MSG,
            name,
            GetLastError()
        );

        return FALSE;
    }

    if (bytesWritten != size)
    {
        MessageBoxUsingResourceString(
            g_hInstance,
            g_mainHWND,
            MB_ICONERROR | MB_OK,
            IDS_WRITE_FILE_ERROR_TITLE,
            IDS_BYTE_WRITE_MISMATCH_MSG,
            name,
            size,
            bytesWritten
        );

        return FALSE;
    }

    return TRUE;
}

wchar_t *GetWideSignature(const char *signature)
{
    wchar_t wideSignature[32];

    MultiByteToWideChar(CP_ACP, 0, signature, -1, wideSignature, 32);

    return _wcsdup(wideSignature);
}

BOOL ValidateTaskListFileSignature(char *signature)
{
    for (int i = 0; i < g_taskListFileSignatureLength; i ++)
    {
        if (signature[i] != g_taskListFileSignature[i])
        {
            wchar_t *expectedWideSignature = GetWideSignature(g_taskListFileSignature);
            wchar_t *actualWideSignature   = GetWideSignature(signature);

            MessageBoxUsingResourceString(
                g_hInstance,
                g_mainHWND,
                MB_ICONERROR | MB_OK,
                IDS_READ_FILE_ERROR_TITLE,
                IDS_SIGNATURE_MISMATCH_MSG,
                expectedWideSignature,
                actualWideSignature
            );

            free(expectedWideSignature);
            free(actualWideSignature);

            goto Failed;
        }
    }

    return TRUE;

Failed:
    return FALSE;
}

void MemoryAllocationError(void)
{
    MessageBoxUsingResourceString(
        g_hInstance,
        g_mainHWND,
        MB_ICONERROR | MB_OK,
        IDS_MEMORY_ERROR_TITLE,
        IDS_FAILED_TO_ALLOCATE_MEMORY_MSG
    );
}

BOOL ReadTaskListFile(wchar_t *filePath, TaskListFile *taskListFile)
{
    HANDLE fileHandle;
    if (!CreateFileAndValidate(filePath, &fileHandle, CREATE_FILE_MODE_OPEN))
    {
        return FALSE;
    }

    TaskEntry *taskEntries = NULL;
    wchar_t *friendlyName = NULL;

    char signatureBuffer[g_taskListFileSignatureLength];
    BOOL signatureReadResult = ReadFileAndValidate(
        L"Signature",
        fileHandle,
        signatureBuffer,
        sizeof(char) * g_taskListFileSignatureLength
    );
    if (!signatureReadResult)
    {
        goto Failed;
    }
    if (!ValidateTaskListFileSignature(signatureBuffer))
    {
        goto Failed;
    }

    int friendlyNameLength;
    BOOL friendlyNameLengthReadResult = ReadFileAndValidate(
        L"FriendlyNameLength",
        fileHandle,
        &friendlyNameLength,
        sizeof(int)
    );
    if (!friendlyNameLengthReadResult)
    {
        goto Failed;
    }
    if (friendlyNameLength != 0)
    {
        friendlyName = calloc(friendlyNameLength + 1, sizeof(wchar_t));
        BOOL friendlyNameReadResult = ReadFileAndValidate(
            L"FriendlyName",
            fileHandle,
            friendlyName,
            sizeof(wchar_t) * friendlyNameLength
        );
        if (!friendlyNameReadResult)
        {
            goto Failed;
        }
        friendlyName[friendlyNameLength] = L'\0';
        taskListFile->friendlyName = friendlyName;
    }
    else
    {
        taskListFile->friendlyName = NULL;
    }

    int taskItemCount;
    BOOL taskItemCountReadResult = ReadFileAndValidate(
        L"TaskItemCount",
        fileHandle,
        &taskItemCount,
        sizeof(int)
    );
    if (!taskItemCountReadResult)
    {
        goto Failed;
    }
    taskListFile->taskItemCount = taskItemCount;

    taskEntries = (TaskEntry *) calloc(taskItemCount, sizeof(TaskEntry));
    if (taskEntries == NULL)
    {
        MemoryAllocationError();

        goto Failed;
    }

    for (int i = 0; i < taskItemCount; i ++)
    {
        uint8_t taskStatus = 0;
        BOOL taskStatusReadResult = ReadFileAndValidate(
            L"TaskStatus",
            fileHandle,
            &taskStatus,
            sizeof(uint8_t)
        );
        if (!taskStatusReadResult)
        {
            goto Failed;
        }

        int taskDetailLength;
        BOOL taskDetailLengthReadResult = ReadFileAndValidate(
            L"TaskDetailLength",
            fileHandle,
            &taskDetailLength,
            sizeof(int)
        );
        if (!taskDetailLengthReadResult)
        {
            goto Failed;
        }

        wchar_t *taskDetail = calloc(taskDetailLength + 1, sizeof(wchar_t));
        if (taskDetail == NULL)
        {
            MemoryAllocationError();

            goto Failed;
        }
        BOOL taskNameReadResult = ReadFileAndValidate(
            L"TaskName",
            fileHandle,
            taskDetail,
            sizeof(wchar_t) * taskDetailLength
        );
        if (!taskNameReadResult)
        {
            free(taskDetail);

            goto Failed;
        }

        taskDetail[taskDetailLength] = L'\0';

        taskEntries[i].taskDetail = taskDetail;
        taskEntries[i].isDone     = taskStatus == 1 ? TRUE : FALSE;
    }
    taskListFile->taskEntries = taskEntries;

    CloseHandle(fileHandle);
    return TRUE;

Failed:
    if (taskEntries != NULL)
    {
        for (int i = 0; i < taskItemCount; i ++)
        {
            if (taskEntries[i].taskDetail != NULL)
            {
                free(taskEntries[i].taskDetail);
                taskEntries[i].taskDetail = NULL;
            }
        }

        free(taskEntries);
        taskEntries = NULL;
    }
    if (friendlyName != NULL)
    {
        free(friendlyName);
        friendlyName = NULL;
    }

    CloseHandle(fileHandle);
    return FALSE;
    return TRUE;
}

BOOL WriteTaskListFile(wchar_t *filePath, TaskListFile *taskListFile)
{
    HANDLE fileHandle;
    if (!CreateFileAndValidate(filePath, &fileHandle, CREATE_FILE_MODE_READ_OVERWRITE))
    {
        return FALSE;
    }

    BOOL signatureWriteResult = WriteFileAndValidate(
        L"Signature",
        fileHandle,
        (void *) g_taskListFileSignature,
        sizeof(char) * g_taskListFileSignatureLength
    );
    if (!signatureWriteResult)
    {
        goto Failed;
    }

    int friendlyNameLength = taskListFile->friendlyName != NULL ? wcslen(taskListFile->friendlyName)
                                                                : 0;
    BOOL friendlyNameLengthWriteResult = WriteFileAndValidate(
        L"FriendlyNameLength",
        fileHandle,
        &friendlyNameLength,
        sizeof(int)
    );
    if (!friendlyNameLengthWriteResult)
    {
        goto Failed;
    }

    if (friendlyNameLength != 0)
    {
        BOOL friendlyNameWriteResult = WriteFileAndValidate(
            L"FriendlyName",
            fileHandle,
            taskListFile->friendlyName,
            sizeof(wchar_t) * friendlyNameLength
        );
        if (!friendlyNameWriteResult)
        {
            goto Failed;
        }
    }

    BOOL taskItemCountWriteResult = WriteFileAndValidate(
        L"TaskItemCount",
        fileHandle,
        &(taskListFile->taskItemCount),
        sizeof(int)
    );
    if (!taskItemCountWriteResult)
    {
        goto Failed;
    }

    if (taskListFile->taskEntries == NULL || taskListFile->taskItemCount == 0)
    {
        goto Success;
    }

    for (int i = 0; i < taskListFile->taskItemCount; i ++)
    {
        TaskEntry taskEntry = taskListFile->taskEntries[i];

        uint8_t taskStatus = taskEntry.isDone ? 1 : 0;
        BOOL taskStatusWriteResult = WriteFileAndValidate(
            L"TaskStatus",
            fileHandle,
            &taskStatus,
            sizeof(uint8_t)
        );
        if (!taskStatusWriteResult)
        {
            goto Failed;
        }

        int taskDetailLength = wcslen(taskEntry.taskDetail);
        BOOL taskDetailLengthWriteResult = WriteFileAndValidate(
            L"TaskDetailLength",
            fileHandle,
            &taskDetailLength,
            sizeof(int)
        );
        if (!taskDetailLengthWriteResult)
        {
            goto Failed;
        }

        BOOL taskDetailWriteResult = WriteFileAndValidate(
            L"TaskDetail",
            fileHandle,
            taskEntry.taskDetail,
            sizeof(wchar_t) * taskDetailLength
        );
        if (!taskDetailWriteResult)
        {
            goto Failed;
        }
    }

Success:
    CloseHandle(fileHandle);
    return TRUE;

Failed:
    CloseHandle(fileHandle);
    return FALSE;
}

