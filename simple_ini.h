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
    INI_ERROR_CLOSE_QUOTE_EXPECTED            = 22,
    INI_ERROR_SECTION_ALREADY_EXIST           = 23,
    INI_ERROR_KEY_ALREADY_EXIST               = 24,
    INI_ERROR_SECTION_NOT_FOUND               = 25,
    INI_ERROR_KEY_NOT_FOUND                   = 26,
    INI_ERROR_INVALID_BOOL_VALUE              = 27,
    INI_ERROR_INVALID_LONG_VALUE              = 28,
    INI_ERROR_EMPTY_VALUE                     = 29
} IniError;

IniError GetLastIniError(void);

BOOL ReadIniFileWchar(wchar_t *contentBuffer, IniFile *iniFile);

BOOL ReadIniFile(const wchar_t *filePath, IniFile *iniFile);

void FreeIniFile(IniFile *iniFile);

BOOL GetStringFromIni(IniFile *iniFile, wchar_t *sectionName, wchar_t *keyName, wchar_t **resultPtr);

BOOL GetBoolFromIni(IniFile *iniFile, wchar_t *sectionName, wchar_t *keyName, BOOL *resultPtr);

LONG GetLongFromIni(IniFile *iniFile, wchar_t *sectionName, wchar_t *keyName, LONG *resultPtr);

BOOL SubstituteStringDefault(wchar_t **resultPtr, wchar_t *defValPtr, BOOL substituteEmptyValue);

BOOL SubstituteBoolDefault(BOOL *resultPtr, BOOL defValPtr, BOOL substituteEmptyValue);

BOOL SubstituteLongDefault(LONG *resultPtr, LONG defValPtr, BOOL substituteEmptyValue);

BOOL WriteIniValueString(const wchar_t *filePath, wchar_t *section, wchar_t *key, wchar_t *value);

BOOL WriteIniValueBool(const wchar_t *filePath, wchar_t *section, wchar_t *key, BOOL value);

BOOL WriteIniValueLong(const wchar_t *filePath, wchar_t *section, wchar_t *key, LONG value);

#endif

