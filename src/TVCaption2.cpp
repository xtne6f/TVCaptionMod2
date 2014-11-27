// TVTestに字幕を表示するプラグイン(based on TVCaption 2008-12-16 by odaru)
// 最終更新: 2012-08-14
// 署名: xt(849fa586809b0d16276cd644c6749503)
#include <Windows.h>
#include <Shlwapi.h>
#include <vector>
#include "Util.h"
#include "PseudoOSD.h"
#include "Caption.h"
#include "CaptionManager.h"
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
static const LPCTSTR INFO_DESCRIPTION = TEXT("字幕を表示 (ver.1.1; based on TVCaption081216 by odaru)");
static const int INFO_VERSION = 7;
static const LPCTSTR TV_CAPTION2_WINDOW_CLASS = TEXT("TVTest TVCaption2");

//半角置換可能文字リスト
static const LPCTSTR HALF_S_LIST = TEXT("　。、・（）「」");
static const LPCTSTR HALF_R_LIST = TEXT(" ｡､･()｢｣");

enum {
    TIMER_ID_DONE_MOVE,
    TIMER_ID_DONE_SIZE,
};

enum {
    ID_COMMAND_SWITCH_LANG,
    ID_COMMAND_SWITCH_SETTING,
    ID_COMMAND_CAPTURE,
    ID_COMMAND_SAVE,
};

static const TVTest::CommandInfo COMMAND_LIST[] = {
    {ID_COMMAND_SWITCH_LANG, L"SwitchLang", L"字幕言語切り替え"},
    {ID_COMMAND_SWITCH_SETTING, L"SwitchSetting", L"表示設定切り替え"},
    {ID_COMMAND_CAPTURE, L"Capture", L"字幕付き画像のコピー"},
    {ID_COMMAND_SAVE, L"Save", L"字幕付き画像の保存"},
};

CTVCaption2::CTVCaption2()
    : m_fTVH264(false)
    , m_settingsIndex(0)
    , m_paintingMethod(0)
    , m_fIgnorePts(false)
    , m_fEnTextColor(false)
    , m_fEnBackColor(false)
    , m_textColor(RGB(0,0,0))
    , m_backColor(RGB(0,0,0))
    , m_textOpacity(0)
    , m_backOpacity(0)
    , m_vertAntiAliasing(0)
    , m_fontSizeAdjust(100)
    , m_strokeWidth(0)
    , m_strokeSmoothLevel(0)
    , m_strokeByDilate(0)
    , m_fCentering(false)
    , m_fFixRatio(false)
    , m_hwndPainting(NULL)
    , m_osdUsedCount(0)
    , m_fNeedtoShow(false)
    , m_pcr(0)
    , m_procCapTick(0)
    , m_fResetPat(false)
    , m_videoPid(-1)
    , m_pcrPid(-1)
    , m_caption1Pid(-1)
    , m_caption2Pid(-1)
{
    m_szIniPath[0] = 0;
    m_szCaptionDllPath[0] = 0;
    m_szCaptureFolder[0] = 0;
    m_szCaptureFileName[0] = 0;
    m_szFaceName[0] = 0;
    m_szGaijiFaceName[0] = 0;
    m_szGaijiTableName[0] = 0;
    m_szRomSoundList[0] = 0;

    for (int index = 0; index < STREAM_MAX; ++index) {
        m_showFlags[index] = 0;
        m_delayTime[index] = 0;
        m_osdUsingCount[index] = 0;
        m_osdShowCount[index] = 0;
    }
    ::memset(&m_pat, 0, sizeof(m_pat));
}

CTVCaption2::~CTVCaption2()
{
    reset_pat(&m_pat);
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

    if (!GetLongModuleFileName(g_hinstDLL, m_szIniPath, _countof(m_szIniPath)) ||
        !::PathRenameExtension(m_szIniPath, TEXT(".ini"))) return false;

    TVTest::HostInfo hostInfo;
    m_fTVH264 = m_pApp->GetHostInfo(&hostInfo) && !::lstrcmp(hostInfo.pszAppName, TEXT("TVH264"));

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


// 映像PIDを取得する(無い場合は-1)
// プラグインAPIが内部でストリームをロックするので、デッドロックを完成させないように注意
int CTVCaption2::GetVideoPid()
{
    int index = m_pApp->GetService();
    TVTest::ServiceInfo si;
    if (index >= 0 && m_pApp->GetServiceInfo(index, &si) && si.VideoPID != 0) {
        return si.VideoPID;
    }
    return -1;
}


#define SKIP_CRLF(p) { if(*(p)==TEXT('\r')) ++(p); if(*(p)) ++(p); }

bool CTVCaption2::ConfigureGaijiTable(LPCTSTR tableName, std::vector<DRCS_PAIR> *pDrcsStrMap, WCHAR (*pCustomTable)[2])
{
    if (!tableName[0]) return false;
    pDrcsStrMap->clear();

    bool fRet = false;
    TCHAR gaijiPath[MAX_PATH + LF_FACESIZE + 32];

    // 組み込みのもので外字テーブル設定
    if (!::lstrcmpi(tableName, TEXT("!std"))) {
        // DLLデフォルトにリセット
        DWORD tableSize;
        fRet = m_captionDll.SetGaiji(0, NULL, &tableSize);
    }
    else if (!::lstrcmpi(tableName, TEXT("!typebank"))) {
        DWORD tableSize = _countof(GaijiTable_typebank);
        fRet = m_captionDll.SetGaiji(1, GaijiTable_typebank, &tableSize);
    }
    // ファイルから外字テーブル設定
    else if (GetLongModuleFileName(g_hinstDLL, gaijiPath, MAX_PATH)) {
        ::PathRemoveExtension(gaijiPath);
        ::wsprintf(gaijiPath + ::lstrlen(gaijiPath), TEXT("_Gaiji_%s.txt"), tableName);
        LPWSTR text = NewReadTextFileToEnd(gaijiPath, FILE_SHARE_READ);
        if (text) {
            // 1行目は外字テーブル
            int len = ::StrCSpn(text, TEXT("\r\n"));
            DWORD tableSize = len;
            fRet = m_captionDll.SetGaiji(1, text, &tableSize);

            // あれば2行目からDRCS→文字列マップ
            LPCTSTR p = text + len;
            SKIP_CRLF(p);
            if (!::StrCmpNI(p, TEXT("[DRCSMap]"), 9)) {
                p += ::StrCSpn(p, TEXT("\r\n"));
                SKIP_CRLF(p);
                while (*p && *p != TEXT('[')) {
                    DRCS_PAIR pair;
                    int i = 0;
                    for (; i < 16 && *p && *(p+1); ++i, p+=2) {
                        TCHAR hex[] = { TEXT('0'), TEXT('x'), *p, *(p+1), 0 };
                        int ret;
                        if (!::StrToIntEx(hex, STIF_SUPPORT_HEX, &ret)) break;
                        pair.md5[i] = static_cast<BYTE>(ret);
                    }
                    int len = ::StrCSpn(p, TEXT("\r\n"));
                    if (i == 16 && *p == TEXT('=')) {
                        ::lstrcpyn(pair.str, p+1, min(len, _countof(pair.str)));
                        pDrcsStrMap->push_back(pair);
                        // 既ソートにしておく
                        std::vector<DRCS_PAIR>::iterator it = pDrcsStrMap->end() - 1;
                        for (; it != pDrcsStrMap->begin() && ::memcmp(it->md5, (it-1)->md5, 16) < 0; --it) {
                            std::swap(*it, *(it-1));
                        }
                    }
                    p += len;
                    SKIP_CRLF(p);
                }
            }
            delete [] text;
        }
    }

    if (fRet && pCustomTable) {
        // 外字をプラグインで置換する場合
        WCHAR gaijiTable[GAIJI_TABLE_SIZE * 2];
        DWORD tableSize = GAIJI_TABLE_SIZE * 2;
        fRet = m_captionDll.GetGaiji(1, gaijiTable, &tableSize);
        if (fRet) {
            for (int i = 0, j = 0; i < GAIJI_TABLE_SIZE; ++i) {
                pCustomTable[i][0] = gaijiTable[j++];
                pCustomTable[i][1] = (pCustomTable[i][0] & 0xFC00) == 0xD800 ? gaijiTable[j++] : 0;
            }
            // U+E000から線形マップ
            WCHAR wc = 0xE000;
            for (int i = 0; i < GAIJI_TABLE_SIZE; ++i, ++wc) {
                gaijiTable[i] = wc;
            }
            tableSize = GAIJI_TABLE_SIZE;
            fRet = m_captionDll.SetGaiji(1, gaijiTable, &tableSize);
        }
    }
    return fRet;
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
        if (m_captionDll.Initialize(m_szCaptionDllPath)) {
            if (!ConfigureGaijiTable(m_szGaijiTableName, &m_drcsStrMap, m_szGaijiFaceName[0]?m_customGaijiTable:NULL)) {
                m_pApp->AddLog(L"外字テーブルの設定に失敗しました。");
                m_drcsStrMap.clear();
                ::memset(m_customGaijiTable, 0, sizeof(m_customGaijiTable));
            }
            m_caption1Manager.SetCaptionDll(&m_captionDll, 0, m_fTVH264);
            m_caption2Manager.SetCaptionDll(&m_captionDll, 1, m_fTVH264);

            // 字幕描画処理ウィンドウ作成
            m_hwndPainting = ::CreateWindow(TV_CAPTION2_WINDOW_CLASS, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, g_hinstDLL, this);
            if (m_hwndPainting) {
                m_videoPid = GetVideoPid();
                m_fResetPat = true;

                // コールバックの登録
                m_pApp->SetStreamCallback(0, StreamCallback, this);
                m_pApp->SetWindowMessageCallback(WindowMsgCallback, this);
                return true;
            }
            m_captionDll.UnInitialize();
        }
        return false;
    }
    else {
        // コールバックの登録解除
        m_pApp->SetWindowMessageCallback(NULL, NULL);
        m_pApp->SetStreamCallback(TVTest::STREAM_CALLBACK_REMOVE, StreamCallback);

        // 字幕描画ウィンドウの破棄
        if (m_hwndPainting) {
            ::DestroyWindow(m_hwndPainting);
            m_hwndPainting = NULL;
        }
        // 内蔵音再生停止
        ::PlaySound(NULL, NULL, 0);

        m_captionDll.UnInitialize();
        return true;
    }
}


// 設定の読み込み
void CTVCaption2::LoadSettings()
{
    ::GetPrivateProfileString(TEXT("Settings"), TEXT("CaptionDll"),
                              m_fTVH264 ? TEXT("H264Plugins\\Caption.dll") : TEXT("Plugins\\Caption.dll"),
                              m_szCaptionDllPath, _countof(m_szCaptionDllPath), m_szIniPath);
    ::GetPrivateProfileString(TEXT("Settings"), TEXT("CaptureFolder"), TEXT(""),
                              m_szCaptureFolder, _countof(m_szCaptureFolder), m_szIniPath);
    ::GetPrivateProfileString(TEXT("Settings"), TEXT("CaptureFileName"), TEXT("Capture"),
                              m_szCaptureFileName, _countof(m_szCaptureFileName), m_szIniPath);
    m_settingsIndex = GetPrivateProfileSignedInt(TEXT("Settings"), TEXT("SettingsIndex"), 0, m_szIniPath);

    // ここからはセクション固有
    TCHAR section[32];
    ::lstrcpy(section, TEXT("Settings"));
    if (m_settingsIndex > 0) {
        ::wsprintf(section + ::lstrlen(section), TEXT("%d"), m_settingsIndex);
    }
    ::GetPrivateProfileString(section, TEXT("FaceName"), TEXT(""), m_szFaceName, _countof(m_szFaceName), m_szIniPath);
    ::GetPrivateProfileString(section, TEXT("GaijiFaceName"), TEXT(""), m_szGaijiFaceName, _countof(m_szGaijiFaceName), m_szIniPath);
    ::GetPrivateProfileString(section, TEXT("GaijiTableName"), TEXT("!std"), m_szGaijiTableName, _countof(m_szGaijiTableName), m_szIniPath);
    m_paintingMethod    = GetPrivateProfileSignedInt(section, TEXT("Method"), 2, m_szIniPath);
    m_showFlags[STREAM_CAPTION]     = GetPrivateProfileSignedInt(section, TEXT("ShowFlags"), 65535, m_szIniPath);
    m_showFlags[STREAM_SUPERIMPOSE] = GetPrivateProfileSignedInt(section, TEXT("ShowFlagsSuper"), 65535, m_szIniPath);
    m_delayTime[STREAM_CAPTION]     = GetPrivateProfileSignedInt(section, TEXT("DelayTime"), 450, m_szIniPath);
    m_delayTime[STREAM_SUPERIMPOSE] = GetPrivateProfileSignedInt(section, TEXT("DelayTimeSuper"), 0, m_szIniPath);
    m_fIgnorePts        = GetPrivateProfileSignedInt(section, TEXT("IgnorePts"), 0, m_szIniPath) != 0;
    int textColor       = GetPrivateProfileSignedInt(section, TEXT("TextColor"), -1, m_szIniPath);
    int backColor       = GetPrivateProfileSignedInt(section, TEXT("BackColor"), -1, m_szIniPath);
    m_textOpacity       = GetPrivateProfileSignedInt(section, TEXT("TextOpacity"), -1, m_szIniPath);
    m_backOpacity       = GetPrivateProfileSignedInt(section, TEXT("BackOpacity"), -1, m_szIniPath);
    m_vertAntiAliasing  = GetPrivateProfileSignedInt(section, TEXT("VertAntiAliasing"), 22, m_szIniPath);
    m_fontSizeAdjust    = GetPrivateProfileSignedInt(section, TEXT("FontSizeAdjust"), 100, m_szIniPath);
    m_strokeWidth       = GetPrivateProfileSignedInt(section, TEXT("StrokeWidth"), -5, m_szIniPath);
    m_strokeSmoothLevel = GetPrivateProfileSignedInt(section, TEXT("StrokeSmoothLevel"), 1, m_szIniPath);
    m_strokeByDilate    = GetPrivateProfileSignedInt(section, TEXT("StrokeByDilate"), 22, m_szIniPath);
    m_fCentering        = GetPrivateProfileSignedInt(section, TEXT("Centering"), 0, m_szIniPath) != 0;
    m_fFixRatio         = GetPrivateProfileSignedInt(section, TEXT("FixRatio"), 1, m_szIniPath) != 0;
    ::GetPrivateProfileString(section, TEXT("RomSoundList"),
                              TEXT(";!SystemExclamation:01:02:03:04:05:06:07:08:09:10:11:12:13:14:15:!SystemAsterisk::"),
                              m_szRomSoundList, _countof(m_szRomSoundList), m_szIniPath);

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
    ::WritePrivateProfileString(TEXT("Settings"), TEXT("CaptureFolder"), m_szCaptureFolder, m_szIniPath);
    ::WritePrivateProfileString(TEXT("Settings"), TEXT("CaptureFileName"), m_szCaptureFileName, m_szIniPath);
    WritePrivateProfileInt(TEXT("Settings"), TEXT("SettingsIndex"), m_settingsIndex, m_szIniPath);

    // ここからはセクション固有
    TCHAR section[32];
    ::lstrcpy(section, TEXT("Settings"));
    if (m_settingsIndex > 0) {
        ::wsprintf(section + ::lstrlen(section), TEXT("%d"), m_settingsIndex);
    }
    ::WritePrivateProfileString(section, TEXT("FaceName"), m_szFaceName, m_szIniPath);
    ::WritePrivateProfileString(section, TEXT("GaijiFaceName"), m_szGaijiFaceName, m_szIniPath);
    ::WritePrivateProfileString(section, TEXT("GaijiTableName"), m_szGaijiTableName, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("Method"), m_paintingMethod, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("ShowFlags"), m_showFlags[STREAM_CAPTION], m_szIniPath);
    WritePrivateProfileInt(section, TEXT("ShowFlagsSuper"), m_showFlags[STREAM_SUPERIMPOSE], m_szIniPath);
    WritePrivateProfileInt(section, TEXT("DelayTime"), m_delayTime[STREAM_CAPTION], m_szIniPath);
    WritePrivateProfileInt(section, TEXT("DelayTimeSuper"), m_delayTime[STREAM_SUPERIMPOSE], m_szIniPath);
    WritePrivateProfileInt(section, TEXT("IgnorePts"), m_fIgnorePts, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("TextColor"), !m_fEnTextColor ? -1 :
                           GetRValue(m_textColor)*1000000 + GetGValue(m_textColor)*1000 + GetBValue(m_textColor), m_szIniPath);
    WritePrivateProfileInt(section, TEXT("BackColor"), !m_fEnBackColor ? -1 :
                           GetRValue(m_backColor)*1000000 + GetGValue(m_backColor)*1000 + GetBValue(m_backColor), m_szIniPath);
    WritePrivateProfileInt(section, TEXT("TextOpacity"), m_textOpacity, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("BackOpacity"), m_backOpacity, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("VertAntiAliasing"), m_vertAntiAliasing, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("FontSizeAdjust"), m_fontSizeAdjust, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("StrokeWidth"), m_strokeWidth, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("StrokeSmoothLevel"), m_strokeSmoothLevel, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("StrokeByDilate"), m_strokeByDilate, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("Centering"), m_fCentering, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("FixRatio"), m_fFixRatio, m_szIniPath);
    ::WritePrivateProfileString(section, TEXT("RomSoundList"), m_szRomSoundList, m_szIniPath);
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


// 内蔵音再生する
static bool PlayRomSound(LPCTSTR list, int index)
{
    if (index<0 || list[0]==TEXT(';')) return false;

    int len = ::StrCSpn(list, TEXT(":"));
    for (; index>0 && list[len]; --index) {
        list += len+1;
        len = ::StrCSpn(list, TEXT(":"));
    }
    if (index>0) return false;

    TCHAR id[32], path[MAX_PATH + 64];
    ::lstrcpyn(id, list, min(len+1,_countof(id)));

    if (id[0]==TEXT('!')) {
        // 定義済みのサウンド
        return ::PlaySound(id+1, NULL, SND_ASYNC|SND_NODEFAULT|SND_ALIAS) != FALSE;
    }
    else if (id[0] && GetLongModuleFileName(g_hinstDLL, path, MAX_PATH)) {
        ::PathRemoveExtension(path);
        ::wsprintf(path + ::lstrlen(path), TEXT("\\%s.wav"), id);
        return ::PlaySound(path, NULL, SND_ASYNC|SND_NODEFAULT|SND_FILENAME) != FALSE;
    }
    return false;
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CTVCaption2::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
    CTVCaption2 *pThis = static_cast<CTVCaption2*>(pClientData);

    switch (Event) {
    case TVTest::EVENT_PLUGINENABLE:
        // プラグインの有効状態が変化した
        if (pThis->EnablePlugin(lParam1 != 0)) {
            PlayRomSound(pThis->m_szRomSoundList, lParam1 != 0 ? 17 : 18);
            return TRUE;
        }
        return 0;
    case TVTest::EVENT_FULLSCREENCHANGE:
        // 全画面表示状態が変化した
        if (pThis->m_pApp->IsPluginEnabled()) {
            // オーナーが変わるので破棄する必要がある
            pThis->DestroyOsds();
            HWND hwndContainer = pThis->FindVideoContainer();
            if (hwndContainer) {
                for (int i = 0; i < OSD_PRE_CREATE_NUM; ++i) {
                    pThis->m_osdList[i].Create(hwndContainer, pThis->m_paintingMethod == 2);
                }
            }
        }
        break;
    case TVTest::EVENT_SERVICECHANGE:
        // サービスが変更された
    case TVTest::EVENT_SERVICEUPDATE:
        // サービスの構成が変化した
        if (pThis->m_pApp->IsPluginEnabled()) {
            ::SendMessage(pThis->m_hwndPainting, WM_RESET_CAPTION, 0, 0);
            pThis->m_videoPid = pThis->GetVideoPid();
            pThis->m_fResetPat = true;
        }
        break;
    case TVTest::EVENT_PREVIEWCHANGE:
        // プレビュー表示状態が変化した
        if (pThis->m_pApp->IsPluginEnabled()) {
            if (lParam1 != 0) {
                pThis->m_fNeedtoShow = true;
            }
            else {
                pThis->HideOsds(STREAM_CAPTION);
                pThis->HideOsds(STREAM_SUPERIMPOSE);
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
                {
                    pThis->m_caption1Manager.SwitchLang();
                    bool fShowLang2 = pThis->m_caption2Manager.SwitchLang();
                    TCHAR str[32];
                    ::wsprintf(str, TEXT("第%d言語に切り替えました。"), fShowLang2 ? 2 : 1);
                    pThis->m_pApp->AddLog(str);
                }
                break;
            case ID_COMMAND_SWITCH_SETTING:
                pThis->HideOsds(STREAM_CAPTION);
                pThis->HideOsds(STREAM_SUPERIMPOSE);
                pThis->SwitchSettings();
                if (!pThis->ConfigureGaijiTable(pThis->m_szGaijiTableName, &pThis->m_drcsStrMap,
                                                pThis->m_szGaijiFaceName[0]?pThis->m_customGaijiTable:NULL))
                {
                    pThis->m_pApp->AddLog(L"外字テーブルの設定に失敗しました。");
                    pThis->m_drcsStrMap.clear();
                    ::memset(pThis->m_customGaijiTable, 0, sizeof(pThis->m_customGaijiTable));
                }
                PlayRomSound(pThis->m_szRomSoundList, 16);
                break;
            case ID_COMMAND_CAPTURE:
                pThis->OnCapture(false);
                break;
            case ID_COMMAND_SAVE:
                pThis->OnCapture(true);
                break;
            }
        }
        return TRUE;
    }
    return 0;
}


void CTVCaption2::OnCapture(bool fSaveToFile)
{
    void *pPackDib = m_pApp->CaptureImage();
    if (!pPackDib) return;

    BITMAPINFOHEADER *pBih = static_cast<BITMAPINFOHEADER*>(pPackDib);
    if (pBih->biWidth>0 && pBih->biHeight>0 && (pBih->biBitCount==24 || pBih->biBitCount==32)) {
        // 常に24bitビットマップにする
        BITMAPINFOHEADER bih = *pBih;
        bih.biBitCount = 24;
        void *pBits;
        HBITMAP hbm = ::CreateDIBSection(NULL, reinterpret_cast<BITMAPINFO*>(&bih), DIB_RGB_COLORS, &pBits, NULL, 0);
        if (hbm) {
            BYTE *pBitmap = static_cast<BYTE*>(pPackDib) + sizeof(BITMAPINFOHEADER);
            if (pBih->biBitCount == 24) {
                ::SetDIBits(NULL, hbm, 0, pBih->biHeight, pBitmap, reinterpret_cast<BITMAPINFO*>(pBih), DIB_RGB_COLORS);
            }
            else {
                // 32-24bit変換
                for (int y=0; y<bih.biHeight; ++y) {
                    BYTE *pDest = static_cast<BYTE*>(pBits) + (bih.biWidth*3+3) / 4 * 4 * y;
                    BYTE *pSrc = pBitmap + bih.biWidth * 4 * y;
                    for (int x=0; x<bih.biWidth; ++x) {
                        ::memcpy(&pDest[3 * x], &pSrc[4 * x], 3);
                    }
                }
            }

            HWND hwndContainer = FindVideoContainer();
            RECT rc;
            if (hwndContainer && ::GetClientRect(hwndContainer, &rc)) {
                TVTest::VideoInfo vi;
                if (m_pApp->GetVideoInfo(&vi) && vi.XAspect!=0 && vi.YAspect!=0) {
                    BITMAPINFOHEADER bihRes = bih;
                    if (vi.XAspect * rc.bottom < rc.right * vi.YAspect) {
                        // 描画ウィンドウが動画よりもワイド
                        bihRes.biWidth = rc.bottom * vi.XAspect / vi.YAspect;
                        bihRes.biHeight = rc.bottom;
                    }
                    else {
                        bihRes.biWidth = rc.right;
                        bihRes.biHeight = rc.right * vi.YAspect / vi.XAspect;
                    }
                    // キャプチャ画像が表示中の動画サイズと異なるときは動画サイズのビットマップに変換する
                    if (bih.biWidth < bihRes.biWidth-1 || bihRes.biWidth+1 < bih.biWidth ||
                        bih.biHeight < bihRes.biHeight-1 || bihRes.biHeight+1 < bih.biHeight)
                    {
                        void *pBitsRes;
                        HBITMAP hbmRes = ::CreateDIBSection(NULL, reinterpret_cast<BITMAPINFO*>(&bihRes), DIB_RGB_COLORS, &pBitsRes, NULL, 0);
                        if (hbmRes) {
                            HDC hdc = ::CreateCompatibleDC(NULL);
                            HBITMAP hbmOld = static_cast<HBITMAP>(::SelectObject(hdc, hbmRes));
                            DrawUtil::DrawBitmap(hdc, 0, 0, bihRes.biWidth, bihRes.biHeight, hbm);
                            ::SelectObject(hdc, hbmOld);
                            ::DeleteDC(hdc);
                            ::DeleteObject(hbm);
                            hbm = hbmRes;
                            bih = bihRes;
                            pBits = pBitsRes;
                        }
                    }
                }

                // ビットマップに表示中のOSDを合成
                // コンテナ中央に動画が表示されていることを仮定
                int offsetLeft, offsetTop;
                if (bih.biWidth * rc.bottom < rc.right * bih.biHeight) {
                    // 描画ウィンドウが動画よりもワイド
                    offsetLeft = (rc.right - rc.bottom * bih.biWidth / bih.biHeight) / 2;
                    offsetTop = 0;
                }
                else {
                    offsetLeft = 0;
                    offsetTop = (rc.bottom - rc.right * bih.biHeight / bih.biWidth) / 2;
                }
                HDC hdc = ::CreateCompatibleDC(NULL);
                HBITMAP hbmOld = static_cast<HBITMAP>(::SelectObject(hdc, hbm));
                for (int i = 0; i < STREAM_MAX; ++i) {
                    for (int j = 0; j < m_osdShowCount[i]; ++j) {
                        int left, top;
                        m_pOsdUsingList[i][j]->GetPosition(&left, &top, NULL, NULL);
                        m_pOsdUsingList[i][j]->Compose(hdc, left-offsetLeft, top-offsetTop);
                    }
                }
                ::SelectObject(hdc, hbmOld);
                ::DeleteDC(hdc);
            }

            ::GdiFlush();
            int sizeImage = (bih.biWidth*3+3) / 4 * 4 * bih.biHeight;
            if (fSaveToFile) {
                // ファイルに保存
                if (::PathIsDirectory(m_szCaptureFolder)) {
                    TCHAR fileName[_countof(m_szCaptureFileName) + 64];
                    SYSTEMTIME st;
                    ::GetLocalTime(&st);
                    ::wsprintf(fileName, TEXT("%s%d%02d%02d-%02d%02d%02d"),
                               m_szCaptureFileName, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
                    TCHAR path[MAX_PATH + 16];
                    if (::PathCombine(path, m_szCaptureFolder, fileName)) {
                        int len = ::lstrlen(path);
                        ::lstrcpy(path+len, TEXT(".bmp"));
                        for (int i = 1; i <= 10 && ::PathFileExists(path); ++i) {
                            ::wsprintf(path+len, TEXT("-%d.bmp"), i);
                        }
                        // ファイルがなければ書きこむ
                        HANDLE hFile = ::CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
                        if (hFile != INVALID_HANDLE_VALUE) {
                            BITMAPFILEHEADER bmfHeader = {0};
                            bmfHeader.bfType = 0x4D42;
                            bmfHeader.bfOffBits = sizeof(bmfHeader) + sizeof(bih);
                            bmfHeader.bfSize = bmfHeader.bfOffBits + sizeImage;
                            DWORD dwWritten;
                            ::WriteFile(hFile, &bmfHeader, sizeof(bmfHeader), &dwWritten, NULL);
                            ::WriteFile(hFile, &bih, sizeof(bih), &dwWritten, NULL);
                            ::WriteFile(hFile, pBits, sizeImage, &dwWritten, NULL);
                            ::CloseHandle(hFile);
                        }
                    }
                }
            }
            else if (::OpenClipboard(m_hwndPainting)) {
                // クリップボードにコピー
                if (::EmptyClipboard()) {
                    HGLOBAL hg = ::GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFOHEADER) + sizeImage);
                    if (hg) {
                        BYTE *pClip = reinterpret_cast<BYTE*>(::GlobalLock(hg));
                        if (pClip) {
                            ::memcpy(pClip, &bih, sizeof(BITMAPINFOHEADER));
                            ::memcpy(pClip + sizeof(BITMAPINFOHEADER), pBits, sizeImage);
                            ::GlobalUnlock(hg);
                            if (!::SetClipboardData(CF_DIB, hg)) ::GlobalFree(hg);
                        }
                        else ::GlobalFree(hg);
                    }
                }
                ::CloseClipboard();
            }
            ::DeleteObject(hbm);
        }
    }
    m_pApp->MemoryFree(pPackDib);
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


void CTVCaption2::HideOsds(STREAM_INDEX index)
{
    DEBUG_OUT(TEXT(__FUNCTION__) TEXT("()\n"));

    while (m_osdShowCount[index] > 0) {
        m_pOsdUsingList[index][--m_osdShowCount[index]]->Hide();
    }
}

void CTVCaption2::DestroyOsds()
{
    DEBUG_OUT(TEXT(__FUNCTION__) TEXT("()\n"));

    for (int i = 0; i < OSD_LIST_MAX; ++i) {
        m_osdList[i].Destroy();
    }
    CPseudoOSD::FreeWorkBitmap();
    m_osdUsedCount = 0;
    m_osdUsingCount[STREAM_CAPTION] = 0;
    m_osdUsingCount[STREAM_SUPERIMPOSE] = 0;
    m_osdShowCount[STREAM_CAPTION] = 0;
    m_osdShowCount[STREAM_SUPERIMPOSE] = 0;
}

static void AddOsdText(CPseudoOSD *pOsd, LPCTSTR text, int width, int charWidth, int charHeight,
                       int fontSizeAdjust, LPCTSTR faceName, const CAPTION_CHAR_DATA_DLL &style)
{
    LOGFONT logFont;
    logFont.lfHeight         = -charHeight * fontSizeAdjust / 100;
    logFont.lfWidth          = charWidth * fontSizeAdjust / 100 / 2;
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
    logFont.lfPitchAndFamily = (faceName[0]?DEFAULT_PITCH:FIXED_PITCH) | FF_DONTCARE;
    ::lstrcpy(logFont.lfFaceName, faceName);
    pOsd->AddText(text, width, logFont);
}

// 利用可能なOSDを1つだけ用意する
CPseudoOSD &CTVCaption2::CreateOsd(STREAM_INDEX index, HWND hwndContainer, int charHeight, int nomalHeight, const CAPTION_CHAR_DATA_DLL &style)
{
    DEBUG_OUT(TEXT(__FUNCTION__) TEXT("()\n"));

    CPseudoOSD *pOsd;
    if (m_osdShowCount[index] >= OSD_LIST_MAX) {
        pOsd = m_pOsdUsingList[index][0];
    }
    else if (m_osdShowCount[index] < m_osdUsingCount[index]) {
        pOsd = m_pOsdUsingList[index][m_osdShowCount[index]++];
    }
    else if (m_osdUsedCount >= OSD_LIST_MAX) {
        pOsd = &m_osdList[0];
    }
    else {
        m_pOsdUsingList[index][m_osdUsingCount[index]++] = &m_osdList[m_osdUsedCount++];
        pOsd = m_pOsdUsingList[index][m_osdShowCount[index]++];
    }
    CPseudoOSD &osd = *pOsd;
    osd.ClearText();
    osd.SetImage(NULL, 0);
    osd.SetTextColor(m_fEnTextColor ? m_textColor : RGB(style.stCharColor.ucR, style.stCharColor.ucG, style.stCharColor.ucB),
                     m_fEnBackColor ? m_backColor : RGB(style.stBackColor.ucR, style.stBackColor.ucG, style.stBackColor.ucB));

    int textOpacity = m_textOpacity>=0 ? min(m_textOpacity,100) : style.stCharColor.ucAlpha*100/255;
    int backOpacity = !m_fTVH264 && style.stBackColor.ucAlpha==0 ? 0 :
                      m_backOpacity>=0 ? min(m_backOpacity,100) : style.stBackColor.ucAlpha*100/255;
    osd.SetOpacity(textOpacity, backOpacity);

    osd.SetStroke(m_strokeWidth < 0 ? max(-m_strokeWidth*nomalHeight,1) : m_strokeWidth*72,
                  m_strokeSmoothLevel, charHeight<=m_strokeByDilate ? true : false);
    osd.SetHighlightingBlock((style.bHLC&0x80)!=0, (style.bHLC&0x40)!=0, (style.bHLC&0x20)!=0, (style.bHLC&0x10)!=0);
    osd.SetVerticalAntiAliasing(charHeight<=m_vertAntiAliasing ? false : true);
    osd.SetFlashingInterval(style.bFlushMode==1 ? 500 : style.bFlushMode==2 ? -500 : 0);
    osd.Create(hwndContainer, m_paintingMethod==2 ? true : false);
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
void CTVCaption2::ShowCaptionData(STREAM_INDEX index, const CAPTION_DATA_DLL &caption, const DRCS_PATTERN_DLL *pDrcsList, DWORD drcsCount, HWND hwndContainer)
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
    if (caption.wSWFMode==14) {
        // Cプロファイル
        scaleX = (double)(rc.right-rc.left) / (320+20/2);
        scaleY = (double)(rc.bottom-rc.top) / 180;
        offsetY += (int)((180-24*3)*scaleY);
    }
    else if (caption.wSWFMode==9 || caption.wSWFMode==10) {
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

    // DRCSか外字の描画後に繰り越す文字列
    LPCTSTR pszCarry = NULL;
    // スタイルが同じなのでOSDをまとめられるときの、繰り越すOSD
    CPseudoOSD *pOsdCarry = NULL;

    for (DWORD i = 0; i < caption.dwListCount; i += pszCarry?0:1) {
        const CAPTION_CHAR_DATA_DLL &charData = caption.pstCharList[i];
        int charW, charH, dirW, dirH;
        GetCharSize(&charW, &charH, &dirW, &dirH, charData);
        int charScaleW = (int)(charW * scaleX);
        int charScaleH = (int)(charH * scaleY);
        int charNormalScaleW = (int)(charData.wCharW * scaleX);
        int charNormalScaleH = (int)(charData.wCharH * scaleY);

        LPCTSTR pszShow = pszCarry ? pszCarry : charData.pszDecode;
        pszCarry = NULL;

        // 文字列にDRCSか外字か半角置換可能文字が含まれるか調べる
        const DRCS_PATTERN_DLL *pDrcs = NULL;
        LPCTSTR pszDrcsStr = NULL;
        bool fSearchGaiji = m_szGaijiFaceName[0] != 0;
        WCHAR szGaiji[3] = {0};
        bool fSearchHalf = charData.wCharSizeMode == CP_STR_MEDIUM;
        WCHAR szHalf[2] = {0};
        if (drcsCount != 0 || fSearchGaiji || fSearchHalf) {
            for (int j = 0; pszShow[j]; ++j) {
                if (0xEC00 <= pszShow[j] && pszShow[j] <= 0xECFF) {
                    // DRCS
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
                    if (pDrcs) {
                        // もしあれば置きかえ可能な文字列を取得
                        BYTE md5[16];
                        if (!m_drcsStrMap.empty() && CalcMD5FromDRCSPattern(md5, pDrcs)) {
                            // 2分探索
                            int lo = 0;
                            int hi = (int)m_drcsStrMap.size() - 1;
                            do {
                                int k = (lo + hi) / 2;
                                int cmp = ::memcmp(m_drcsStrMap[k].md5, md5, 16);
                                if (!cmp) {
                                    pszDrcsStr = m_drcsStrMap[k].str;
                                    break;
                                }
                                else if (cmp > 0) hi = k - 1;
                                else lo = k + 1;
                            } while (lo <= hi);
                        }
                        pszCarry = &pszShow[j+1];
                        break;
                    }
                }
                else if (fSearchGaiji && 0xE000 <= pszShow[j] && pszShow[j] < 0xE000+GAIJI_TABLE_SIZE) {
                    // 外字
                    szGaiji[0] = m_customGaijiTable[pszShow[j] - 0xE000][0];
                    szGaiji[1] = m_customGaijiTable[pszShow[j] - 0xE000][1];
                    szGaiji[2] = 0;
                    if (szGaiji[0]) {
                        pszCarry = &pszShow[j+1];
                        break;
                    }
                }
                else if (fSearchHalf) {
                    LPCTSTR p = ::StrChr(HALF_S_LIST, pszShow[j]);
                    if (p) {
                        // 半角置換可能文字
                        szHalf[0] = HALF_R_LIST[p-HALF_S_LIST];
                        szHalf[1] = 0;
                        pszCarry = &pszShow[j+1];
                        break;
                    }
                }
            }
        }

        TCHAR szTmp[256];
        bool fSameStyle = false;
        if (pszCarry) {
            // 繰り越す
            ::lstrcpyn(szTmp, pszShow, min(static_cast<int>(pszCarry - pszShow), _countof(szTmp)));
            pszShow = szTmp;
            if ((pDrcs && pszDrcsStr) || szGaiji[0] || szHalf[0]) {
                fSameStyle = true;
            }
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
                charData.bFlushMode == nextCharData.bFlushMode &&
                charData.bHLC == nextCharData.bHLC &&
                charData.stCharColor == nextCharData.stCharColor &&
                charData.stBackColor == nextCharData.stBackColor)
            {
                fSameStyle = true;
            }
        }

        // 文字列を描画
        int lenWos = StrlenWoLoSurrogate(pszShow);
        if (pOsdCarry) {
            AddOsdText(pOsdCarry, pszShow, (int)((posX+dirW*lenWos)*scaleX) - (int)(posX*scaleX),
                       charScaleW, charScaleH, m_fontSizeAdjust, m_szFaceName, charData);
            if (!fSameStyle) {
                pOsdCarry->PrepareWindow();
                pOsdCarry = NULL;
            }
        }
        else {
            if (fSameStyle || lenWos > 0) {
                CPseudoOSD &osd = CreateOsd(index, hwndContainer, charScaleH, charNormalScaleH, charData);
                osd.SetPosition((int)(posX*scaleX) + offsetX, (int)((posY-dirH+1)*scaleY) + offsetY,
                                (int)((posY+1)*scaleY) - (int)((posY-dirH+1)*scaleY));
                AddOsdText(&osd, pszShow, (int)((posX+dirW*lenWos)*scaleX) - (int)(posX*scaleX),
                           charScaleW, charScaleH, m_fontSizeAdjust, m_szFaceName, charData);
                if (fSameStyle) {
                    pOsdCarry = &osd;
                }
                else {
                    osd.PrepareWindow();
                }
            }
        }
        posX += dirW * lenWos;

        if (pDrcs && !pszDrcsStr) {
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
            // 残りのテーブルは被らない色にしておく
            for (int j=4; j<16; ++j) {
                bmi.bmiColors[j].rgbGreen = 15;
            }

            void *pBits;
            HBITMAP hbm = ::CreateDIBSection(NULL, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
            if (hbm) {
                ::SetDIBits(NULL, hbm, 0, bmi.bmiHeader.biHeight, pDrcs->pbBitmap, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);
                CPseudoOSD &osd = CreateOsd(index, hwndContainer, charScaleH, charNormalScaleH, charData);
                osd.SetPosition((int)(posX*scaleX) + offsetX, (int)((posY-dirH+1)*scaleY) + offsetY,
                                (int)((posY+1)*scaleY) - (int)((posY-dirH+1)*scaleY));
                RECT rc;
                ::SetRect(&rc, (int)((dirW-charW)/2*scaleX), (int)((dirH-charH)/2*scaleY), charScaleW, charScaleH);
                osd.SetImage(hbm, (int)((posX+dirW)*scaleX) - (int)(posX*scaleX), &rc);
                osd.PrepareWindow();
                posX += dirW;
            }
        }
        else if (pDrcs && pszDrcsStr) {
            // DRCSを文字列で描画
            int lenWos = StrlenWoLoSurrogate(pszDrcsStr);
            if (pOsdCarry && lenWos > 0) {
                // レイアウト維持のため、何文字であっても1文字幅に詰める
                AddOsdText(pOsdCarry, pszDrcsStr, (int)((posX+dirW)*scaleX) - (int)(posX*scaleX),
                           charScaleW / lenWos + 1,
                           charScaleH, m_fontSizeAdjust, m_szGaijiFaceName[0] ? m_szGaijiFaceName : m_szFaceName, charData);
                posX += dirW;
            }
        }
        else if (szGaiji[0]) {
            // 外字を描画
            if (pOsdCarry) {
                AddOsdText(pOsdCarry, szGaiji, (int)((posX+dirW)*scaleX) - (int)(posX*scaleX),
                           charScaleW, charScaleH, m_fontSizeAdjust, m_szGaijiFaceName, charData);
                posX += dirW;
            }
        }
        else if (szHalf[0]) {
            // 半角文字を描画
            if (pOsdCarry) {
                AddOsdText(pOsdCarry, szHalf, (int)((posX+dirW)*scaleX) - (int)(posX*scaleX),
                           -charNormalScaleW, charScaleH, m_fontSizeAdjust, m_szFaceName, charData);
                posX += dirW;
            }
        }

        if (charData.bPRA != 0) {
            PlayRomSound(m_szRomSoundList, charData.bPRA - 1);
        }
    }
}


#define MSB(x) ((x) & 0x80000000)

// 字幕描画のウィンドウプロシージャ
LRESULT CALLBACK CTVCaption2::PaintingWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // WM_CREATEのとき不定
    CTVCaption2 *pThis = reinterpret_cast<CTVCaption2*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (uMsg) {
    case WM_CREATE:
        {
            LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
            pThis = reinterpret_cast<CTVCaption2*>(pcs->lpCreateParams);
            ::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));

            HWND hwndContainer = pThis->FindVideoContainer();
            if (hwndContainer) {
                for (int i = 0; i < OSD_PRE_CREATE_NUM; ++i) {
                    pThis->m_osdList[i].Create(hwndContainer, pThis->m_paintingMethod == 2);
                }
            }
            pThis->m_fNeedtoShow = pThis->m_pApp->GetPreview();
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
        pThis->HideOsds(STREAM_CAPTION);
        pThis->HideOsds(STREAM_SUPERIMPOSE);
        {
            CBlockLock lock(&pThis->m_streamLock);
            pThis->m_caption1Manager.Clear();
            pThis->m_caption2Manager.Clear();
        }
        return 0;
    case WM_PROCESS_CAPTION:
        pThis->ProcessCaption(&pThis->m_caption1Manager);
        pThis->ProcessCaption(&pThis->m_caption2Manager);
        return 0;
    case WM_DONE_MOVE:
        for (int i = 0; i < pThis->m_osdShowCount[STREAM_CAPTION]; ++i) {
            pThis->m_pOsdUsingList[STREAM_CAPTION][i]->OnParentMove();
        }
        for (int i = 0; i < pThis->m_osdShowCount[STREAM_SUPERIMPOSE]; ++i) {
            pThis->m_pOsdUsingList[STREAM_SUPERIMPOSE][i]->OnParentMove();
        }
        return 0;
    case WM_DONE_SIZE:
        pThis->OnSize(STREAM_CAPTION);
        pThis->OnSize(STREAM_SUPERIMPOSE);
        return 0;
    }
    return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


void CTVCaption2::ProcessCaption(CCaptionManager *pCaptionManager)
{
    STREAM_INDEX index;
    DWORD pcr;
    {
        CBlockLock lock(&m_streamLock);
        if (pCaptionManager->IsEmpty()) {
            // 次の字幕文を取得する
            pCaptionManager->Analyze(m_pcr);
            if (pCaptionManager->IsEmpty()) {
                return;
            }
            // 字幕か文字スーパーのどちらかが取得できた
        }
        index = pCaptionManager->IsSuperimpose() ? STREAM_SUPERIMPOSE : STREAM_CAPTION;
        if (m_delayTime[index] < 0) {
            pcr = m_pcr + static_cast<DWORD>(-max(m_delayTime[index],-5000) * PCR_PER_MSEC);
        }
        else {
            pcr = m_pcr - static_cast<DWORD>(min(m_delayTime[index],5000) * PCR_PER_MSEC);
        }
    }

    HWND hwndContainer = NULL;
    int lastShowCount = m_osdShowCount[index];
    for (;;) {
        // 表示タイミングに達した字幕本文を1つだけ取得する
        const CAPTION_DATA_DLL *pCaption = pCaptionManager->PopCaption(pcr, m_fIgnorePts);
        if (!pCaption) {
            break;
        }
        const LANG_TAG_INFO_DLL *pLangTag = pCaptionManager->GetLangTag();
        if (m_fNeedtoShow && pLangTag && ((m_showFlags[index]>>pLangTag->ucDMF)&1)) {
            if (pCaption->bClear) {
                HideOsds(index);
                lastShowCount = 0;
            }
            else {
                if (!hwndContainer) {
                    hwndContainer = FindVideoContainer();
                }
                const DRCS_PATTERN_DLL *pDrcsList;
                DWORD drcsCount;
                pCaptionManager->GetDrcsList(&pDrcsList, &drcsCount);
                ShowCaptionData(index, *pCaption, pDrcsList, drcsCount, hwndContainer);
            }
        }
    }
    // ちらつき防止のため表示処理をまとめる
    while (lastShowCount < m_osdShowCount[index]) {
        m_pOsdUsingList[index][lastShowCount++]->Show();
    }
}


void CTVCaption2::OnSize(STREAM_INDEX index)
{
    if (m_osdShowCount[index] > 0) {
        HWND hwndContainer = FindVideoContainer();
        RECT rc;
        if (hwndContainer && ::GetClientRect(hwndContainer, &rc)) {
            // とりあえずはみ出ないようにする
            for (int i = 0; i < m_osdShowCount[index]; ++i) {
                int left, top, width, height;
                m_pOsdUsingList[index][i]->GetPosition(&left, &top, &width, &height);
                int adjLeft = left+width>=rc.right ? rc.right-width : left;
                int adjTop = top+height>=rc.bottom ? rc.bottom-height : top;
                if (adjLeft < 0 || adjTop < 0) {
                    m_pOsdUsingList[index][i]->Hide();
                }
                else if (left != adjLeft || top != adjTop) {
                    m_pOsdUsingList[index][i]->SetPosition(adjLeft, adjTop, height);
                }
            }
        }
    }
}


// ストリームコールバック(別スレッド)
BOOL CALLBACK CTVCaption2::StreamCallback(BYTE *pData, void *pClientData)
{
    static_cast<CTVCaption2*>(pClientData)->ProcessPacket(pData);
    return TRUE;
}


static void GetPidsFromVideoPmt(int *pPcrPid, int *pCaption1Pid, int *pCaption2Pid, int videoPid, const PAT *pPat)
{
    for (int i = 0; i < pPat->pid_count; ++i) {
        const PMT *pPmt = pPat->pmt[i];
        // このPMTに映像PIDが含まれるか調べる
        bool fVideoPmt = false;
        int privPid[2] = {-1, -1};
        for (int j = 0; j < pPmt->pid_count; ++j) {
            if (pPmt->stream_type[j] == PES_PRIVATE_DATA) {
                int ct = pPmt->component_tag[j];
                if (ct == 0x30 || ct == 0x87) {
                    privPid[0] = pPmt->pid[j];
                }
                else if (ct == 0x38 || ct == 0x88) {
                    privPid[1] = pPmt->pid[j];
                }
            }
            else if (pPmt->pid[j] == videoPid) {
                fVideoPmt = true;
            }
        }
        if (fVideoPmt) {
            *pPcrPid = pPmt->pcr_pid;
            *pCaption1Pid = privPid[0];
            *pCaption2Pid = privPid[1];
            return;
        }
    }
    *pPcrPid = -1;
    *pCaption1Pid = -1;
    *pCaption2Pid = -1;
}


void CTVCaption2::ProcessPacket(BYTE *pPacket)
{
    if (m_fResetPat) {
        reset_pat(&m_pat);
        m_pcrPid = -1;
        m_caption1Pid = -1;
        m_caption2Pid = -1;
        m_fResetPat = false;
    }

    TS_HEADER header;
    extract_ts_header(&header, pPacket);

    // PCRを取得
    if ((header.adaptation_field_control&2)/*2,3*/ &&
        !header.transport_error_indicator)
    {
        // アダプテーションフィールドがある
        ADAPTATION_FIELD adapt;
        extract_adaptation_field(&adapt, pPacket + 4);

        if (adapt.pcr_flag) {
            // 参照PIDのときはPCRを取得する
            if (header.pid == m_pcrPid) {
                CBlockLock lock(&m_streamLock);
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

    // 字幕か文字スーパーとPCRのPIDを取得
    if ((header.adaptation_field_control&1)/*1,3*/ &&
        !header.transport_scrambling_control &&
        !header.transport_error_indicator)
    {
        LPCBYTE pPayload = pPacket + 4;
        if (header.adaptation_field_control == 3) {
            // アダプテーションに続けてペイロードがある
            ADAPTATION_FIELD adapt;
            extract_adaptation_field(&adapt, pPayload);
            if (adapt.adaptation_field_length < 0) return;
            pPayload += adapt.adaptation_field_length + 1;
        }
        int payloadSize = 188 - static_cast<int>(pPayload - pPacket);

        // PAT監視
        if (header.pid == 0) {
            extract_pat(&m_pat, pPayload, payloadSize,
                        header.payload_unit_start_indicator,
                        header.continuity_counter);
            GetPidsFromVideoPmt(&m_pcrPid, &m_caption1Pid, &m_caption2Pid, m_videoPid, &m_pat);
            return;
        }
        // PATリストにあるPMT監視
        for (int i = 0; i < m_pat.pid_count; ++i) {
            if (header.pid == m_pat.pid[i]/* && header.pid != 0*/) {
                PMT *pPmt = m_pat.pmt[i];
                extract_pmt(pPmt, pPayload, payloadSize,
                            header.payload_unit_start_indicator,
                            header.continuity_counter);
                GetPidsFromVideoPmt(&m_pcrPid, &m_caption1Pid, &m_caption2Pid, m_videoPid, &m_pat);
                return;
            }
        }
    }

    // 字幕か文字スーパーのストリームを取得
    if (!header.transport_scrambling_control &&
        !header.transport_error_indicator)
    {
        if (header.pid == m_caption1Pid) {
            CBlockLock lock(&m_streamLock);
            m_caption1Manager.AddPacket(pPacket);
        }
        else if (header.pid == m_caption2Pid) {
            CBlockLock lock(&m_streamLock);
            m_caption2Manager.AddPacket(pPacket);
        }
    }
}


TVTest::CTVTestPlugin *CreatePluginClass()
{
    return new CTVCaption2;
}
