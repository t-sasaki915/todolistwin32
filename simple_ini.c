#include "simple_ini.h"

#include <windows.h>
#include <wchar.h>

#define END_OF_LINE (-10)
#define CHARACTER_NOT_FOUND (-10)

typedef struct
{
    wchar_t *currentSectionName;

    IniEntry *currentSectionIniEntries;
    int currentSectionNumberOfEntries;
    int currentSectionEntryCapacity;

    IniSection *iniSections;
    int numberOfSections;
    int iniSectionsCapacity;
} IniReadContext;

#define ACCEPTABLE_TRUE_VALUE_COUNT 9
static const wchar_t *g_acceptableTrueValue[ACCEPTABLE_TRUE_VALUE_COUNT] =
    { L"TRUE", L"True", L"true", L"YES", L"Yes", L"yes", L"Y", L"y", L"1" };

#define ACCEPTABLE_FALSE_VALUE_COUNT 9
static const wchar_t *g_acceptableFalseValue[ACCEPTABLE_FALSE_VALUE_COUNT] =
    { L"FALSE", L"False", L"false", L"NO", L"No", L"no", L"N", L"n", L"0" };

static IniError g_lastIniError = INI_ERROR_NO_ERROR;

IniError GetLastIniError(void)
{
    return g_lastIniError;
}

void FreeIniSections(IniSection *iniSections, int numberOfSections)
{
    if (iniSections == NULL)
    {
        return;
    }

    for (int i = 0; i < numberOfSections; i ++)
    {
        IniSection *section = &(iniSections[i]);
        if (section->sectionName != NULL)
        {
            free(section->sectionName);
            section->sectionName = NULL;
        }

        if (section->iniEntries != NULL)
        {
            for (int j = 0; j < section->numberOfEntries; j ++)
            {
                IniEntry *entry = &(section->iniEntries[j]);
                if (entry->key != NULL)
                {
                    free(entry->key);
                    entry->key = NULL;
                }
                if(entry->value != NULL)
                {
                    free(entry->value);
                    entry->value = NULL;
                }
            }

            free(section->iniEntries);
            section->iniEntries = NULL;
        }
    }

    free(iniSections);
}

void FreeIniReadContext(IniReadContext *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    if (ctx->currentSectionName != NULL)
    {
        free(ctx->currentSectionName);
        ctx->currentSectionName = NULL;
    }

    if (ctx->iniSections != NULL)
    {
        FreeIniSections(ctx->iniSections, ctx->numberOfSections);
        ctx->iniSections = NULL;
    }

    if (ctx->currentSectionIniEntries != NULL)
    {
        for (int i = 0; i < ctx->currentSectionNumberOfEntries; i ++)
        {
            IniEntry *entry = &(ctx->currentSectionIniEntries[i]);
            if (entry->key != NULL)
            {
                free(entry->key);
                entry->key = NULL;
            }
            if (entry->value != NULL)
            {
                free(entry->value);
                entry->value = NULL;
            }
        }

        free(ctx->currentSectionIniEntries);
        ctx->currentSectionIniEntries = NULL;
    }
}

void FreeIniFile(IniFile *iniFile)
{
    if (iniFile == NULL)
    {
        return;
    }

    FreeIniSections(iniFile->iniSections, iniFile->numberOfSections);

    iniFile->iniSections = NULL;
    iniFile->numberOfSections = 0;
}

BOOL ReadIniFileRaw(const wchar_t *filePath, wchar_t **contentBufferPtr)
{
    if (filePath == NULL || contentBufferPtr == NULL)
    {
        g_lastIniError = INI_ERROR_INVALID_ARGUMENT;

        return FALSE;
    }

    *contentBufferPtr = NULL;

    char *rawBuffer = NULL;
    wchar_t *contentBuffer = NULL;

    HANDLE hFile = CreateFileW(
        filePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        g_lastIniError = INI_ERROR_CANNOT_OPEN_FILE;

        return FALSE;
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize))
    {
        g_lastIniError = INI_ERROR_CANNOT_GET_FILE_SIZE;

        goto Failed;
    }
    if (fileSize.QuadPart < 0 || fileSize.QuadPart > 1024 * 1024)
    {
        g_lastIniError = INI_ERROR_FILE_SIZE_INVALID;

        goto Failed;
    }

    DWORD fileSizeBytes = (DWORD) fileSize.QuadPart;

    rawBuffer = malloc(fileSizeBytes + 1);
    if (rawBuffer == NULL)
    {
        g_lastIniError = INI_ERROR_CANNOT_ALLOCATE_MEMORY;

        goto Failed;
    }

    DWORD bytesRead = 0;
    if (!ReadFile(hFile, rawBuffer, fileSizeBytes, &bytesRead, NULL))
    {
        g_lastIniError = INI_ERROR_CANNOT_READ_FILE;

        goto Failed;
    }
    
    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

    if (bytesRead != fileSizeBytes)
    {
        g_lastIniError = INI_ERROR_BYTE_READ_MISMATCH;

        goto Failed;
    }

    rawBuffer[bytesRead] = '\0';

    DWORD offset = 0;
    if (bytesRead >= 3)
    {
        unsigned char buf0 = (unsigned char) rawBuffer[0];
        unsigned char buf1 = (unsigned char) rawBuffer[1];
        unsigned char buf2 = (unsigned char) rawBuffer[2];
        
        if (buf0 == 0xEF && buf1 == 0xBB && buf2 == 0xBF)
        {
            offset = 3;
        }
    }

    DWORD inputBytes = bytesRead - offset;

    if (inputBytes == 0)
    {
        *contentBufferPtr = _wcsdup(L"");
        if (*contentBufferPtr == NULL)
        {
            g_lastIniError = INI_ERROR_CANNOT_ALLOCATE_MEMORY;

            goto Failed;
        }

        goto Success;
    }

    int requiredChars = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        rawBuffer + offset,
        inputBytes,
        NULL,
        0
    );
    if (requiredChars == 0)
    {
        g_lastIniError = INI_ERROR_INVALID_UTF8;

        goto Failed;
    }

    contentBuffer = malloc((requiredChars + 1) * sizeof(wchar_t));
    if (contentBuffer == NULL)
    {
        g_lastIniError = INI_ERROR_CANNOT_ALLOCATE_MEMORY;

        goto Failed;
    }

    int convertedChars = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        rawBuffer + offset,
        inputBytes,
        contentBuffer,
        requiredChars
    );

    if (convertedChars != requiredChars)
    {
        g_lastIniError = INI_ERROR_WIDECHAR_CONVERT_BYTE_MISMATCH;

        goto Failed;
    }

    contentBuffer[requiredChars] = L'\0';

    *contentBufferPtr = contentBuffer;

Success:
    free(rawBuffer);
    rawBuffer = NULL;

    return TRUE;

Failed:
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }
    if (rawBuffer != NULL)
    {
        free(rawBuffer);
        rawBuffer = NULL;
    }
    if (contentBuffer != NULL)
    {
        free(contentBuffer);
        contentBuffer = NULL;
    }

    return FALSE;
}

BOOL IsWhitespace(wchar_t character)
{
    return (character == L' ' || character == L'\t');
}

int ConsumeWhitespaces(int currentIndex, wchar_t *buffer, int lineLength)
{
    for (int i = currentIndex; i < lineLength; i ++)
    {
        if (!IsWhitespace(buffer[i]))
        {
            return i;
        }
    }

    return END_OF_LINE;
}

BOOL DoesCommentStart(int currentIndex, wchar_t *buffer)
{
    return (buffer[currentIndex] == L';' || buffer[currentIndex] == L'#');
}

int FindFirstCharacter(wchar_t character, int currentIndex, wchar_t *buffer, int lineLength)
{
    for (int i = currentIndex; i < lineLength; i ++)
    {
        if (buffer[i] == character)
        {
            return i;
        }
    }

    return CHARACTER_NOT_FOUND;
}

BOOL IsLastCharacterOfLine(int currentIndex, int lineLength)
{
    return (currentIndex + 1 >= lineLength);
}

BOOL ExpectCharacter(wchar_t character, int currentIndex, wchar_t *buffer)
{
    return (buffer[currentIndex] == character);
}

BOOL UnexpectedCharacter(wchar_t character, int currentIndex, wchar_t *buffer)
{
    return (buffer[currentIndex] == character);
}

BOOL IsThisWhitespaceFollowedBy(wchar_t character, int currentIndex, wchar_t *buffer, int lineLength)
{
    if (!IsWhitespace(buffer[currentIndex]))
    {
        return FALSE;
    }

    int indexOfSomeNonSpace = ConsumeWhitespaces(currentIndex, buffer, lineLength);
    if (indexOfSomeNonSpace == END_OF_LINE)
    {
        return FALSE;
    }

    return (buffer[indexOfSomeNonSpace] == character);
}

int FindNonWhitespaceBefore(wchar_t character, int currentIndex, wchar_t *buffer, int lineLength)
{
    for (int i = currentIndex; i < lineLength - 1; i ++)
    {
        if (!IsWhitespace(buffer[i]))
        {
            if (buffer[i + 1] == character)
            {
                return i;
            }
            if (IsWhitespace(buffer[i + 1]) && IsThisWhitespaceFollowedBy(character, i + 1, buffer, lineLength))
            {
                return i;
            }
        }
    }

    return CHARACTER_NOT_FOUND;
}

int FindUnquotedValueEnd(int currentIndex, wchar_t *buffer, int lineLength)
{
    for (int i = currentIndex; i < lineLength; i ++)
    {
        if (DoesCommentStart(i, buffer))
        {
            return i - 1;
        }

        if (IsWhitespace(buffer[i]))
        {
            int nextIndex = ConsumeWhitespaces(i, buffer, lineLength);
            if (nextIndex == END_OF_LINE || DoesCommentStart(nextIndex, buffer))
            {
                return i - 1;
            }
        }
    }
    
    return lineLength - 1;
}

BOOL FinaliseCurrentSection(IniReadContext *ctx, BOOL allocateNewAreas)
{
    if (ctx->numberOfSections >= ctx->iniSectionsCapacity)
    {
        int newCapacity = ctx->iniSectionsCapacity == 0 ? 8
                                                        : ctx->iniSectionsCapacity * 2;

        IniSection *newIniSections = realloc(
            ctx->iniSections,
            sizeof(IniSection) * newCapacity
        );
        if (newIniSections == NULL)
        {
            g_lastIniError = INI_ERROR_CANNOT_ALLOCATE_MEMORY;

            return FALSE;
        }

        ctx->iniSections         = newIniSections;
        ctx->iniSectionsCapacity = newCapacity;
    }

    ctx->iniSections[ctx->numberOfSections].sectionName     = ctx->currentSectionName;
    ctx->iniSections[ctx->numberOfSections].iniEntries      = ctx->currentSectionIniEntries;
    ctx->iniSections[ctx->numberOfSections].numberOfEntries = ctx->currentSectionNumberOfEntries;

    (ctx->numberOfSections) ++;

    ctx->currentSectionName            = NULL;
    ctx->currentSectionIniEntries      = NULL;
    ctx->currentSectionNumberOfEntries = 0;

    if (allocateNewAreas)
    {
        ctx->currentSectionIniEntries = malloc(sizeof(IniEntry) * 1024);
        if (ctx->currentSectionIniEntries == NULL)
        {
            g_lastIniError = INI_ERROR_CANNOT_ALLOCATE_MEMORY;

            return FALSE;
        }
    }

    return TRUE;
}

BOOL DoesSectionWithThisNameAlreadyExist(wchar_t *sectionName, IniReadContext *ctx)
{
    for (int i = 0; i < ctx->numberOfSections; i ++)
    {
        if (wcscmp(sectionName, ctx->iniSections[i].sectionName) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL DoesKeyWithThisNameAlreadyExist(wchar_t *keyName, IniReadContext *ctx)
{
    for (int i = 0; i < ctx->currentSectionNumberOfEntries; i ++)
    {
        if (wcscmp(keyName, ctx->currentSectionIniEntries[i].key) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL ProcessSectionLine(int startIndex, wchar_t *buffer, int lineLength, IniReadContext *ctx)
{
    if (!ExpectCharacter(L'[', startIndex, buffer))
    {
        g_lastIniError = INI_ERROR_OPEN_SQUARE_BRACKET_EXPECTED;

        return FALSE;
    }
    
    int sectionNameStartIndex = ConsumeWhitespaces(startIndex + 1, buffer, lineLength);
    if (sectionNameStartIndex == END_OF_LINE)
    {
        g_lastIniError = INI_ERROR_UNEXPECTED_END_OF_LINE;

        return FALSE;
    }
    if (UnexpectedCharacter(L']', sectionNameStartIndex, buffer))
    {
        g_lastIniError = INI_ERROR_EMPTY_SECTION_NAME;

        return FALSE;
    }

    int sectionNameEndIndex = FindNonWhitespaceBefore(L']', sectionNameStartIndex, buffer, lineLength);
    if (sectionNameEndIndex == CHARACTER_NOT_FOUND)
    {
        g_lastIniError = INI_ERROR_CLOSE_SQUARE_BRACKET_NOT_FOUND;

        return FALSE;
    }

    int closeBracketIndex = ConsumeWhitespaces(sectionNameEndIndex + 1, buffer, lineLength);
    if (closeBracketIndex == END_OF_LINE)
    {
        g_lastIniError = INI_ERROR_CLOSE_SQUARE_BRACKET_NOT_FOUND;

        return FALSE;
    }
    if (!ExpectCharacter(L']', closeBracketIndex, buffer))
    {
        g_lastIniError = INI_ERROR_CLOSE_SQUARE_BRACKET_EXPECTED;

        return FALSE;
    }

    int remainingIndex = ConsumeWhitespaces(closeBracketIndex + 1, buffer, lineLength);
    if (remainingIndex != END_OF_LINE && !DoesCommentStart(remainingIndex, buffer))
    {
        g_lastIniError = INI_ERROR_EXPECTED_END_OF_LINE;

        return FALSE;
    }

    int sectionNameLength = sectionNameEndIndex - sectionNameStartIndex + 1;

    wchar_t *sectionName = malloc(sizeof(wchar_t) * (sectionNameLength + 1));
    if (sectionName == NULL)
    {
        g_lastIniError = INI_ERROR_CANNOT_ALLOCATE_MEMORY;

        return FALSE;
    }

    memcpy(sectionName, &(buffer[sectionNameStartIndex]), sizeof(wchar_t) * sectionNameLength);
    sectionName[sectionNameLength] = L'\0';

    if (ctx->currentSectionName != NULL)
    {
        if (!FinaliseCurrentSection(ctx, TRUE))
        {
            return FALSE;
        }
    }

    if (DoesSectionWithThisNameAlreadyExist(sectionName, ctx))
    {
        free(sectionName);

        g_lastIniError = INI_ERROR_SECTION_ALREADY_EXIST;

        return FALSE;
    }

    ctx->currentSectionName = sectionName;

    return TRUE;
}

BOOL ProcessEntryValue(int startIndex, wchar_t *buffer, int lineLength, wchar_t **valuePtr)
{
    int keyValueStartIndex = ConsumeWhitespaces(startIndex, buffer, lineLength);
    if (keyValueStartIndex == END_OF_LINE || DoesCommentStart(keyValueStartIndex, buffer))
    {
        *valuePtr = _wcsdup(L"");
        if (*valuePtr == NULL)
        {
            g_lastIniError = INI_ERROR_CANNOT_ALLOCATE_MEMORY;
            
            return FALSE;
        }

        return TRUE;
    }

    if (buffer[keyValueStartIndex] == L'"')
    {
        int closeQuoteIndex = FindFirstCharacter(L'"', keyValueStartIndex + 1, buffer, lineLength);
        if (closeQuoteIndex == CHARACTER_NOT_FOUND)
        {
            g_lastIniError = INI_ERROR_QUOTE_NOT_CLOSED;

            return FALSE;
        }
        if (!ExpectCharacter(L'"', closeQuoteIndex, buffer))
        {
            g_lastIniError = INI_ERROR_CLOSE_QUOTE_EXPECTED;

            return FALSE;
        }

        int remainingIndex = ConsumeWhitespaces(closeQuoteIndex + 1, buffer, lineLength);
        if (remainingIndex != END_OF_LINE && !DoesCommentStart(remainingIndex, buffer))
        {
            g_lastIniError = INI_ERROR_EXPECTED_END_OF_LINE;

            return FALSE;
        }

        int quotedValueLength = closeQuoteIndex - keyValueStartIndex - 1;

        wchar_t *quotedValue = malloc(sizeof(wchar_t) * (quotedValueLength + 1));
        if (quotedValue == NULL)
        {
            g_lastIniError = INI_ERROR_CANNOT_ALLOCATE_MEMORY;

            return FALSE;
        }

        memcpy(quotedValue, &(buffer[keyValueStartIndex + 1]), sizeof(wchar_t) * quotedValueLength);
        quotedValue[quotedValueLength] = L'\0';

        *valuePtr = quotedValue;

        return TRUE;
    }
    else
    {
        int valueEndIndex = FindUnquotedValueEnd(keyValueStartIndex, buffer, lineLength);

        int valueLength = valueEndIndex - keyValueStartIndex + 1;

        wchar_t *unquotedValue = malloc(sizeof(wchar_t) * (valueLength + 1));
        if (unquotedValue == NULL)
        {
            g_lastIniError = INI_ERROR_CANNOT_ALLOCATE_MEMORY;

            return FALSE;
        }

        memcpy(unquotedValue, &(buffer[keyValueStartIndex]), sizeof(wchar_t) * valueLength);
        unquotedValue[valueLength] = L'\0';

        *valuePtr = unquotedValue;

        return TRUE;    
    }
}

BOOL AddEntryToCurrentSection(wchar_t *key, wchar_t *value, IniReadContext *ctx)
{
    if (DoesKeyWithThisNameAlreadyExist(key, ctx))
    {
        g_lastIniError = INI_ERROR_KEY_ALREADY_EXIST;

        return FALSE;
    }

    if (ctx->currentSectionNumberOfEntries >= ctx->currentSectionEntryCapacity)
    {
        int newCapacity =
            ctx->currentSectionEntryCapacity == 0 ? 8
                                                  : ctx->currentSectionEntryCapacity * 2;

        IniEntry *newEntries = realloc(
            ctx->currentSectionIniEntries,
            sizeof(IniEntry) * newCapacity
        );
        if (newEntries == NULL)
        {
            g_lastIniError = INI_ERROR_CANNOT_ALLOCATE_MEMORY;

            return FALSE;
        }

        ctx->currentSectionIniEntries    = newEntries;
        ctx->currentSectionEntryCapacity = newCapacity;
    }

    ctx->currentSectionIniEntries[ctx->currentSectionNumberOfEntries].key   = key;
    ctx->currentSectionIniEntries[ctx->currentSectionNumberOfEntries].value = value;

    (ctx->currentSectionNumberOfEntries) ++;

    return TRUE;
}

BOOL ProcessEntryLine(int startIndex, wchar_t *buffer, int lineLength, IniReadContext *ctx)
{
    if (ctx->currentSectionName == NULL)
    {
        g_lastIniError = INI_ERROR_ORPHAN_ENTRY;

        return FALSE;
    }

    if (UnexpectedCharacter(L'=', startIndex, buffer))
    {
        g_lastIniError = INI_ERROR_EMPTY_KEY_NAME;

        return FALSE;
    }

    int keyNameStartIndex = startIndex;

    int keyNameEndIndex = FindNonWhitespaceBefore(L'=', keyNameStartIndex, buffer, lineLength);
    if (keyNameEndIndex == CHARACTER_NOT_FOUND)
    {
        g_lastIniError = INI_ERROR_EQUAL_SIGN_NOT_FOUND;

        return FALSE;
    }

    int equalSignIndex = ConsumeWhitespaces(keyNameEndIndex + 1, buffer, lineLength);
    if (equalSignIndex == END_OF_LINE)
    {
        g_lastIniError = INI_ERROR_EQUAL_SIGN_NOT_FOUND;

        return FALSE;
    }

    if (!ExpectCharacter(L'=', equalSignIndex, buffer))
    {
        g_lastIniError = INI_ERROR_EXPECTED_EQUAL_SIGN;

        return FALSE;
    }

    wchar_t *value = NULL;
    if (!ProcessEntryValue(equalSignIndex + 1, buffer, lineLength, &value))
    {
        return FALSE;
    }

    int keyNameLength = keyNameEndIndex - keyNameStartIndex + 1;

    wchar_t *keyName = malloc(sizeof(wchar_t) * (keyNameLength + 1));
    if (keyName == NULL)
    {
        free(value);

        g_lastIniError = INI_ERROR_CANNOT_ALLOCATE_MEMORY;

        return FALSE;
    }

    memcpy(keyName, &(buffer[keyNameStartIndex]), sizeof(wchar_t) * keyNameLength);
    keyName[keyNameLength] = L'\0';
    
    if (!AddEntryToCurrentSection(keyName, value, ctx))
    {
        free(keyName);
        free(value);

        return FALSE;
    }

    return TRUE;
}

BOOL ProcessLine(wchar_t *buffer, int lineLength, IniReadContext *ctx)
{
    int startIndex = ConsumeWhitespaces(0, buffer, lineLength);
    if (startIndex == END_OF_LINE || DoesCommentStart(startIndex, buffer))
    {
        return TRUE;
    }

    if (buffer[startIndex] == L']')
    {
        g_lastIniError = INI_ERROR_UNEXPECTED_CLOSE_SQUARE_BRACKET;

        return FALSE;
    }
    else if (buffer[startIndex] == L'[')
    {
        return ProcessSectionLine(startIndex, buffer, lineLength, ctx);
    }
    else
    {
        return ProcessEntryLine(startIndex, buffer, lineLength, ctx);
    }
}

BOOL ProcessLines(wchar_t *contentBuffer, IniReadContext *ctx)
{
    wchar_t *lineStart = contentBuffer;
    wchar_t *current = contentBuffer;

    while (*current != L'\0')
    {
        if (*current != L'\r' && *current != L'\n')
        {
            current ++;
            continue;
        }

        int lineLength = (int) (current - lineStart);
        if (lineLength != 0)
        {
            if (!ProcessLine(lineStart, lineLength, ctx))
            {
                return FALSE;
            }
        }

        if (current[0] == L'\r' && current[1] == L'\n')
        {
            current += 2;
        }
        else
        {
            current += 1;
        }

        lineStart = current;
    }

    int lineLength = (int) (current - lineStart);
    if (lineLength != 0)
    {
        if (!ProcessLine(lineStart, lineLength, ctx))
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL ReadIniFileWchar(wchar_t *contentBuffer, IniFile *iniFile)
{
    if (contentBuffer == NULL || iniFile == NULL)
    {
        g_lastIniError = INI_ERROR_INVALID_ARGUMENT;

        return FALSE;
    }

    IniReadContext ctx                = {0};
    ctx.currentSectionName            = NULL;
    ctx.currentSectionIniEntries      = NULL;
    ctx.currentSectionNumberOfEntries = 0;
    ctx.currentSectionEntryCapacity   = 0;
    ctx.iniSections                   = NULL;
    ctx.numberOfSections              = 0;
    ctx.iniSectionsCapacity           = 0;

    if (!ProcessLines(contentBuffer, &ctx))
    {
        FreeIniReadContext(&ctx);

        return FALSE;
    }

    if (!FinaliseCurrentSection(&ctx, FALSE))
    {
        FreeIniReadContext(&ctx);

        return FALSE;
    }

    iniFile->iniSections      = ctx.iniSections;
    iniFile->numberOfSections = ctx.numberOfSections;

    return TRUE;
}

BOOL ReadIniFile(const wchar_t *filePath, IniFile *iniFile)
{
    if (filePath == NULL || iniFile == NULL)
    {
        g_lastIniError = INI_ERROR_INVALID_ARGUMENT;

        return FALSE;
    }

    wchar_t *contentBuffer = NULL;
    if (!ReadIniFileRaw(filePath, &contentBuffer))
    {
        return FALSE;
    }

    BOOL result = ReadIniFileWchar(contentBuffer, iniFile);

    free(contentBuffer);

    return result;
}

BOOL GetStringFromIni(IniFile *iniFile, wchar_t *sectionName, wchar_t *keyName, wchar_t **resultPtr)
{
    for (int i = 0; i < iniFile->numberOfSections; i ++)
    {
        IniSection iniSection = iniFile->iniSections[i];

        if (wcscmp(iniSection.sectionName, sectionName) == 0)
        {
            for (int j = 0; j < iniSection.numberOfEntries; j ++)
            {
                IniEntry iniEntry = iniSection.iniEntries[j];
                
                if (wcscmp(iniEntry.key, keyName) == 0)
                {
                    if (wcslen(iniEntry.value) == 0)
                    {
                        g_lastIniError = INI_ERROR_EMPTY_VALUE;

                        return FALSE;
                    }

                    *resultPtr = iniEntry.value;

                    return TRUE;
                }
            }

            g_lastIniError = INI_ERROR_KEY_NOT_FOUND;

            return FALSE;
        }
    }

    g_lastIniError = INI_ERROR_SECTION_NOT_FOUND;

    return FALSE;
}

BOOL GetBoolFromIni(IniFile *iniFile, wchar_t *sectionName, wchar_t *keyName, BOOL *resultPtr)
{
    wchar_t *stringValue;
    if (!GetStringFromIni(iniFile, sectionName, keyName, &stringValue))
    {
        return FALSE;
    }

    for (int i = 0; i < ACCEPTABLE_TRUE_VALUE_COUNT; i ++)
    {
        if (wcscmp(stringValue, g_acceptableTrueValue[i]) == 0)
        {
            *resultPtr = TRUE;

            return TRUE;
        }
    }

    for (int i = 0; i < ACCEPTABLE_FALSE_VALUE_COUNT; i ++)
    {
        if (wcscmp(stringValue, g_acceptableFalseValue[i]) == 0)
        {
            *resultPtr = FALSE;

            return TRUE;
        }
    }

    g_lastIniError = INI_ERROR_INVALID_BOOL_VALUE;

    return FALSE;
}

LONG GetLongFromIni(IniFile *iniFile, wchar_t *sectionName, wchar_t *keyName, LONG *resultPtr)
{
    wchar_t *stringValue;
    if (!GetStringFromIni(iniFile, sectionName, keyName, &stringValue))
    {
        return FALSE;
    }

    wchar_t *endPtr;
    errno = 0;
    LONG val = wcstol(stringValue, &endPtr, 10);

    if (endPtr == stringValue)
    {
        g_lastIniError = INI_ERROR_INVALID_LONG_VALUE;

        return FALSE;
    }
    if (*endPtr != L'\0')
    {
        g_lastIniError = INI_ERROR_INVALID_LONG_VALUE;

        return FALSE;
    }
    if (errno == ERANGE)
    {
        g_lastIniError = INI_ERROR_INVALID_LONG_VALUE;

        return FALSE;
    }

    *resultPtr = val;

    return TRUE;
}

BOOL SubstituteStringDefault(wchar_t **resultPtr, wchar_t *defValPtr, BOOL substituteEmptyValue)
{
    IniError err = GetLastIniError();
    if (err == INI_ERROR_SECTION_NOT_FOUND || err == INI_ERROR_KEY_NOT_FOUND)
    {
        g_lastIniError = INI_ERROR_NO_ERROR;

        *resultPtr = defValPtr;

        return TRUE;
    }

    if (substituteEmptyValue && err == INI_ERROR_EMPTY_VALUE)
    {
        g_lastIniError = INI_ERROR_NO_ERROR;

        *resultPtr = defValPtr;

        return TRUE;
    }

    return FALSE;
}

BOOL SubstituteBoolDefault(BOOL *resultPtr, BOOL defValPtr, BOOL substituteEmptyValue)
{
    IniError err = GetLastIniError();
    if (err == INI_ERROR_SECTION_NOT_FOUND || err == INI_ERROR_KEY_NOT_FOUND)
    {
        g_lastIniError = INI_ERROR_NO_ERROR;

        *resultPtr = defValPtr;

        return TRUE;
    }

    if (substituteEmptyValue && err == INI_ERROR_EMPTY_VALUE)
    {
        g_lastIniError = INI_ERROR_NO_ERROR;

        *resultPtr = defValPtr;

        return TRUE;
    }

    return FALSE;
}

BOOL SubstituteLongDefault(LONG *resultPtr, LONG defValPtr, BOOL substituteEmptyValue)
{
    IniError err = GetLastIniError();
    if (err == INI_ERROR_SECTION_NOT_FOUND || err == INI_ERROR_KEY_NOT_FOUND)
    {
        g_lastIniError = INI_ERROR_NO_ERROR;

        *resultPtr = defValPtr;

        return TRUE;
    }

    if (substituteEmptyValue && err == INI_ERROR_EMPTY_VALUE)
    {
        g_lastIniError = INI_ERROR_NO_ERROR;

        *resultPtr = defValPtr;

        return TRUE;
    }

    return FALSE;
}

BOOL WriteIniValueString(const wchar_t *filePath, wchar_t *section, wchar_t *key, wchar_t *value)
{
    wchar_t test[1024];
    swprintf(test, 1024, L"FilePath: %ls\nSection: %ls\nKey: %ls\nValue: %ls", filePath, section, key, value);

    MessageBoxW(NULL, test, L"WRITE", MB_OK);

    return TRUE;
}

BOOL WriteIniValueBool(const wchar_t *filePath, wchar_t *section, wchar_t *key, BOOL value)
{
    return WriteIniValueString(
        filePath,
        section,
        key,
        value ? L"TRUE" : L"FALSE"
    );
}

BOOL WriteIniValueLong(const wchar_t *filePath, wchar_t *section, wchar_t *key, LONG value)
{
    wchar_t longStr[32];
    swprintf_s(longStr, 32, L"%ld", value);
    
    return WriteIniValueString(
        filePath,
        section,
        key,
        longStr
    );
}

