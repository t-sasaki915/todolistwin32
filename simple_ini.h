#ifndef SIMPLE_INI_H
#define SIMPLE_INI_H

#include <windows.h>

typedef struct
{
    wchar_t *key;
    wchar_t *value;
} IniEntry;

typedef struct
{
    wchar_t *sectionName;
    IniEntry *iniEntries;

    int numberOfEntries;
} IniSection;

typedef struct
{
    IniSection *iniSections;

    int numberOfSections;
} IniFile;

typedef enum
{
    INI_ERROR_NO_ERROR                        = 0,
    INI_ERROR_INVALID_ARGUMENT                = 1,
    INI_ERROR_CANNOT_OPEN_FILE                = 2,
    INI_ERROR_CANNOT_GET_FILE_SIZE            = 3,
    INI_ERROR_FILE_SIZE_INVALID               = 4,
    INI_ERROR_CANNOT_READ_FILE                = 5,
    INI_ERROR_CANNOT_ALLOCATE_MEMORY          = 6,
    INI_ERROR_INVALID_UTF8                    = 7,
    INI_ERROR_WIDECHAR_CONVERT_BYTE_MISMATCH  = 8,
    INI_ERROR_BYTE_READ_MISMATCH              = 9,
    INI_ERROR_OPEN_SQUARE_BRACKET_EXPECTED    = 10,
    INI_ERROR_UNEXPECTED_END_OF_LINE          = 11,
    INI_ERROR_CLOSE_SQUARE_BRACKET_NOT_FOUND  = 12,
    INI_ERROR_CLOSE_SQUARE_BRACKET_EXPECTED   = 13,
    INI_ERROR_EMPTY_SECTION_NAME              = 14,
    INI_ERROR_EXPECTED_END_OF_LINE            = 15,
    INI_ERROR_UNEXPECTED_CLOSE_SQUARE_BRACKET = 16,
    INI_ERROR_ORPHAN_ENTRY                    = 17,
    INI_ERROR_EMPTY_KEY_NAME                  = 18,
    INI_ERROR_EQUAL_SIGN_NOT_FOUND            = 19,
    INI_ERROR_EXPECTED_EQUAL_SIGN             = 20,
    INI_ERROR_QUOTE_NOT_CLOSED                = 21,
    INI_ERROR_CLOSE_QUOTE_EXPECTED            = 22
} IniError;

IniError GetLastIniError(void);

BOOL ReadIniFileWchar(wchar_t *contentBuffer, IniFile *iniFile);

BOOL ReadIniFile(const wchar_t *filePath, IniFile *iniFile);

void FreeIniFile(IniFile *iniFile);

#endif

