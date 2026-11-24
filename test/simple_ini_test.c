#include "../simple_ini.h"

#include <windows.h>
#include <stdio.h>

static int g_numberOfSuccess = 0;
static int g_numberOfFailure = 0;

void PrettyPrintIniFile(IniFile *iniFile)
{
    printf("Begin: Ini File (%d sections)\n", iniFile->numberOfSections);

    for (int i = 0; i < iniFile->numberOfSections; i ++)
    {
        IniSection iniSection = iniFile->iniSections[i];
        printf("    Begin: Ini Section \"%ls\" (%d entries)\n", iniSection.sectionName, iniSection.numberOfEntries);

        for (int j = 0; j < iniSection.numberOfEntries; j ++)
        {
            IniEntry iniEntry = iniSection.iniEntries[j];

            printf("        Key: \"%ls\", Value: \"%ls\"\n", iniEntry.key, iniEntry.value);
        }

        printf("    End: Ini Section \"%ls\"\n", iniSection.sectionName);
    }

    printf("End: Ini File\n");
}

BOOL AreIniFilesEquivalent(IniFile *iniFileA, IniFile *iniFileB)
{
    if (iniFileA->numberOfSections != iniFileB->numberOfSections)
    {
        return FALSE;
    }

    for (int i = 0; i < iniFileA->numberOfSections; i ++)
    {
        IniSection iniSectionA = iniFileA->iniSections[i];
        IniSection iniSectionB = iniFileB->iniSections[i];

        if (iniSectionA.numberOfEntries != iniSectionB.numberOfEntries)
        {
            return FALSE;
        }
        if (wcscmp(iniSectionA.sectionName, iniSectionB.sectionName) != 0)
        {
            return FALSE;
        }

        for (int j = 0; j < iniSectionA.numberOfEntries; j ++)
        {
            IniEntry iniEntryA = iniSectionA.iniEntries[j];
            IniEntry iniEntryB = iniSectionB.iniEntries[j];

            if (wcscmp(iniEntryA.key, iniEntryB.key) != 0)
            {
                return FALSE;
            }
            if (wcscmp(iniEntryA.value, iniEntryB.value) != 0)
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

void ExpectSuccess(wchar_t *testName, wchar_t *input, IniFile *expectedStruct)
{
    printf("=============================================================\n");
    
    printf("Test Case %ls is expected to be succeeded.\n", testName);
    printf("Input:\n%ls\n", input);

    IniFile iniFile = {0};
    if (ReadIniFileWchar(input, &iniFile))
    {
        if (AreIniFilesEquivalent(expectedStruct, &iniFile))
        {
            printf("Test Success: ReadIniFileWchar succeeded with expected structure:\n");
            PrettyPrintIniFile(expectedStruct);

            g_numberOfSuccess ++;
        }
        else
        {
            printf("Test Failure: ReadIniFileWchar succeeded with unexpected structure:\n");
            PrettyPrintIniFile(&iniFile);
            printf("Expected structure:\n");
            PrettyPrintIniFile(expectedStruct);

            g_numberOfFailure ++;
        }
    }
    else
    {
        printf("Test Failure: ReadIniFileWchar failed unexpectedly with IniError %d. Expected:\n", GetLastIniError());
        PrettyPrintIniFile(expectedStruct);

        g_numberOfFailure ++;
    }

    printf("=============================================================\n");
}

void ExpectFailure(wchar_t *testName, wchar_t *input, IniError expectedError)
{
    printf("=============================================================\n");
    
    printf("Test Case %ls is expected to be failed.\n", testName);
    printf("Input:\n%ls\n", input);

    IniFile iniFile = {0};
    if (!ReadIniFileWchar(input, &iniFile))
    {
        IniError actualError = GetLastIniError();
        if (actualError == expectedError)
        {
            printf("Test Success: ReadIniFileWchar failed with expected IniError %d.\n", expectedError);

            g_numberOfSuccess ++;
        }
        else
        {
            printf("Test Failure: ReadIniFileWchar failed with unexpected IniError %d. (Expected %d)\n", actualError, expectedError);

            g_numberOfFailure ++;
        }
    }
    else
    {
        printf("Test Failure: ReadIniFileWchar succeed unexpectedly.\nIniFile structure:\n");
        PrettyPrintIniFile(&iniFile);
        FreeIniFile(&iniFile);

        g_numberOfFailure ++;
    }

    printf("=============================================================\n");
}

int main(void)
{

    printf("\nTest Result: %d Succeeded, %d Failed.\n", g_numberOfSuccess, g_numberOfFailure);

    if (g_numberOfFailure == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

