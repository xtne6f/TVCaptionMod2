// TVTestに字幕を表示するプラグイン(based on TVCaption 2008-12-16 by odaru)
// 最終更新: 2012-05-17
// 署名: xt(849fa586809b0d16276cd644c6749503)
#include <Windows.h>
#include <Shlwapi.h>
#include <vector>
#include "Util.h"
#include "PseudoOSD.h"
#include "Caption.h"
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#define TVTEST_PLUGIN_VERSION TVTEST_PLUGIN_VERSION_(0,0,11)
#include "TVTestPlugin.h"
#include "TVCaption2.h"

// 外字テーブル
#include "GaijiTable_typebank.h"

#ifndef ASSERT
#include <cassert>
#define ASSERT assert
#endif

#define WM_RESET_CAPTION        (WM_APP + 0)
#define WM_PROCESS_CAPTION      (WM_APP + 1)
#define WM_DONE_MOVE            (WM_APP + 2)
#define WM_DONE_SIZE            (WM_APP + 3)

static const LPCTSTR INFO_PLUGIN_NAME = TEXT("TVCaptionMod2");
static const LPCTSTR INFO_DESCRIPTION = TEXT("字幕を表示(based on TVCaption 2008-12-16 by odaru) (ver.0.2)");
static const int INFO_VERSION = 2;
static const LPCTSTR TV_CAPTION2_WINDOW_CLASS = TEXT("TVTest TVCaption2");

enum {
    TIMER_ID_DONE_MOVE,
    TIMER_ID_DONE_SIZE,
};

enum {
    ID_COMMAND_SWITCH_LANG,
    ID_COMMAND_SWITCH_SETTING,
};

static const TVTest::CommandInfo COMMAND_LIST[] = {
    {ID_COMMAND_SWITCH_LANG, L"SwitchLang", L"字幕言語切り替え"},
    {ID_COMMAND_SWITCH_SETTING, L"SwitchSetting", L"表示設定切り替え"},
};

CTVCaption2::CTVCaption2()
    : m_settingsIndex(0)
    , m_paintingMethod(0)
    , m_delayTime(0)
    , m_fEnTextColor(false)
    , m_fEnBackColor(false)
    , m_textColor(RGB(0,0,0))
    , m_backColor(RGB(0,0,0))
    , m_textOpacity(0)
    , m_backOpacity(0)
    , m_strokeWidth(0)
    , m_strokeSmoothLevel(0)
    , m_strokeByDilate(0)
    , m_fCentering(false)
    , m_fFixRatio(false)
    , m_hwndPainting(NULL)
    , m_osdShowCount(0)
    , m_fNeedtoShow(false)
    , m_fShowLang2(false)
    , m_fEnCaptionPts(false)
    , m_captionPts(0)
    , m_pCapList(NULL)
    , m_pDrcsList(NULL)
    , m_capCount(0)
    , m_drcsCount(0)
    , m_pcr(0)
    , m_procCapTick(0)
    , m_pcrPid(-1)
    , m_pcrPidsLen(0)
    , m_packetQueueFront(0)
    , m_packetQueueRear(0)
    , m_captionPid(-1)
    , m_hCaptionDll(NULL)
    , m_pfnUnInitializeCP(NULL)
    , m_pfnAddTSPacketCP(NULL)
    , m_pfnClearCP(NULL)
    , m_pfnGetTagInfoCP(NULL)
    , m_pfnGetCaptionDataCPW(NULL)
    , m_pfnDRCSPatternCP(NULL)
    , m_pfnSetGaijiCP(NULL)
{
    m_szIniPath[0] = 0;
    m_szCaptionDllPath[0] = 0;
    m_szFaceName[0] = 0;
    m_szGaijiTableName[0] = 0;
    m_lang1.ucLangTag = 0xFF;
    m_lang2.ucLangTag = 0xFF;
}


bool CTVCaption2::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
    // プラグインの情報を返す
    pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
    pInfo->Flags          = 0;
    pInfo->pszPluginName  = INFO_PLUGIN_NAME;
    pInfo->pszCopyright   = L"Public Domain";
    pInfo->pszDescription = INFO_DESCRIPTION;
    return true;
}


// 初期化処理
bool CTVCaption2::Initialize()
{
    // ウィンドウクラスの登録
    WNDCLASS wc = {0};
    wc.lpfnWndProc = PaintingWndProc;
    wc.hInstance = g_hinstDLL;
    wc.lpszClassName = TV_CAPTION2_WINDOW_CLASS;
    if (!::RegisterClass(&wc)) return false;

    if (!CPseudoOSD::Initialize(g_hinstDLL)) return false;

    if (!::GetModuleFileName(g_hinstDLL, m_szIniPath, _countof(m_szIniPath)) ||
        !::PathRenameExtension(m_szIniPath, TEXT(".ini"))) return false;

    // コマンドを登録
    m_pApp->RegisterCommand(COMMAND_LIST, _countof(COMMAND_LIST));

    // イベントコールバック関数を登録
    m_pApp->SetEventCallback(EventCallback, this);
    return true;
}


// 終了処理
bool CTVCaption2::Finalize()
{
    if (m_pApp->IsPluginEnabled()) EnablePlugin(false);
    return true;
}


// TVTestのフルスクリーンHWNDを取得する
// 必ず取得できると仮定してはいけない
HWND CTVCaption2::GetFullscreenWindow()
{
    TVTest::HostInfo hostInfo;
    if (m_pApp->GetFullscreen() && m_pApp->GetHostInfo(&hostInfo)) {
        TCHAR className[64];
        ::lstrcpyn(className, hostInfo.pszAppName, 48);
        ::lstrcat(className, L" Fullscreen");

        HWND hwnd = NULL;
        while ((hwnd = ::FindWindowEx(NULL, hwnd, className, NULL)) != NULL) {
            DWORD pid;
            ::GetWindowThreadProcessId(hwnd, &pid);
            if (pid == ::GetCurrentProcessId()) return hwnd;
        }
    }
    return NULL;
}


static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    void **params = reinterpret_cast<void**>(lParam);
    TCHAR className[64];
    if (::GetClassName(hwnd, className, _countof(className)) &&
        !::lstrcmp(className, static_cast<LPCTSTR>(params[1])))
    {
        // 見つかった
        *static_cast<HWND*>(params[0]) = hwnd;
        return FALSE;
    }
    return TRUE;
}


// TVTestのVideo Containerウィンドウを探す
// 必ず取得できると仮定してはいけない
HWND CTVCaption2::FindVideoContainer()
{
    HWND hwndFound = NULL;
    TVTest::HostInfo hostInfo;
    if (m_pApp->GetHostInfo(&hostInfo)) {
        TCHAR searchName[64];
        ::lstrcpyn(searchName, hostInfo.pszAppName, 32);
        ::lstrcat(searchName, L" Video Container");

        void *params[2] = { &hwndFound, searchName };
        HWND hwndFull = GetFullscreenWindow();
        ::EnumChildWindows(hwndFull ? hwndFull : m_pApp->GetAppWindow(), EnumWindowsProc, reinterpret_cast<LPARAM>(params));
    }
    return hwndFound;
}


// 字幕PIDを取得する(無い場合は-1)
// プラグインAPIが内部でストリームをロックするので、デッドロックを完成させないように注意
int CTVCaption2::GetSubtitlePid()
{
    int index = m_pApp->GetService();
    TVTest::ServiceInfo si;
    if (index >= 0 && m_pApp->GetServiceInfo(index, &si) && si.SubtitlePID != 0) {
        return si.SubtitlePID;
    }
    return -1;
}


bool CTVCaption2::InitializeCaptionDll()
{
    if (m_hCaptionDll) return true;

    m_hCaptionDll = ::LoadLibrary(m_szCaptionDllPath);
    if (!m_hCaptionDll) return false;

    InitializeCPW *pfnInitializeCPW = reinterpret_cast<InitializeCPW*>(::GetProcAddress(m_hCaptionDll, "InitializeCPW"));
    m_pfnUnInitializeCP = reinterpret_cast<UnInitializeCP*>(::GetProcAddress(m_hCaptionDll, "UnInitializeCP"));
    m_pfnAddTSPacketCP = reinterpret_cast<AddTSPacketCP*>(::GetProcAddress(m_hCaptionDll, "AddTSPacketCP"));
    m_pfnClearCP = reinterpret_cast<ClearCP*>(::GetProcAddress(m_hCaptionDll, "ClearCP"));
    m_pfnGetTagInfoCP = reinterpret_cast<GetTagInfoCP*>(::GetProcAddress(m_hCaptionDll, "GetTagInfoCP"));
    m_pfnGetCaptionDataCPW = reinterpret_cast<GetCaptionDataCPW*>(::GetProcAddress(m_hCaptionDll, "GetCaptionDataCPW"));
    m_pfnDRCSPatternCP = reinterpret_cast<GetDRCSPatternCP*>(::GetProcAddress(m_hCaptionDll, "GetDRCSPatternCP"));
    m_pfnSetGaijiCP = reinterpret_cast<SetGaijiCP*>(::GetProcAddress(m_hCaptionDll, "SetGaijiCP"));
    if (pfnInitializeCPW &&
        m_pfnUnInitializeCP &&
        m_pfnAddTSPacketCP &&
        m_pfnClearCP &&
        m_pfnGetTagInfoCP &&
        m_pfnGetCaptionDataCPW &&
        m_pfnDRCSPatternCP &&
        m_pfnSetGaijiCP &&
        pfnInitializeCPW() == TRUE)
    {
        return true;
    }
    ::FreeLibrary(m_hCaptionDll);
    m_hCaptionDll = NULL;
    return false;
}


bool CTVCaption2::ConfigureGaijiTable(LPCTSTR tableName)
{
    if (!m_hCaptionDll || !tableName[0]) return false;

    // 組み込みのもので外字テーブル設定
    if (!::lstrcmpi(tableName, TEXT("!std"))) {
        // DLLデフォルトにリセット
        DWORD tableSize;
        return m_pfnSetGaijiCP(0, NULL, &tableSize) == TRUE;
    }
    else if (!::lstrcmpi(tableName, TEXT("!typebank"))) {
        DWORD tableSize = _countof(GaijiTable_typebank);
        return m_pfnSetGaijiCP(1, GaijiTable_typebank, &tableSize) == TRUE;
    }

    // ファイルから外字テーブル設定
    TCHAR gaijiPath[MAX_PATH + LF_FACESIZE + 32];
    if (::GetModuleFileName(g_hinstDLL, gaijiPath, MAX_PATH)) {
        ::PathRemoveExtension(gaijiPath);
        ::wsprintf(gaijiPath + ::lstrlen(gaijiPath), TEXT("_Gaiji_%s.txt"), tableName);
        HANDLE hFile = ::CreateFile(gaijiPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            bool fRet = false;
            WCHAR gaijiTable[1/*BOM*/ + G_CELL_SIZE * 7];
            DWORD readBytes;
            if (::ReadFile(hFile, gaijiTable, sizeof(gaijiTable), &readBytes, NULL) &&
                readBytes >= sizeof(WCHAR) && gaijiTable[0] == L'\xFEFF')
            {
                DWORD tableSize = readBytes / sizeof(WCHAR) - 1;
                fRet = m_pfnSetGaijiCP(1, gaijiTable + 1, &tableSize) == TRUE;
            }
            ::CloseHandle(hFile);
            return fRet;
        }
    }
    return false;
}

// プラグインの有効状態が変化した
bool CTVCaption2::EnablePlugin(bool fEnable)
{
    if (fEnable) {
        // 設定の読み込み
        LoadSettings();
        int iniVer = GetPrivateProfileSignedInt(TEXT("Settings"), TEXT("Version"), 0, m_szIniPath);
        if (iniVer < INFO_VERSION) {
            // デフォルトの設定キーを出力するため
            SaveSettings();
        }
        // 字幕描画処理ウィンドウ作成
        m_hwndPainting = ::CreateWindow(TV_CAPTION2_WINDOW_CLASS, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, g_hinstDLL, this);
        if (m_hwndPainting) {
            if (InitializeCaptionDll()) {
                if (!ConfigureGaijiTable(m_szGaijiTableName)) {
                    m_pApp->AddLog(L"外字テーブルの設定に失敗しました。");
                }
                m_captionPid = GetSubtitlePid();
                m_packetQueueFront = m_packetQueueRear;
                m_pcrPid = -1;
                m_pcrPidsLen = 0;

                // コールバックの登録
                m_pApp->SetStreamCallback(0, StreamCallback, this);
                m_pApp->SetWindowMessageCallback(WindowMsgCallback, this);
                return true;
            }
            ::DestroyWindow(m_hwndPainting);
            m_hwndPainting = NULL;
        }
        return false;
    }
    else {
        // コールバックの登録解除
        m_pApp->SetWindowMessageCallback(NULL, NULL);
        m_pApp->SetStreamCallback(TVTest::STREAM_CALLBACK_REMOVE, StreamCallback);

        if (m_hCaptionDll) {
            m_pfnUnInitializeCP();
            ::FreeLibrary(m_hCaptionDll);
            m_hCaptionDll = NULL;
        }
        // 字幕描画ウィンドウの破棄
        if (m_hwndPainting) {
            ::DestroyWindow(m_hwndPainting);
            m_hwndPainting = NULL;
        }
        return true;
    }
}


// 設定の読み込み
void CTVCaption2::LoadSettings()
{
    ::GetPrivateProfileString(TEXT("Settings"), TEXT("CaptionDll"), TEXT("Plugins\\Caption.dll"),
                              m_szCaptionDllPath, _countof(m_szCaptionDllPath), m_szIniPath);
    m_settingsIndex = GetPrivateProfileSignedInt(TEXT("Settings"), TEXT("SettingsIndex"), 0, m_szIniPath);

    // ここからはセクション固有
    TCHAR section[32];
    ::lstrcpy(section, TEXT("Settings"));
    if (m_settingsIndex > 0) {
        ::wsprintf(section + ::lstrlen(section), TEXT("%d"), m_settingsIndex);
    }
    ::GetPrivateProfileString(section, TEXT("FaceName"), TEXT(""), m_szFaceName, _countof(m_szFaceName), m_szIniPath);
    ::GetPrivateProfileString(section, TEXT("GaijiTableName"), TEXT("!std"), m_szGaijiTableName, _countof(m_szGaijiTableName), m_szIniPath);
    m_paintingMethod    = GetPrivateProfileSignedInt(section, TEXT("Method"), 2, m_szIniPath);
    m_delayTime         = GetPrivateProfileSignedInt(section, TEXT("DelayTime"), 450, m_szIniPath);
    int textColor       = GetPrivateProfileSignedInt(section, TEXT("TextColor"), -1, m_szIniPath);
    int backColor       = GetPrivateProfileSignedInt(section, TEXT("BackColor"), -1, m_szIniPath);
    m_textOpacity       = GetPrivateProfileSignedInt(section, TEXT("TextOpacity"), -1, m_szIniPath);
    m_backOpacity       = GetPrivateProfileSignedInt(section, TEXT("BackOpacity"), -1, m_szIniPath);
    m_strokeWidth       = GetPrivateProfileSignedInt(section, TEXT("StrokeWidth"), -5, m_szIniPath);
    m_strokeSmoothLevel = GetPrivateProfileSignedInt(section, TEXT("StrokeSmoothLevel"), 1, m_szIniPath);
    m_strokeByDilate    = GetPrivateProfileSignedInt(section, TEXT("StrokeByDilate"), 22, m_szIniPath);
    m_fCentering        = GetPrivateProfileSignedInt(section, TEXT("Centering"), 0, m_szIniPath) != 0;
    m_fFixRatio         = GetPrivateProfileSignedInt(section, TEXT("FixRatio"), 1, m_szIniPath) != 0;

    m_fEnTextColor = textColor >= 0;
    m_textColor = RGB(textColor/1000000%1000, textColor/1000%1000, textColor%1000);
    m_fEnBackColor = backColor >= 0;
    m_backColor = RGB(backColor/1000000%1000, backColor/1000%1000, backColor%1000);
}


// 設定の保存
void CTVCaption2::SaveSettings() const
{
    WritePrivateProfileInt(TEXT("Settings"), TEXT("Version"), INFO_VERSION, m_szIniPath);
    ::WritePrivateProfileString(TEXT("Settings"), TEXT("CaptionDll"), m_szCaptionDllPath, m_szIniPath);
    WritePrivateProfileInt(TEXT("Settings"), TEXT("SettingsIndex"), m_settingsIndex, m_szIniPath);

    // ここからはセクション固有
    TCHAR section[32];
    ::lstrcpy(section, TEXT("Settings"));
    if (m_settingsIndex > 0) {
        ::wsprintf(section + ::lstrlen(section), TEXT("%d"), m_settingsIndex);
    }
    ::WritePrivateProfileString(section, TEXT("FaceName"), m_szFaceName, m_szIniPath);
    ::WritePrivateProfileString(section, TEXT("GaijiTableName"), m_szGaijiTableName, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("Method"), m_paintingMethod, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("DelayTime"), m_delayTime, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("TextColor"), !m_fEnTextColor ? -1 :
                           GetRValue(m_textColor)*1000000 + GetGValue(m_textColor)*1000 + GetBValue(m_textColor), m_szIniPath);
    WritePrivateProfileInt(section, TEXT("BackColor"), !m_fEnBackColor ? -1 :
                           GetRValue(m_backColor)*1000000 + GetGValue(m_backColor)*1000 + GetBValue(m_backColor), m_szIniPath);
    WritePrivateProfileInt(section, TEXT("TextOpacity"), m_textOpacity, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("BackOpacity"), m_backOpacity, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("StrokeWidth"), m_strokeWidth, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("StrokeSmoothLevel"), m_strokeSmoothLevel, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("StrokeByDilate"), m_strokeByDilate, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("Centering"), m_fCentering, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("FixRatio"), m_fFixRatio, m_szIniPath);
}


// 設定の切り替え
void CTVCaption2::SwitchSettings()
{
    TCHAR section[32];
    ::lstrcpy(section, TEXT("Settings"));
    ::wsprintf(section + ::lstrlen(section), TEXT("%d"), ++m_settingsIndex);

    if (GetPrivateProfileSignedInt(section, TEXT("Method"), -1, m_szIniPath) == -1) {
        m_settingsIndex = 0;
    }
    WritePrivateProfileInt(TEXT("Settings"), TEXT("SettingsIndex"), m_settingsIndex, m_szIniPath);
    LoadSettings();
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CTVCaption2::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
    CTVCaption2 *pThis = static_cast<CTVCaption2*>(pClientData);

    switch (Event) {
    case TVTest::EVENT_PLUGINENABLE:
        // プラグインの有効状態が変化した
        return pThis->EnablePlugin(lParam1 != 0);
    case TVTest::EVENT_FULLSCREENCHANGE:
        // 全画面表示状態が変化した
        if (pThis->m_pApp->IsPluginEnabled()) {
            // オーナーが変わるので破棄する必要がある
            pThis->DestroyOsds();
        }
        break;
    case TVTest::EVENT_SERVICEUPDATE:
        // サービスの構成が変化した
        if (pThis->m_pApp->IsPluginEnabled()) {
            ::SendMessage(pThis->m_hwndPainting, WM_RESET_CAPTION, 0, 0);
            pThis->m_captionPid = pThis->GetSubtitlePid();
        }
        break;
    case TVTest::EVENT_PREVIEWCHANGE:
        // プレビュー表示状態が変化した
        if (pThis->m_pApp->IsPluginEnabled()) {
            if (lParam1 != 0) {
                pThis->m_fNeedtoShow = true;
            }
            else {
                pThis->HideOsds();
                pThis->m_fNeedtoShow = false;
            }
        }
        break;
    case TVTest::EVENT_COMMAND:
        // コマンドが選択された
        if (pThis->m_pApp->IsPluginEnabled()) {
            switch (static_cast<int>(lParam1)) {
            case ID_COMMAND_SWITCH_LANG:
                ::SendMessage(pThis->m_hwndPainting, WM_RESET_CAPTION, 0, 0);
                pThis->m_fShowLang2 = !pThis->m_fShowLang2;
                break;
            case ID_COMMAND_SWITCH_SETTING:
                pThis->HideOsds();
                pThis->SwitchSettings();
                if (!pThis->ConfigureGaijiTable(pThis->m_szGaijiTableName)) {
                    pThis->m_pApp->AddLog(L"外字テーブルの設定に失敗しました。");
                }
                break;
            }
        }
        return TRUE;
    }
    return 0;
}


// ウィンドウメッセージコールバック関数
// TRUEを返すとTVTest側でメッセージを処理しなくなる
// WM_CREATEは呼ばれない
BOOL CALLBACK CTVCaption2::WindowMsgCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult, void *pUserData)
{
    CTVCaption2 *pThis = static_cast<CTVCaption2*>(pUserData);

    switch (uMsg) {
    case WM_MOVE:
        ::SendMessage(pThis->m_hwndPainting, WM_DONE_MOVE, 0, 0);
        ::SetTimer(pThis->m_hwndPainting, TIMER_ID_DONE_MOVE, 500, NULL);
        break;
    case WM_SIZE:
        ::SendMessage(pThis->m_hwndPainting, WM_DONE_SIZE, 0, 0);
        ::SetTimer(pThis->m_hwndPainting, TIMER_ID_DONE_SIZE, 500, NULL);
        break;
    }
    return FALSE;
}


void CTVCaption2::HideOsds()
{
    DEBUG_OUT(TEXT(__FUNCTION__) TEXT("()\n"));

    while (m_osdShowCount > 0) {
        m_osdList[--m_osdShowCount].Hide();
    }
}

void CTVCaption2::DestroyOsds()
{
    DEBUG_OUT(TEXT(__FUNCTION__) TEXT("()\n"));

    for (int i = 0; i < OSD_LIST_MAX; ++i) {
        m_osdList[i].Destroy();
    }
    m_osdShowCount = 0;
}

// 利用可能なOSDを1つだけ用意する
CPseudoOSD &CTVCaption2::CreateOsd(HWND hwndContainer, int charHeight, int nomalHeight, const CAPTION_CHAR_DATA_DLL &style)
{
    CPseudoOSD &osd = m_osdList[m_osdShowCount % OSD_LIST_MAX];
    osd.ClearText();
    osd.SetImage(NULL, 0);

    LOGFONT logFont;
    logFont.lfHeight         = -charHeight;
    logFont.lfWidth          = 0;
    logFont.lfEscapement     = 0;
    logFont.lfOrientation    = 0;
    logFont.lfWeight         = style.bBold ? FW_EXTRABOLD : FW_DONTCARE;
    logFont.lfItalic         = style.bItalic ? TRUE : FALSE;
    logFont.lfUnderline      = style.bUnderLine ? TRUE : FALSE;
    logFont.lfStrikeOut      = 0;
    logFont.lfCharSet        = DEFAULT_CHARSET;
    logFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
    logFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    logFont.lfQuality        = DRAFT_QUALITY;
    logFont.lfPitchAndFamily = (m_szFaceName[0]?DEFAULT_PITCH:FIXED_PITCH) | FF_DONTCARE;
    ::lstrcpy(logFont.lfFaceName, m_szFaceName);
    osd.SetFont(&logFont);

    osd.SetTextColor(m_fEnTextColor ? m_textColor : RGB(style.stCharColor.ucR, style.stCharColor.ucG, style.stCharColor.ucB),
                     m_fEnBackColor ? m_backColor : RGB(style.stBackColor.ucR, style.stBackColor.ucG, style.stBackColor.ucB));

    int textOpacity = style.stCharColor.ucAlpha==0 ? 0 :
                      m_textOpacity>=0 ? min(m_textOpacity,100) : style.stCharColor.ucAlpha*100/255;
    int backOpacity = style.stBackColor.ucAlpha==0 ? 0 :
                      m_backOpacity>=0 ? min(m_backOpacity,100) : style.stBackColor.ucAlpha*100/255;
    osd.SetOpacity(textOpacity, backOpacity);

    osd.SetStroke(m_strokeWidth < 0 ? max(-m_strokeWidth*nomalHeight,1) : m_strokeWidth*72,
                  m_strokeSmoothLevel, charHeight<=m_strokeByDilate ? true : false);
    osd.SetHighlightingBlock((style.bHLC&0x80)!=0, (style.bHLC&0x40)!=0, (style.bHLC&0x20)!=0, (style.bHLC&0x10)!=0);
    osd.Create(hwndContainer, m_paintingMethod==2 ? true : false);
    if (m_osdShowCount < OSD_LIST_MAX) ++m_osdShowCount;
    return osd;
}


// 拡縮後の文字サイズを得る
static void GetCharSize(int *pCharW, int *pCharH, int *pDirW, int *pDirH, const CAPTION_CHAR_DATA_DLL &charData)
{
    int charTransX = 2;
    int charTransY = 2;
    switch (charData.wCharSizeMode) {
    case CP_STR_SMALL:
        charTransX = 1;
        charTransY = 1;
        break;
    case CP_STR_MEDIUM:
        charTransX = 1;
        charTransY = 2;
        break;
    case CP_STR_HIGH_W:
        charTransX = 2;
        charTransY = 4;
        break;
    case CP_STR_WIDTH_W:
        charTransX = 4;
        charTransY = 2;
        break;
    case CP_STR_W:
        charTransX = 4;
        charTransY = 4;
        break;
    }
    if (pCharW) *pCharW = charData.wCharW * charTransX / 2;
    if (pCharH) *pCharH = charData.wCharH * charTransY / 2;
    if (pDirW) *pDirW = (charData.wCharW + charData.wCharHInterval) * charTransX / 2;
    if (pDirH) *pDirH = (charData.wCharH + charData.wCharVInterval) * charTransY / 2;
}


// 字幕本文を1行だけ処理する
void CTVCaption2::ShowCaptionData(const CAPTION_DATA_DLL &caption, const DRCS_PATTERN_DLL *pDrcsList, DWORD drcsCount, HWND hwndContainer)
{
#ifdef DDEBUG_OUT
    DEBUG_OUT(TEXT(__FUNCTION__) TEXT("(): "));
    for (DWORD i = 0; i < caption.dwListCount; ++i) {
        DEBUG_OUT(caption.pstCharList[i].pszDecode);
    }
    DEBUG_OUT(TEXT("\n"));
#endif

    RECT rc;
    if (!hwndContainer || !::GetClientRect(hwndContainer, &rc)) {
        return;
    }

    // 動画のアスペクト比を考慮して描画領域を調整
    TVTest::VideoInfo vi;
    if (m_fFixRatio && m_pApp->GetVideoInfo(&vi) && vi.XAspect!=0 && vi.YAspect!=0) {
        if (vi.XAspect * rc.bottom < rc.right * vi.YAspect) {
            // 描画ウィンドウが動画よりもワイド
            int shrink = (rc.right - rc.bottom * vi.XAspect / vi.YAspect) / 2;
            rc.left += shrink;
            rc.right -= shrink;
        }
        else {
            int shrink = (rc.bottom - rc.right * vi.YAspect / vi.XAspect) / 2;
            rc.top += shrink;
            rc.bottom -= shrink;
        }
    }

    // 字幕プレーンから描画ウィンドウへの変換係数を計算
    double scaleX = 1.0;
    double scaleY = 1.0;
    int offsetX = rc.left;
    int offsetY = rc.top;
    if (caption.wSWFMode==9 || caption.wSWFMode==10) {
        scaleX = (double)(rc.right-rc.left) / 720;
        scaleY = (double)(rc.bottom-rc.top) / 480;
    }
    else {
        scaleX = (double)(rc.right-rc.left) / 960;
        scaleY = (double)(rc.bottom-rc.top) / 540;
    }
    if (m_fCentering) {
        scaleX /= 1.5;
        scaleY /= 1.5;
        offsetX += (rc.right-rc.left) / 6;
    }

    int posX = caption.wPosX;
    int posY = caption.wPosY;

    // DRCS描画のためにOSDを分割するときの、繰り越す文字列
    LPCTSTR pszCarry = NULL;
    // 文字幅が変化するだけなのでOSDをまとめられるときの、繰り越すOSD
    CPseudoOSD *pOsdCarry = NULL;

    for (DWORD i = 0; i < caption.dwListCount; i += pszCarry?0:1) {
        const CAPTION_CHAR_DATA_DLL &charData = caption.pstCharList[i];
        int charW, charH, dirW, dirH;
        GetCharSize(&charW, &charH, &dirW, &dirH, charData);
        int charScaleW = (int)(charW * scaleX);
        int charScaleH = (int)(charH * scaleY);
        int charNormalScaleH = (int)(charData.wCharH * scaleY);

        LPCTSTR pszShow = pszCarry ? pszCarry : charData.pszDecode;
        pszCarry = NULL;

        // 文字列にDRCSが含まれるか調べる
        const DRCS_PATTERN_DLL *pDrcs = NULL;
        if (drcsCount != 0) {
            for (int j = 0; pszShow[j]; ++j) {
                if (0xEC00 <= pszShow[j] && pszShow[j] <= 0xECFF) {
                    for (DWORD k = 0; k < drcsCount; ++k) {
                        if (pDrcsList[k].dwUCS == pszShow[j]) {
                            pDrcs = &pDrcsList[k];
                            if (pDrcsList[k].bmiHeader.biWidth == charW &&
                                pDrcsList[k].bmiHeader.biHeight == charH)
                            {
                                break;
                            }
                        }
                    }
                    // 対応する図形があるかどうかにかかわらず分割する
                    pszCarry = &pszShow[j+1];
                    break;
                }
            }
        }

        TCHAR szTmp[256];
        bool fSameStyle = false;
        if (pszCarry) {
            // 繰り越す
            ::lstrcpyn(szTmp, pszShow, min(static_cast<int>(pszCarry - pszShow), _countof(szTmp)));
            pszShow = szTmp;
        }
        else if (i+1 < caption.dwListCount) {
            const CAPTION_CHAR_DATA_DLL &nextCharData = caption.pstCharList[i+1];
            int nextCharH, nextDirH;
            GetCharSize(NULL, &nextCharH, NULL, &nextDirH, nextCharData);
            // OSDをまとめられるかどうか
            if (charH == nextCharH && dirH == nextDirH &&
                charData.bUnderLine == nextCharData.bUnderLine &&
                charData.bBold == nextCharData.bBold &&
                charData.bItalic == nextCharData.bItalic &&
                charData.bHLC == nextCharData.bHLC &&
                charData.stCharColor == nextCharData.stCharColor &&
                charData.stBackColor == nextCharData.stBackColor)
            {
                fSameStyle = true;
            }
        }

        // 文字列を描画
        if (pOsdCarry) {
            if (pszShow[0]) {
                pOsdCarry->AddText(pszShow, (int)((posX+dirW*::lstrlen(pszShow))*scaleX) - (int)(posX*scaleX), charScaleW/2);
            }
            if (!fSameStyle) {
                pOsdCarry->Show();
                pOsdCarry = NULL;
            }
        }
        else {
            if (pszShow[0]) {
                CPseudoOSD &osd = CreateOsd(hwndContainer, charScaleH, charNormalScaleH, charData);
                osd.SetPosition((int)(posX*scaleX) + offsetX, (int)((posY-dirH+1)*scaleY) + offsetY,
                                (int)((posY+1)*scaleY) - (int)((posY-dirH+1)*scaleY));
                osd.AddText(pszShow, (int)((posX+dirW*::lstrlen(pszShow))*scaleX) - (int)(posX*scaleX), charScaleW/2);
                if (fSameStyle) {
                    pOsdCarry = &osd;
                }
                else {
                    osd.Show();
                }
            }
        }
        posX += dirW * ::lstrlen(pszShow);

        if (pDrcs) {
            // DRCSを描画
            struct {
                BITMAPINFOHEADER bmiHeader;
                RGBQUAD bmiColors[256];
            } bmi = {pDrcs->bmiHeader};
            CLUT_DAT_DLL charC = charData.stCharColor;
            if (m_fEnTextColor) {
                charC.ucB = GetBValue(m_textColor);
                charC.ucG = GetGValue(m_textColor);
                charC.ucR = GetRValue(m_textColor);
            }
            CLUT_DAT_DLL backC = charData.stBackColor;
            if (m_fEnBackColor) {
                backC.ucB = GetBValue(m_backColor);
                backC.ucG = GetGValue(m_backColor);
                backC.ucR = GetRValue(m_backColor);
            }
            bmi.bmiColors[0].rgbBlue  = backC.ucB;
            bmi.bmiColors[0].rgbGreen = backC.ucG;
            bmi.bmiColors[0].rgbRed   = backC.ucR;
            bmi.bmiColors[1].rgbBlue  = (charC.ucB + backC.ucB*2) / 3;
            bmi.bmiColors[1].rgbGreen = (charC.ucG + backC.ucG*2) / 3;
            bmi.bmiColors[1].rgbRed   = (charC.ucR + backC.ucR*2) / 3;
            bmi.bmiColors[2].rgbBlue  = (charC.ucB*2 + backC.ucB) / 3;
            bmi.bmiColors[2].rgbGreen = (charC.ucG*2 + backC.ucG) / 3;
            bmi.bmiColors[2].rgbRed   = (charC.ucR*2 + backC.ucR) / 3;
            bmi.bmiColors[3].rgbBlue  = charC.ucB;
            bmi.bmiColors[3].rgbGreen = charC.ucG;
            bmi.bmiColors[3].rgbRed   = charC.ucR;

            void *pBits;
            HBITMAP hbm = ::CreateDIBSection(NULL, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
            if (hbm) {
                ::SetDIBits(NULL, hbm, 0, bmi.bmiHeader.biHeight, pDrcs->pbBitmap, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);
                CPseudoOSD &osd = CreateOsd(hwndContainer, charScaleH, charNormalScaleH, charData);
                osd.SetPosition((int)(posX*scaleX) + offsetX, (int)((posY-dirH+1)*scaleY) + offsetY,
                                (int)((posY+1)*scaleY) - (int)((posY-dirH+1)*scaleY));
                RECT rc;
                ::SetRect(&rc, (int)((dirW-charW)/2*scaleX), (int)((dirH-charH)/2*scaleY), charScaleW, charScaleH);
                osd.SetImage(hbm, (int)((posX+dirW)*scaleX) - (int)(posX*scaleX), &rc);
                osd.Show();
                posX += dirW;
            }
        }
    }
}


#define MSB(x) ((x) & 0x80000000)

// 字幕描画のウィンドウプロシージャ
LRESULT CALLBACK CTVCaption2::PaintingWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // WM_CREATEのとき不定
    CTVCaption2 *pThis = reinterpret_cast<CTVCaption2*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
    HWND hwndContainer = NULL;

    switch (uMsg) {
    case WM_CREATE:
        {
            LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
            pThis = reinterpret_cast<CTVCaption2*>(pcs->lpCreateParams);
            ::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));

            pThis->m_fEnCaptionPts = false;
            pThis->m_capCount = 0;
            pThis->m_lang1.ucLangTag = 0xFF;
            pThis->m_lang2.ucLangTag = 0xFF;
            pThis->m_fNeedtoShow = true;
            pThis->m_fShowLang2 = false;
        }
        return 0;
    case WM_DESTROY:
        pThis->DestroyOsds();
        return 0;
    case WM_TIMER:
        switch (wParam) {
        case TIMER_ID_DONE_MOVE:
            ::KillTimer(hwnd, TIMER_ID_DONE_MOVE);
            ::SendMessage(hwnd, WM_DONE_MOVE, 0, 0);
            return 0;
        case TIMER_ID_DONE_SIZE:
            ::KillTimer(hwnd, TIMER_ID_DONE_SIZE);
            ::SendMessage(hwnd, WM_DONE_SIZE, 0, 0);
            return 0;
        }
        break;
    case WM_RESET_CAPTION:
        {
            pThis->HideOsds();
            pThis->m_fEnCaptionPts = false;
            pThis->m_capCount = 0;
            CBlockLock lock(&pThis->m_streamLock);
            pThis->m_packetQueueFront = pThis->m_packetQueueRear;
        }
        return 0;
    case WM_PROCESS_CAPTION:
        if (pThis->m_capCount == 0) {
            // 次の字幕文を取得する
            pThis->GetNextCaption(&pThis->m_pCapList, &pThis->m_capCount,
                                  &pThis->m_pDrcsList, &pThis->m_drcsCount);
        }
        if (!pThis->m_fEnCaptionPts) {
            pThis->m_capCount = 0;
        }
        while (pThis->m_capCount != 0) {
            {
                CBlockLock lock(&pThis->m_streamLock);
                // 次の字幕本文の表示タイミングに達したか
                DWORD waitTime = static_cast<DWORD>((min(max(pThis->m_delayTime,0),5000) + pThis->m_pCapList->dwWaitTime) * PCR_PER_MSEC);
                if (!MSB(pThis->m_captionPts + waitTime - pThis->m_pcr)) {
                    break;
                }
            }
            if (pThis->m_fNeedtoShow) {
                // 字幕本文を1つだけ処理
                if (pThis->m_pCapList->bClear) {
                    pThis->HideOsds();
                }
                else {
                    if (!hwndContainer) {
                        hwndContainer = pThis->FindVideoContainer();
                    }
                    pThis->ShowCaptionData(*pThis->m_pCapList, pThis->m_pDrcsList, pThis->m_drcsCount, hwndContainer);
                }
            }
            --pThis->m_capCount;
            ++pThis->m_pCapList;
        }
        return 0;
    case WM_DONE_MOVE:
        for (int i=0; i<pThis->m_osdShowCount; ++i) {
            pThis->m_osdList[i].OnParentMove();
        }
        return 0;
    case WM_DONE_SIZE:
        if (pThis->m_osdShowCount > 0) {
            hwndContainer = pThis->FindVideoContainer();
            RECT rc;
            if (hwndContainer && ::GetClientRect(hwndContainer, &rc)) {
                // とりあえずはみ出ないようにする
                for (int i=0; i<pThis->m_osdShowCount; ++i) {
                    int left, top, width, height;
                    pThis->m_osdList[i].GetPosition(&left, &top, &width, &height);
                    int adjLeft = left+width>=rc.right ? rc.right-width : left;
                    int adjTop = top+height>=rc.bottom ? rc.bottom-height : top;
                    if (adjLeft < 0 || adjTop < 0) {
                        pThis->m_osdList[i].Hide();
                    }
                    else if (left != adjLeft || top != adjTop) {
                        pThis->m_osdList[i].SetPosition(adjLeft, adjTop, height);
                    }
                }
            }
        }
        return 0;
    }
    return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


// キューにある字幕ストリームから次の字幕文を取得する
// キューが空になるか字幕文を得る(*pCapCount!=0)と返る
void CTVCaption2::GetNextCaption(CAPTION_DATA_DLL **ppCapList, DWORD *pCapCount, DRCS_PATTERN_DLL **ppDrcsList, DWORD *pDrcsCount)
{
    int rear;
    {
        CBlockLock lock(&m_streamLock);
        rear = m_packetQueueRear;
    }
    *pCapCount = *pDrcsCount = 0;

    while (m_packetQueueFront != rear) {
        BYTE *pPacket = m_packetQueue[m_packetQueueFront];
        m_packetQueueFront = (m_packetQueueFront+1) % PACKET_QUEUE_SIZE;

        TS_HEADER header;
        extract_ts_header(&header, pPacket);

        // 字幕PTSを取得
        if (header.payload_unit_start_indicator &&
            (header.adaptation_field_control&1)/*1,3*/)
        {
            BYTE *pPayload = pPacket + 4;
            if (header.adaptation_field_control == 3) {
                // アダプテーションに続けてペイロードがある
                ADAPTATION_FIELD adapt;
                extract_adaptation_field(&adapt, pPayload);
                pPayload = adapt.adaptation_field_length >= 0 ? pPayload + adapt.adaptation_field_length + 1 : NULL;
            }
            if (pPayload) {
                int payloadSize = 188 - static_cast<int>(pPayload - pPacket);
                PES_HEADER pesHeader;
                extract_pes_header(&pesHeader, pPayload, payloadSize);
                if (pesHeader.packet_start_code_prefix && pesHeader.pts_dts_flags >= 2) {
                    m_captionPts = (DWORD)pesHeader.pts_45khz;
                    m_fEnCaptionPts = true;
                }
            }
        }

        DWORD ret = m_pfnAddTSPacketCP(pPacket);
        if (!m_fShowLang2 && ret==CP_NO_ERR_CAPTION_1 || m_fShowLang2 && ret==CP_NO_ERR_CAPTION_1+1) {
            // 字幕文データ
            if (m_pfnGetCaptionDataCPW(0, ppCapList, pCapCount) != TRUE) {
                *pCapCount = 0;
            }
            else {
                // DRCS図形データ(あれば)
                if (m_pfnDRCSPatternCP(0, ppDrcsList, pDrcsCount) != TRUE) {
                    *pDrcsCount = 0;
                }
                break;
            }
        }
        else if (ret == CP_CHANGE_VERSION) {
            // 字幕管理データ(未使用)
            m_lang1.ucLangTag = 0xFF;
            m_lang2.ucLangTag = 0xFF;
            LANG_TAG_INFO_DLL *pLangList;
            DWORD langCount;
            if (m_pfnGetTagInfoCP(&pLangList, &langCount) == TRUE) {
                if (langCount >= 1) m_lang1 = pLangList[0];
                if (langCount >= 2) m_lang2 = pLangList[1];
            }
        }
        else if (ret != TRUE && ret != CP_ERR_NEED_NEXT_PACKET && ret != CP_NO_ERR_TAG_INFO &&
                 (ret < CP_NO_ERR_CAPTION_1 || CP_NO_ERR_CAPTION_8 < ret))
        {
            DEBUG_OUT(TEXT(__FUNCTION__) TEXT("(): Error packet!\n"));
        }
    }
}


// ストリームコールバック(別スレッド)
BOOL CALLBACK CTVCaption2::StreamCallback(BYTE *pData, void *pClientData)
{
    CTVCaption2 *pThis = static_cast<CTVCaption2*>(pClientData);
    TS_HEADER header;
    extract_ts_header(&header, pData);

    // Early reject
    if ((header.adaptation_field_control&2)/*2,3*/ ||
        header.pid == pThis->m_captionPid)
    {
        pThis->ProcessPacket(pData);
    }
    return TRUE;
}


void CTVCaption2::ProcessPacket(BYTE *pPacket)
{
    TS_HEADER header;
    extract_ts_header(&header, pPacket);

    // PCRの取得手順はTvtPlayのCTsSenderと同じ
    if ((header.adaptation_field_control&2)/*2,3*/ &&
        !header.transport_error_indicator)
    {
        // アダプテーションフィールドがある
        ADAPTATION_FIELD adapt;
        extract_adaptation_field(&adapt, pPacket + 4);

        if (adapt.pcr_flag) {
            CBlockLock lock(&m_streamLock);

            // 参照PIDが決まっていないとき、最初に3回PCRが出現したPIDを参照PIDとする
            // 参照PIDのPCRが現れることなく5回別のPCRが出現すれば、参照PIDを変更する
            if (header.pid != m_pcrPid) {
                bool fFound = false;
                for (int i = 0; i < m_pcrPidsLen; i++) {
                    if (m_pcrPids[i] == header.pid) {
                        ++m_pcrPidCounts[i];
                        if (m_pcrPid < 0 && m_pcrPidCounts[i] >= 3) {
                            m_pcrPid = header.pid;
                            m_pcr = (DWORD)adapt.pcr_45khz;
                        }
                        else if (m_pcrPidCounts[i] >= 5) {
                            m_pcrPid = header.pid;
                            m_pcr = (DWORD)adapt.pcr_45khz;
                            DEBUG_OUT(TEXT(__FUNCTION__) TEXT("(): PCR-PID changed!\n"));
                            ::SendNotifyMessage(m_hwndPainting, WM_RESET_CAPTION, 0, 0);
                        }
                        fFound = true;
                        break;
                    }
                }
                if (!fFound && m_pcrPidsLen < PCR_PIDS_MAX) {
                    m_pcrPids[m_pcrPidsLen] = header.pid;
                    m_pcrPidCounts[m_pcrPidsLen] = 1;
                    m_pcrPidsLen++;
                }
            }
            // 参照PIDのときはPCRを取得する
            if (header.pid == m_pcrPid) {
                m_pcrPidsLen = 0;
                DWORD pcr = (DWORD)adapt.pcr_45khz;

                // PCRの連続性チェック
                if (MSB(pcr - m_pcr) || pcr - m_pcr >= 1000 * PCR_PER_MSEC) {
                    DEBUG_OUT(TEXT(__FUNCTION__) TEXT("(): Discontinuous packet!\n"));
                    ::SendNotifyMessage(m_hwndPainting, WM_RESET_CAPTION, 0, 0);
                }
                m_pcr = pcr;

                // ある程度呼び出し頻度を抑える(感覚的に遅延を感じなければOK)
                DWORD tick = ::GetTickCount();
                if (tick - m_procCapTick >= 100) {
                    ::SendNotifyMessage(m_hwndPainting, WM_PROCESS_CAPTION, 0, 0);
                    m_procCapTick = tick;
                }
            }
        }
    }

    // 字幕ストリームを取得
    if (header.pid == m_captionPid &&
        !header.transport_scrambling_control &&
        !header.transport_error_indicator)
    {
        CBlockLock lock(&m_streamLock);
        ::memcpy(m_packetQueue[m_packetQueueRear], pPacket, 188);
        m_packetQueueRear = (m_packetQueueRear+1) % PACKET_QUEUE_SIZE;
    }
}


TVTest::CTVTestPlugin *CreatePluginClass()
{
    return new CTVCaption2;
}
