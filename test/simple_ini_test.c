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

    printf("=============================================================\n\n");
}

void ExpectFailure(wchar_t *testName, wchar_t *input, IniError expectedError)
{
    printf("=============================================================\n");
    
    printf("Test Case %ls is expected to be failed.\n", testName);
    printf("Input:\n%ls\n\n", input);

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
        printf("Test Failure: ReadIniFileWchar succeed unexpectedly. (Expected Ini Error %d)\nIniFile structure:\n", expectedError);
        PrettyPrintIniFile(&iniFile);
        FreeIniFile(&iniFile);

        g_numberOfFailure ++;
    }

    printf("=============================================================\n\n");
}

int main(void)
{
    // =========================================================
    // Section syntax errors
    // =========================================================

    ExpectFailure(
        L"Empty section declaration",
        L"[]",
        INI_ERROR_EMPTY_SECTION_NAME
    );

    ExpectFailure(
        L"Section declaration with whitespace only",
        L"[ ]",
        INI_ERROR_EMPTY_SECTION_NAME
    );

    ExpectFailure(
        L"Section declaration with multiple whitespaces only",
        L"[     ]",
        INI_ERROR_EMPTY_SECTION_NAME
    );

    ExpectFailure(
        L"Section declaration without closing bracket",
        L"[section",
        INI_ERROR_CLOSE_SQUARE_BRACKET_NOT_FOUND
    );

    ExpectFailure(
        L"Section declaration with trailing whitespace but no closing bracket",
        L"[section   ",
        INI_ERROR_CLOSE_SQUARE_BRACKET_NOT_FOUND
    );

    ExpectFailure(
        L"Section declaration with whitespace but no name",
        L"[   ",
        INI_ERROR_UNEXPECTED_END_OF_LINE
    );

    ExpectFailure(
        L"Section declaration with only opening bracket",
        L"[",
        INI_ERROR_UNEXPECTED_END_OF_LINE
    );

    ExpectFailure(
        L"Unexpected character after section declaration",
        L"[section]a",
        INI_ERROR_EXPECTED_END_OF_LINE
    );

    ExpectFailure(
        L"Unexpected character after section declaration with whitespace",
        L"[section] abc",
        INI_ERROR_EXPECTED_END_OF_LINE
    );

    ExpectFailure(
        L"Unexpected second closing bracket",
        L"[section]]",
        INI_ERROR_EXPECTED_END_OF_LINE
    );

    ExpectFailure(
        L"Unexpected second opening bracket",
        L"[section][another]",
        INI_ERROR_EXPECTED_END_OF_LINE
    );

    ExpectFailure(
        L"Unexpected equal sign after section declaration",
        L"[section]=",
        INI_ERROR_EXPECTED_END_OF_LINE
    );

    // =========================================================
    // Unexpected closing square bracket
    // =========================================================

    ExpectFailure(
        L"Closing bracket without opening bracket",
        L"]",
        INI_ERROR_UNEXPECTED_CLOSE_SQUARE_BRACKET
    );

    ExpectFailure(
        L"Closing bracket at the beginning of a line",
        L"   ]",
        INI_ERROR_UNEXPECTED_CLOSE_SQUARE_BRACKET
    );

    ExpectFailure(
        L"Closing bracket followed by text",
        L"]unexpected",
        INI_ERROR_UNEXPECTED_CLOSE_SQUARE_BRACKET
    );

    ExpectFailure(
        L"Closing bracket inside a section",
        L"[section]\n]",
        INI_ERROR_UNEXPECTED_CLOSE_SQUARE_BRACKET
    );

    // =========================================================
    // Orphan entries
    // =========================================================

    ExpectFailure(
        L"Entry without section",
        L"key=value",
        INI_ERROR_ORPHAN_ENTRY
    );

    ExpectFailure(
        L"Entry without section with whitespace",
        L"   key=value",
        INI_ERROR_ORPHAN_ENTRY
    );

    ExpectFailure(
        L"Entry without section with empty value",
        L"key=",
        INI_ERROR_ORPHAN_ENTRY
    );

    ExpectFailure(
        L"Entry without section with quoted value",
        L"key=\"value\"",
        INI_ERROR_ORPHAN_ENTRY
    );

    // =========================================================
    // Empty key name
    // =========================================================

    ExpectFailure(
        L"Empty key name",
        L"[section]\n=value",
        INI_ERROR_EMPTY_KEY_NAME
    );

    ExpectFailure(
        L"Empty key name with whitespace before equal sign",
        L"[section]\n   =value",
        INI_ERROR_EMPTY_KEY_NAME
    );

    ExpectFailure(
        L"Empty key name with whitespace before and after equal sign",
        L"[section]\n   =   value",
        INI_ERROR_EMPTY_KEY_NAME
    );

    // =========================================================
    // Missing equal sign
    // =========================================================

    ExpectFailure(
        L"Missing equal sign",
        L"[section]\nkey",
        INI_ERROR_EQUAL_SIGN_NOT_FOUND
    );

    ExpectFailure(
        L"Missing equal sign with trailing whitespace",
        L"[section]\nkey   ",
        INI_ERROR_EQUAL_SIGN_NOT_FOUND
    );

    ExpectFailure(
        L"Missing equal sign with spaces between key characters",
        L"[section]\nkey value",
        INI_ERROR_EQUAL_SIGN_NOT_FOUND
    );

    ExpectFailure(
        L"Missing equal sign with colon",
        L"[section]\nkey:value",
        INI_ERROR_EQUAL_SIGN_NOT_FOUND
    );

    ExpectFailure(
        L"Missing equal sign with colon and whitespace",
        L"[section]\nkey : value",
        INI_ERROR_EQUAL_SIGN_NOT_FOUND
    );

    ExpectFailure(
        L"Missing equal sign with tab",
        L"[section]\nkey\tvalue",
        INI_ERROR_EQUAL_SIGN_NOT_FOUND
    );

    // =========================================================
    // Unclosed quotation marks
    // =========================================================

    ExpectFailure(
        L"Unclosed quotation mark",
        L"[section]\nkey=\"value",
        INI_ERROR_QUOTE_NOT_CLOSED
    );

    ExpectFailure(
        L"Empty unclosed quotation mark",
        L"[section]\nkey=\"",
        INI_ERROR_QUOTE_NOT_CLOSED
    );

    ExpectFailure(
        L"Unclosed quotation mark with whitespace",
        L"[section]\nkey=\"value   ",
        INI_ERROR_QUOTE_NOT_CLOSED
    );

    ExpectFailure(
        L"Unclosed quotation mark with comment character",
        L"[section]\nkey=\"value ; comment",
        INI_ERROR_QUOTE_NOT_CLOSED
    );

    ExpectFailure(
        L"Unclosed quotation mark with hash comment character",
        L"[section]\nkey=\"value # comment",
        INI_ERROR_QUOTE_NOT_CLOSED
    );

    // =========================================================
    // Unexpected characters after quoted values
    // =========================================================

    ExpectFailure(
        L"Unexpected character after quoted value",
        L"[section]\nkey=\"value\"x",
        INI_ERROR_EXPECTED_END_OF_LINE
    );

    ExpectFailure(
        L"Unexpected text after quoted value",
        L"[section]\nkey=\"value\" extra",
        INI_ERROR_EXPECTED_END_OF_LINE
    );

    ExpectFailure(
        L"Unexpected equal sign after quoted value",
        L"[section]\nkey=\"value\"=another",
        INI_ERROR_EXPECTED_END_OF_LINE
    );

    ExpectFailure(
        L"Unexpected second quoted value",
        L"[section]\nkey=\"value\"\"another\"",
        INI_ERROR_EXPECTED_END_OF_LINE
    );

    ExpectFailure(
        L"Unexpected opening bracket after quoted value",
        L"[section]\nkey=\"value\"[",
        INI_ERROR_EXPECTED_END_OF_LINE
    );

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

