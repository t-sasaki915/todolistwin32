#include "../simple_ini.h"

#include <windows.h>
#include <stdio.h>

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define INI_ENTRY(key, value) { (wchar_t *)(key), (wchar_t *)(value) }
#define INI_SECTION(name, ...) \
    { \
        (wchar_t *)(name), \
        (IniEntry[]){ __VA_ARGS__ }, \
        ARRAY_COUNT(((IniEntry[]){ __VA_ARGS__ })) \
    }
#define INI_FILE(...) \
    &(IniFile){ \
        (IniSection[]){ __VA_ARGS__ }, \
        ARRAY_COUNT(((IniSection[]){ __VA_ARGS__ })) \
    }

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

    // =========================================================
    // Duplicate sections
    // =========================================================
    
    ExpectFailure(
        L"Duplicate section",
        L"[Section]\r\n"
        L"[Section]",
        INI_ERROR_SECTION_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate section with entries",
        L"[Section]\r\n"
        L"Key1=Value1\r\n"
        L"[Section]\r\n"
        L"Key2=Value2",
        INI_ERROR_SECTION_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate section with empty first section",
        L"[Section1]\r\n"
        L"[Section2]\r\n"
        L"[Section1]",
        INI_ERROR_SECTION_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate section with empty second section",
        L"[Section1]\r\n"
        L"[Section2]\r\n"
        L"[Section2]",
        INI_ERROR_SECTION_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate section separated by other sections",
        L"[Section1]\r\n"
        L"[Section2]\r\n"
        L"[Section3]\r\n"
        L"[Section1]",
        INI_ERROR_SECTION_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate section with different entries",
        L"[Section]\r\n"
        L"Key1=Value1\r\n"
        L"[AnotherSection]\r\n"
        L"Key2=Value2\r\n"
        L"[Section]\r\n"
        L"Key3=Value3",
        INI_ERROR_SECTION_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate section with comments",
        L"[Section]\r\n"
        L"; comment\r\n"
        L"[Section]",
        INI_ERROR_SECTION_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate section with blank lines",
        L"[Section]\r\n"
        L"\r\n"
        L"\r\n"
        L"[Section]",
        INI_ERROR_SECTION_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate section with whitespace",
        L"[Section]\r\n"
        L"[  Section  ]",
        INI_ERROR_SECTION_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate section with leading whitespace",
        L"[Section]\r\n"
        L"    [Section]",
        INI_ERROR_SECTION_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate section with trailing comment",
        L"[Section]\r\n"
        L"[Section] ; comment",
        INI_ERROR_SECTION_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate section with hash comment",
        L"[Section]\r\n"
        L"[Section] # comment",
        INI_ERROR_SECTION_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Three identical sections",
        L"[Section]\r\n"
        L"[Section]\r\n"
        L"[Section]",
        INI_ERROR_SECTION_ALREADY_EXIST
    );

    // =========================================================
    // Duplicate keys
    // =========================================================
    
    ExpectFailure(
        L"Duplicate key",
        L"[Section]\r\n"
        L"Key=Value1\r\n"
        L"Key=Value2",
        INI_ERROR_KEY_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate key with identical values",
        L"[Section]\r\n"
        L"Key=Value\r\n"
        L"Key=Value",
        INI_ERROR_KEY_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate key with different values",
        L"[Section]\r\n"
        L"Key=Value1\r\n"
        L"Key=Value2",
        INI_ERROR_KEY_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate key separated by another key",
        L"[Section]\r\n"
        L"Key1=Value1\r\n"
        L"Key2=Value2\r\n"
        L"Key1=Value3",
        INI_ERROR_KEY_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate key separated by comments",
        L"[Section]\r\n"
        L"Key=Value1\r\n"
        L"; comment\r\n"
        L"# comment\r\n"
        L"Key=Value2",
        INI_ERROR_KEY_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate key separated by blank lines",
        L"[Section]\r\n"
        L"Key=Value1\r\n"
        L"\r\n"
        L"\r\n"
        L"Key=Value2",
        INI_ERROR_KEY_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate key with leading whitespace",
        L"[Section]\r\n"
        L"Key=Value1\r\n"
        L"    Key=Value2",
        INI_ERROR_KEY_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate key with trailing whitespace before equal sign",
        L"[Section]\r\n"
        L"Key=Value1\r\n"
        L"Key   =Value2",
        INI_ERROR_KEY_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate key with whitespace around equal sign",
        L"[Section]\r\n"
        L"Key=Value1\r\n"
        L"Key   =   Value2",
        INI_ERROR_KEY_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate key with tab before equal sign",
        L"[Section]\r\n"
        L"Key=Value1\r\n"
        L"Key\t=Value2",
        INI_ERROR_KEY_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate key with quoted values",
        L"[Section]\r\n"
        L"Key=\"Value1\"\r\n"
        L"Key=\"Value2\"",
        INI_ERROR_KEY_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate key with quoted and unquoted values",
        L"[Section]\r\n"
        L"Key=\"Value1\"\r\n"
        L"Key=Value2",
        INI_ERROR_KEY_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate key with empty values",
        L"[Section]\r\n"
        L"Key=\r\n"
        L"Key=",
        INI_ERROR_KEY_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate key with empty and non-empty values",
        L"[Section]\r\n"
        L"Key=\r\n"
        L"Key=Value",
        INI_ERROR_KEY_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Duplicate key with comments after values",
        L"[Section]\r\n"
        L"Key=Value1 ; comment\r\n"
        L"Key=Value2 # comment",
        INI_ERROR_KEY_ALREADY_EXIST
    );
    
    ExpectFailure(
        L"Three identical keys",
        L"[Section]\r\n"
        L"Key=Value1\r\n"
        L"Key=Value2\r\n"
        L"Key=Value3",
        INI_ERROR_KEY_ALREADY_EXIST
    );

    // =========================================================
    // Basic
    // =========================================================

    ExpectSuccess(
        L"Single section and single entry",
        L"[Section]\r\n"
        L"Key=Value",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Value")
            )
        )
    );

    ExpectSuccess(
        L"Multiple entries",
        L"[Section]\r\n"
        L"Key1=Value1\r\n"
        L"Key2=Value2\r\n"
        L"Key3=Value3",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key1", L"Value1"),
                INI_ENTRY(L"Key2", L"Value2"),
                INI_ENTRY(L"Key3", L"Value3")
            )
        )
    );

    ExpectSuccess(
        L"Multiple sections",
        L"[Section1]\r\n"
        L"Key1=Value1\r\n"
        L"[Section2]\r\n"
        L"Key2=Value2",
        INI_FILE(
            INI_SECTION(
                L"Section1",
                INI_ENTRY(L"Key1", L"Value1")
            ),
            INI_SECTION(
                L"Section2",
                INI_ENTRY(L"Key2", L"Value2")
            )
        )
    );

    ExpectSuccess(
        L"Multiple sections and multiple entries",
        L"[Section1]\r\n"
        L"Key1=Value1\r\n"
        L"Key2=Value2\r\n"
        L"[Section2]\r\n"
        L"Key3=Value3\r\n"
        L"Key4=Value4",
        INI_FILE(
            INI_SECTION(
                L"Section1",
                INI_ENTRY(L"Key1", L"Value1"),
                INI_ENTRY(L"Key2", L"Value2")
            ),
            INI_SECTION(
                L"Section2",
                INI_ENTRY(L"Key3", L"Value3"),
                INI_ENTRY(L"Key4", L"Value4")
            )
        )
    );

    // =========================================================
    // Empty values
    // =========================================================

    ExpectSuccess(
        L"Empty value",
        L"[Section]\r\n"
        L"Key=",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"")
            )
        )
    );

    ExpectSuccess(
        L"Empty value with spaces",
        L"[Section]\r\n"
        L"Key=   ",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"")
            )
        )
    );

    ExpectSuccess(
        L"Empty value with comment",
        L"[Section]\r\n"
        L"Key=; comment",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"")
            )
        )
    );

    ExpectSuccess(
        L"Empty quoted value",
        L"[Section]\r\n"
        L"Key=\"\"",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"")
            )
        )
    );

    // =========================================================
    // Whitespace
    // =========================================================

    ExpectSuccess(
        L"Whitespace around section name",
        L"[   Section   ]",
        INI_FILE(
            INI_SECTION(
                L"Section"
            )
        )
    );

    ExpectSuccess(
        L"Whitespace around equal sign",
        L"[Section]\r\n"
        L"Key   =   Value",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Value")
            )
        )
    );

    ExpectSuccess(
        L"Leading whitespace before entry",
        L"[Section]\r\n"
        L"    Key=Value",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Value")
            )
        )
    );

    ExpectSuccess(
        L"Trailing whitespace in key",
        L"[Section]\r\n"
        L"Key   =Value",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Value")
            )
        )
    );

    ExpectSuccess(
        L"Whitespace before section declaration",
        L"    [Section]",
        INI_FILE(
            INI_SECTION(
                L"Section"
            )
        )
    );

    ExpectSuccess(
        L"Tab around section name",
        L"[\tSection\t]",
        INI_FILE(
            INI_SECTION(
                L"Section"
            )
        )
    );

    // =========================================================
    // Quoted values
    // =========================================================

    ExpectSuccess(
        L"Quoted value",
        L"[Section]\r\n"
        L"Key=\"Hello World\"",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Hello World")
            )
        )
    );

    ExpectSuccess(
        L"Quoted empty value",
        L"[Section]\r\n"
        L"Key=\"\"",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"")
            )
        )
    );

    ExpectSuccess(
        L"Quoted value with leading and trailing spaces",
        L"[Section]\r\n"
        L"Key=\"  Hello World  \"",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"  Hello World  ")
            )
        )
    );

    ExpectSuccess(
        L"Quoted value containing equal sign",
        L"[Section]\r\n"
        L"Key=\"A=B=C\"",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"A=B=C")
            )
        )
    );

    ExpectSuccess(
        L"Quoted value containing semicolon",
        L"[Section]\r\n"
        L"Key=\"Value;Comment\"",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Value;Comment")
            )
        )
    );

    ExpectSuccess(
        L"Quoted value containing hash",
        L"[Section]\r\n"
        L"Key=\"Value#Comment\"",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Value#Comment")
            )
        )
    );

    ExpectSuccess(
        L"Quoted value followed by comment",
        L"[Section]\r\n"
        L"Key=\"Value\" ; comment",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Value")
            )
        )
    );

    // =========================================================
    // Unquoted values
    // =========================================================

    ExpectSuccess(
        L"Value containing equal signs",
        L"[Section]\r\n"
        L"Key=A=B=C",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"A=B=C")
            )
        )
    );

    ExpectSuccess(
        L"Value containing internal spaces",
        L"[Section]\r\n"
        L"Key=Hello World",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Hello World")
            )
        )
    );

    ExpectSuccess(
        L"Value with trailing comment",
        L"[Section]\r\n"
        L"Key=Value ; comment",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Value")
            )
        )
    );

    ExpectSuccess(
        L"Value with trailing hash comment",
        L"[Section]\r\n"
        L"Key=Value # comment",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Value")
            )
        )
    );

    ExpectSuccess(
        L"Value containing semicolon without whitespace",
        L"[Section]\r\n"
        L"Key=Value;Comment",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Value")
            )
        )
    );

    ExpectSuccess(
        L"Value containing hash without whitespace",
        L"[Section]\r\n"
        L"Key=Value#Comment",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Value")
            )
        )
    );

    ExpectSuccess(
        L"Value containing multiple internal spaces",
        L"[Section]\r\n"
        L"Key=Hello   World",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Hello   World")
            )
        )
    );

    // =========================================================
    // Comments
    // =========================================================

    ExpectSuccess(
        L"Semicolon comment",
        L"; comment\r\n"
        L"[Section]\r\n"
        L"Key=Value",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Value")
            )
        )
    );

    ExpectSuccess(
        L"Hash comment",
        L"# comment\r\n"
        L"[Section]\r\n"
        L"Key=Value",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Value")
            )
        )
    );

    ExpectSuccess(
        L"Comment after section declaration",
        L"[Section] ; comment\r\n"
        L"Key=Value",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Value")
            )
        )
    );

    ExpectSuccess(
        L"Hash comment after section declaration",
        L"[Section] # comment\r\n"
        L"Key=Value",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Value")
            )
        )
    );

    ExpectSuccess(
        L"Comments between entries",
        L"[Section]\r\n"
        L"Key1=Value1\r\n"
        L"; comment\r\n"
        L"# comment\r\n"
        L"Key2=Value2",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key1", L"Value1"),
                INI_ENTRY(L"Key2", L"Value2")
            )
        )
    );

    // =========================================================
    // Line endings
    // =========================================================

    ExpectSuccess(
        L"LF line endings",
        L"[Section]\n"
        L"Key1=Value1\n"
        L"Key2=Value2",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key1", L"Value1"),
                INI_ENTRY(L"Key2", L"Value2")
            )
        )
    );

    ExpectSuccess(
        L"CR line endings",
        L"[Section]\r"
        L"Key1=Value1\r"
        L"Key2=Value2",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key1", L"Value1"),
                INI_ENTRY(L"Key2", L"Value2")
            )
        )
    );

    ExpectSuccess(
        L"Mixed line endings",
        L"[Section]\r\n"
        L"Key1=Value1\n"
        L"Key2=Value2\r"
        L"Key3=Value3",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key1", L"Value1"),
                INI_ENTRY(L"Key2", L"Value2"),
                INI_ENTRY(L"Key3", L"Value3")
            )
        )
    );

    // =========================================================
    // Empty sections
    // =========================================================

    ExpectSuccess(
        L"Empty section",
        L"[Section]",
        INI_FILE(
            INI_SECTION(L"Section")
        )
    );

    ExpectSuccess(
        L"Multiple empty sections",
        L"[Section1]\r\n"
        L"[Section2]",
        INI_FILE(
            INI_SECTION(L"Section1"),
            INI_SECTION(L"Section2")
        )
    );

    ExpectSuccess(
        L"Empty section between non-empty sections",
        L"[Section1]\r\n"
        L"Key=Value\r\n"
        L"[Section2]\r\n"
        L"[Section3]\r\n"
        L"Key2=Value2",
        INI_FILE(
            INI_SECTION(
                L"Section1",
                INI_ENTRY(L"Key", L"Value")
            ),
            INI_SECTION(L"Section2"),
            INI_SECTION(
                L"Section3",
                INI_ENTRY(L"Key2", L"Value2")
            )
        )
    );

    // =========================================================
    // Special characters and Unicode
    // =========================================================

    ExpectSuccess(
        L"Japanese section and entry",
        L"[一般]\r\n"
        L"名前=太郎",
        INI_FILE(
            INI_SECTION(
                L"一般",
                INI_ENTRY(L"名前", L"太郎")
            )
        )
    );

    ExpectSuccess(
        L"Unicode value",
        L"[Section]\r\n"
        L"Key=こんにちは世界",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"こんにちは世界")
            )
        )
    );

    ExpectSuccess(
        L"Special characters in value",
        L"[Section]\r\n"
        L"Key=!@#$%^&*()_+-=[]{};:',.<>/?",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"!@")
            )
        )
    );

    ExpectSuccess(
        L"Special characters in section name",
        L"[Section-123_ABC]",
        INI_FILE(
            INI_SECTION(L"Section-123_ABC")
        )
    );

    // =========================================================
    // Blank lines
    // =========================================================

    ExpectSuccess(
        L"Leading blank lines",
        L"\r\n"
        L"\r\n"
        L"[Section]\r\n"
        L"Key=Value",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Value")
            )
        )
    );

    ExpectSuccess(
        L"Trailing blank lines",
        L"[Section]\r\n"
        L"Key=Value\r\n"
        L"\r\n"
        L"\r\n",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Value")
            )
        )
    );

    ExpectSuccess(
        L"Blank lines between entries",
        L"[Section]\r\n"
        L"Key1=Value1\r\n"
        L"\r\n"
        L"\r\n"
        L"Key2=Value2",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key1", L"Value1"),
                INI_ENTRY(L"Key2", L"Value2")
            )
        )
    );

    // =========================================================
    // Similar but different section names
    // =========================================================
    
    ExpectSuccess(
        L"Section names differing by case",
        L"[Section]\r\n"
        L"[section]",
        INI_FILE(
            INI_SECTION(L"Section"),
            INI_SECTION(L"section")
        )
    );
    
    ExpectSuccess(
        L"Section names with common prefix",
        L"[Section]\r\n"
        L"[Section2]",
        INI_FILE(
            INI_SECTION(L"Section"),
            INI_SECTION(L"Section2")
        )
    );
    
    ExpectSuccess(
        L"Section names with common suffix",
        L"[Section1]\r\n"
        L"[OtherSection1]",
        INI_FILE(
            INI_SECTION(L"Section1"),
            INI_SECTION(L"OtherSection1")
        )
    );
    
    // =========================================================
    // Similar but different key names
    // =========================================================
    
    ExpectSuccess(
        L"Key names differing by case",
        L"[Section]\r\n"
        L"Key=Value1\r\n"
        L"key=Value2",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Value1"),
                INI_ENTRY(L"key", L"Value2")
            )
        )
    );
    
    ExpectSuccess(
        L"Key names with common prefix",
        L"[Section]\r\n"
        L"Key=Value1\r\n"
        L"Key2=Value2",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key", L"Value1"),
                INI_ENTRY(L"Key2", L"Value2")
            )
        )
    );
    
    ExpectSuccess(
        L"Key names with common suffix",
        L"[Section]\r\n"
        L"Key1=Value1\r\n"
        L"OtherKey1=Value2",
        INI_FILE(
            INI_SECTION(
                L"Section",
                INI_ENTRY(L"Key1", L"Value1"),
                INI_ENTRY(L"OtherKey1", L"Value2")
            )
        )
    );
    
    ExpectSuccess(
        L"Same key names in different sections",
        L"[Section1]\r\n"
        L"Key=Value1\r\n"
        L"[Section2]\r\n"
        L"Key=Value2",
        INI_FILE(
            INI_SECTION(
                L"Section1",
                INI_ENTRY(L"Key", L"Value1")
            ),
            INI_SECTION(
                L"Section2",
                INI_ENTRY(L"Key", L"Value2")
            )
        )
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

