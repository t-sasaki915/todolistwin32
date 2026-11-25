#include "config.h"

#include <windows.h>

#include "simple_ini.h"
#include "resource.h"
#include "util.h"

typedef struct
{
    BOOL topMostEnabled;
    wchar_t *fontName;
    LONG fontSize;
} RawConfigData;

static HINSTANCE g_hInstance = NULL;
static HWND g_mainHWND = NULL;

static RawConfigData g_defConfig = {
    TRUE,
    L"Tahoma",
    8
};

void InitialiseConfig(HINSTANCE hInstance, HWND mainHWND)
{
    g_hInstance = hInstance;
    g_mainHWND = mainHWND;
}

int PointSizeToHeight(int pointSize)
{
    HDC hdc = GetDC(g_mainHWND);
    int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);

    ReleaseDC(g_mainHWND, hdc);

    return -MulDiv(pointSize, dpiY, 72);
}

BOOL CreateConfigDataFromRawConfig(RawConfigData *rawData, ConfigData *data)
{
    HFONT customTextListFont = CreateFontW(
        PointSizeToHeight(rawData->fontSize),
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        rawData->fontName
    );
    if (customTextListFont == NULL)
    {
        MessageBoxUsingResourceString(
            g_hInstance,
            g_mainHWND,
            MB_ICONERROR | MB_OK,
            IDS_CONFIG_ERROR_TITLE,
            IDS_FAILED_TO_CREATE_FONT_MSG,
            GetLastError()
        );

        return FALSE;
    }

    data->topMostEnabled     = rawData->topMostEnabled;
    data->customTextListFont = customTextListFont;

    return TRUE;
}

BOOL LoadConfigData(ConfigData *data)
{
    IniFile iniFile;
    if (!ReadIniFile(L"C:\\Users\\tsasaki\\Desktop\\test.ini", &iniFile))
    {
        MessageBoxUsingResourceString(
            g_hInstance,
            g_mainHWND,
            MB_ICONERROR | MB_OK,
            IDS_CONFIG_ERROR_TITLE,
            IDS_FAILED_TO_READ_CONFIG_FILE_MSG,
            GetLastIniError()
        );

        return FALSE;
    }

    wchar_t *configCurrentlyReading = NULL;

    BOOL topMostEnabled;
    if (!GetBoolFromIni(&iniFile, L"General", L"AlwaysOnTop", &topMostEnabled))
    {
        if (!RecoverFromIniErrorUsingDefaultValue((void *) &topMostEnabled, (void *) &(g_defConfig.topMostEnabled), TRUE))
        {
            configCurrentlyReading = L"General.AlwaysOnTop";

            goto ShowErrorAndFail;
        }
    }

    wchar_t *fontName;
    if (!GetStringFromIni(&iniFile, L"Font", L"FontName", &fontName))
    {
        if (!RecoverFromIniErrorUsingDefaultValue((void *) &fontName, (void *) &(g_defConfig.fontName), TRUE))
        {
            configCurrentlyReading = L"Font.FontName";

            goto ShowErrorAndFail;
        }
    }

    LONG fontSize;
    if (!GetLongFromIni(&iniFile, L"Font", L"FontSize", &fontSize))
    {
        if (!RecoverFromIniErrorUsingDefaultValue((void *) &fontSize, (void *) &(g_defConfig.fontSize), TRUE))
        {
            configCurrentlyReading = L"Font.FontSize";

            goto ShowErrorAndFail;
        }
    }

    RawConfigData rawConfig  = {0};
    rawConfig.topMostEnabled = topMostEnabled;
    rawConfig.fontName       = fontName;
    rawConfig.fontSize       = fontSize;

    if (!CreateConfigDataFromRawConfig(&rawConfig, data))
    {
        goto Fail;
    }

    FreeIniFile(&iniFile);

    return TRUE;

ShowErrorAndFail:
    MessageBoxUsingResourceString(
        g_hInstance,
        g_mainHWND,
        MB_ICONERROR | MB_OK,
        IDS_CONFIG_ERROR_TITLE,
        IDS_INI_ERROR_MSG,
        configCurrentlyReading,
        GetLastIniError()
    );

Fail:
    FreeIniFile(&iniFile);

    return FALSE;
}

