// TVTestに字幕を表示するプラグイン(based on TVCaption 2008-12-16 by odaru)
// 最終更新: 2017-04-28
// 署名: xt(849fa586809b0d16276cd644c6749503)
#include <Windows.h>
#include <Shlwapi.h>
#include <vector>
#include <algorithm>
#include "Util.h"
#include "PseudoOSD.h"
#include "Caption.h"
#include "CaptionManager.h"
#include "OsdCompositor.h"
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#define TVTEST_PLUGIN_VERSION TVTEST_PLUGIN_VERSION_(0,0,14)
#include "TVTestPlugin.h"
#include "resource.h"
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
#define WM_RESET_OSDS           (WM_APP + 4)

static const LPCTSTR INFO_PLUGIN_NAME = TEXT("TVCaptionMod2");
static const LPCTSTR INFO_DESCRIPTION = TEXT("字幕を表示 (ver.2.1; based on TVCaption081216 by odaru)");
static const int INFO_VERSION = 12;
static const LPCTSTR TV_CAPTION2_WINDOW_CLASS = TEXT("TVTest TVCaption2");

// テスト字幕
static const CAPTION_CHAR_DATA_DLL TESTCAP_CHAR_LIST[] = {
    {NULL,CP_STR_NORMAL,{255,255,255,255},{0,0,0,128},{0,0,0,0},0,0,0,0,0,0,36,36,4,24,0,0}, // 外字例示用
    {L"まさ",CP_STR_SMALL,{255,255,0,255},{0,0,0,128},{0,0,0,0},0,0,0,0,0,0,36,36,4,24,0,0},
    {L"決めた",CP_STR_NORMAL,{255,255,0,255},{0,0,0,128},{0,0,0,0},0,0,0,0,0,0,36,36,4,24,0,0},
    {L"　",CP_STR_MEDIUM,{255,255,0,255},{0,0,0,128},{0,0,0,0},0,0,0,0,0,0,36,36,4,24,0,0},
    {L"みんなで正の湯に行こう",CP_STR_NORMAL,{255,255,0,255},{0,0,0,128},{0,0,0,0},0,0,0,0,0,0,36,36,4,24,0,0},
    {L"お～",CP_STR_NORMAL,{0,255,255,255},{0,0,0,128},{0,0,0,0},0,0,0,0,0,0,36,36,4,24,0,0},
    {L"ッ！ｘ６",CP_STR_MEDIUM,{0,255,255,255},{0,0,0,128},{0,0,0,0},0,0,0,0,0,0,36,36,4,24,0,0},
};
static const CAPTION_DATA_DLL TESTCAP_LIST[] = {
    {FALSE,7,170,30,620,480,170,359,0,1,NULL,0}, // 外字例示用
    {FALSE,7,170,30,620,480,510,389,0,1,const_cast<CAPTION_CHAR_DATA_DLL*>(&TESTCAP_CHAR_LIST[1]),0},
    {FALSE,7,170,30,620,480,210,449,0,3,const_cast<CAPTION_CHAR_DATA_DLL*>(&TESTCAP_CHAR_LIST[2]),0},
    {FALSE,7,170,30,620,480,310,509,0,2,const_cast<CAPTION_CHAR_DATA_DLL*>(&TESTCAP_CHAR_LIST[5]),0},
};

static const LPCTSTR ROMSOUND_EXAMPLE = TEXT(";!SystemExclamation:01:02:03:04:05:06:07:08:09:10:11:12:13:14:15:!SystemAsterisk::");
static const LPCTSTR ROMSOUND_ENABLED = TEXT("00:01:02:03:04:05:06:07:08:09:10:11:12:13:14:15:16:17:18");

// 半角置換可能文字リスト
// 記号はJISX0213 1面1区のうちグリフが用意されている可能性が十分高そうなものだけ
static const LPCTSTR HALF_F_LIST = TEXT("　、。，．・：；？！＾＿／｜（［］｛｝「＋－＝＜＞＄％＃＆＊＠０Ａａ");
static const LPCTSTR HALF_T_LIST = TEXT("　、。，．・：；？！＾＿／｜）［］｛｝」＋－＝＜＞＄％＃＆＊＠９Ｚｚ");
static const LPCTSTR HALF_R_LIST = TEXT(" ､｡,.･:;?!^_/|([]{}｢+-=<>$%#&*@0Aa");

enum {
    TIMER_ID_DONE_MOVE,
    TIMER_ID_DONE_SIZE,
    TIMER_ID_FLASHING_TEXTURE,
};

enum {
    ID_COMMAND_SWITCH_LANG,
    ID_COMMAND_SWITCH_SETTING,
    ID_COMMAND_CAPTURE,
    ID_COMMAND_SAVE,
};

CTVCaption2::CTVCaption2()
    : m_settingsIndex(0)
    , m_paintingMethod(0)
    , m_fIgnorePts(false)
    , m_fEnTextColor(false)
    , m_fEnBackColor(false)
    , m_textColor(RGB(0,0,0))
    , m_backColor(RGB(0,0,0))
    , m_textOpacity(0)
    , m_backOpacity(0)
    , m_vertAntiAliasing(0)
    , m_strokeWidth(0)
    , m_strokeSmoothLevel(0)
    , m_strokeByDilate(0)
    , m_paddingWidth(0)
    , m_avoidHalfFlags(0)
    , m_fIgnoreSmall(false)
    , m_fShiftSmall(false)
    , m_fCentering(false)
    , m_fShrinkSDScale(false)
    , m_adjustViewX(0)
    , m_adjustViewY(0)
    , m_hwndPainting(NULL)
    , m_hwndContainer(NULL)
    , m_fNeedtoShow(false)
    , m_fFlashingFlipFlop(false)
    , m_fProfileC(false)
    , m_pcr(0)
    , m_procCapTick(0)
    , m_fResetPat(false)
    , m_videoPid(-1)
    , m_pcrPid(-1)
    , m_caption1Pid(-1)
    , m_caption2Pid(-1)
{
    m_szIniPath[0] = 0;
    m_szCaptureFolder[0] = 0;
    m_szCaptureFileName[0] = 0;
    m_szFaceName[0] = 0;
    m_szGaijiFaceName[0] = 0;
    m_szGaijiTableName[0] = 0;
    m_szRomSoundList[0] = 0;
    RECT rcZero = {};
    m_rcAdjust = rcZero;
    m_rcGaijiAdjust = rcZero;

    for (int index = 0; index < STREAM_MAX; ++index) {
        m_showFlags[index] = 0;
        m_delayTime[index] = 0;
        m_osdShowCount[index] = 0;
        m_osdPrepareCount[index] = 0;
        m_fOsdClear[index] = false;
    }
    PAT zeroPat = {};
    m_pat = zeroPat;
}

CTVCaption2::~CTVCaption2()
{
}


bool CTVCaption2::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
    // プラグインの情報を返す
    pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
    pInfo->Flags          = TVTest::PLUGIN_FLAG_HASSETTINGS;
    pInfo->pszPluginName  = INFO_PLUGIN_NAME;
    pInfo->pszCopyright   = L"Public Domain";
    pInfo->pszDescription = INFO_DESCRIPTION;
    return true;
}


// 初期化処理
bool CTVCaption2::Initialize()
{
    // ウィンドウクラスの登録
    WNDCLASS wc = {};
    wc.lpfnWndProc = PaintingWndProc;
    wc.hInstance = g_hinstDLL;
    wc.lpszClassName = TV_CAPTION2_WINDOW_CLASS;
    if (!::RegisterClass(&wc)) return false;

    if (!CPseudoOSD::Initialize(g_hinstDLL)) return false;

    if (!GetLongModuleFileName(g_hinstDLL, m_szIniPath, _countof(m_szIniPath)) ||
        !::PathRenameExtension(m_szIniPath, TEXT(".ini"))) return false;

    // 必要ならOsdCompositorを初期化(プラグインの有効無効とは独立)
    UINT EnOsdCompositor = ::GetPrivateProfileInt(TEXT("Settings"), TEXT("EnOsdCompositor"), 99, m_szIniPath);
    if (EnOsdCompositor == 99) {
        ::WritePrivateProfileString(TEXT("Settings"), TEXT("EnOsdCompositor"), TEXT("0"), m_szIniPath);
    }
    else if (EnOsdCompositor != 0) {
        // フィルタグラフを取得できないバージョンではAPIフックを使う
        bool fSetHook = m_pApp->GetVersion() < TVTest::MakeVersion(0, 9, 0);
        if (m_osdCompositor.Initialize(fSetHook)) {
            m_pApp->AddLog(L"OsdCompositorを初期化しました。");
        }
    }

    // アイコンを登録
    m_pApp->RegisterPluginIconFromResource(g_hinstDLL, MAKEINTRESOURCE(IDB_ICON));

    // コマンドを登録
    TVTest::PluginCommandInfo ciList[4];
    ciList[0].ID = ID_COMMAND_SWITCH_LANG;
    ciList[0].State = TVTest::PLUGIN_COMMAND_STATE_DISABLED;
    ciList[0].pszText = L"SwitchLang";
    ciList[0].pszName = L"字幕言語切り替え";
    ciList[0].hbmIcon = static_cast<HBITMAP>(::LoadImage(g_hinstDLL, MAKEINTRESOURCE(IDB_SWITCH_LANG), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    ciList[1].ID = ID_COMMAND_SWITCH_SETTING;
    ciList[1].State = TVTest::PLUGIN_COMMAND_STATE_DISABLED;
    ciList[1].pszText = L"SwitchSetting";
    ciList[1].pszName = L"表示設定切り替え";
    ciList[1].hbmIcon = static_cast<HBITMAP>(::LoadImage(g_hinstDLL, MAKEINTRESOURCE(IDB_SWITCH_SETTING), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    ciList[2].ID = ID_COMMAND_CAPTURE;
    ciList[2].State = 0;
    ciList[2].pszText = L"Capture";
    ciList[2].pszName = L"字幕付き画像のコピー";
    ciList[2].hbmIcon = NULL;
    ciList[3].ID = ID_COMMAND_SAVE;
    ciList[3].State = 0;
    ciList[3].pszText = L"Save";
    ciList[3].pszName = L"字幕付き画像の保存";
    ciList[3].hbmIcon = NULL;
    for (int i = 0; i < _countof(ciList); ++i) {
        ciList[i].Size = sizeof(ciList[0]);
        ciList[i].Flags = ciList[i].hbmIcon ? TVTest::PLUGIN_COMMAND_FLAG_ICONIZE : 0;
        ciList[i].pszDescription = ciList[i].pszName;
        if (!m_pApp->RegisterPluginCommand(&ciList[i])) {
            m_pApp->RegisterCommand(ciList[i].ID, ciList[i].pszText, ciList[i].pszName);
        }
        if (ciList[i].hbmIcon) {
            ::DeleteObject(ciList[i].hbmIcon);
        }
    }

    // イベントコールバック関数を登録
    m_pApp->SetEventCallback(EventCallback, this);
    return true;
}


// 終了処理
bool CTVCaption2::Finalize()
{
    if (m_pApp->IsPluginEnabled()) EnablePlugin(false);
    m_osdCompositor.Uninitialize();
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


// Video Containerウィンドウ上の映像の位置を得る
bool CTVCaption2::GetVideoContainerLayout(HWND hwndContainer, RECT *pRect, RECT *pVideoRect, RECT *pExVideoRect)
{
    RECT rc;
    if (!hwndContainer || !::GetClientRect(hwndContainer, &rc)) {
        return false;
    }
    if (pRect) {
        *pRect = rc;
    }
    if (pVideoRect || pExVideoRect) {
        RECT rcPadding;
        if (!pVideoRect) pVideoRect = &rcPadding;
        if (!pExVideoRect) pExVideoRect = &rcPadding;

        // 正確に取得する術はなさそうなので中央に配置されていると仮定
        int aspectX = 16;
        int aspectY = 9;
        double cropX = 1;
        TVTest::VideoInfo vi;
        if (m_pApp->GetVideoInfo(&vi)) {
            if (vi.Height == 480 && vi.YAspect * 4 == 3 * vi.XAspect) {
                // 4:3SDを特別扱い(アスペクト比情報はいまいち正確でない可能性があるのであまり信用しない)(参考up0511mod)
                aspectX = 4;
                aspectY = 3;
            }
            if (vi.Width > 0) {
                cropX = (double)(vi.SourceRect.right - vi.SourceRect.left) / vi.Width;
                // Y方向ははみ出しやすいので考慮しない
            }
        }
        if (cropX == 0 || aspectX * cropX * rc.bottom < aspectY * rc.right) {
            // ウィンドウが動画よりもワイド
            pVideoRect->left = (rc.right - (int)(aspectX * cropX * rc.bottom / aspectY)) / 2;
            pVideoRect->right = rc.right - pVideoRect->left;
            pVideoRect->top = 0;
            pVideoRect->bottom = rc.bottom;
        }
        else {
            pVideoRect->top = (rc.bottom - (int)(aspectY * rc.right / (aspectX * cropX))) / 2;
            pVideoRect->bottom = rc.bottom - pVideoRect->top;
            pVideoRect->left = 0;
            pVideoRect->right = rc.right;
        }
        // カットした分だけ拡げる(ウィンドウからはみ出す場合もある)
        *pExVideoRect = *pVideoRect;
        if (cropX != 0) {
            int x = (int)((pExVideoRect->right - pExVideoRect->left) * (1 - cropX) / cropX) / 2;
            pExVideoRect->left -= x;
            pExVideoRect->right += x;
        }
    }
    return true;
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
        std::vector<WCHAR> text = ReadTextFileToEnd(gaijiPath, FILE_SHARE_READ);
        if (!text.empty()) {
            // 1行目は外字テーブル
            int len = ::StrCSpn(&text.front(), TEXT("\r\n"));
            DWORD tableSize = len;
            fRet = m_captionDll.SetGaiji(1, &text.front(), &tableSize);

            // あれば2行目からDRCS→文字列マップ
            LPCTSTR p = &text[len];
            SKIP_CRLF(p);
            if (!::StrCmpNI(p, TEXT("[DRCSMap]"), 9)) {
                p += ::StrCSpn(p, TEXT("\r\n"));
                SKIP_CRLF(p);
                while (*p && *p != TEXT('[')) {
                    DRCS_PAIR pair;
                    len = ::StrCSpn(p, TEXT("\r\n"));
                    if (HexStringToByteArray(p, pair.md5, 16) && p[32] == TEXT('=')) {
                        ::lstrcpyn(pair.str, p+33, min(len-32, _countof(pair.str)));
                        // 既ソートにしておく
                        if (pDrcsStrMap->empty() || DRCS_PAIR::COMPARE()(pDrcsStrMap->back(), pair)) {
                            pDrcsStrMap->push_back(pair);
                        }
                        else {
                            pDrcsStrMap->insert(std::lower_bound(pDrcsStrMap->begin(), pDrcsStrMap->end(), pair, DRCS_PAIR::COMPARE()), pair);
                        }
                    }
                    p += len;
                    SKIP_CRLF(p);
                }
            }
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
    if (!fRet) {
        pDrcsStrMap->clear();
        if (pCustomTable) ::memset(pCustomTable, 0, sizeof(WCHAR) * GAIJI_TABLE_SIZE * 2);
    }
    return fRet;
}

// プラグインの有効状態が変化した
bool CTVCaption2::EnablePlugin(bool fEnable)
{
    if (fEnable) {
        // 設定の読み込み
        LoadSettings();
        int iniVer = ::GetPrivateProfileInt(TEXT("Settings"), TEXT("Version"), 0, m_szIniPath);
        if (iniVer < INFO_VERSION) {
            // デフォルトの設定キーを出力するため
            SaveSettings();
        }
        if (m_captionDll.Initialize()) {
            if (!ConfigureGaijiTable(m_szGaijiTableName, &m_drcsStrMap, m_szGaijiFaceName[0]?m_customGaijiTable:NULL)) {
                m_pApp->AddLog(L"外字テーブルの設定に失敗しました。");
            }
            m_fProfileC = false;
            m_caption1Manager.SetProfileC(m_fProfileC);
            m_caption2Manager.SetProfileC(m_fProfileC);
            m_caption1Manager.ShowLang2(false);
            m_caption2Manager.ShowLang2(false);
            m_caption1Manager.SetCaptionDll(&m_captionDll, 0);
            m_caption2Manager.SetCaptionDll(&m_captionDll, 1);

            // 字幕描画処理ウィンドウ作成
            m_hwndPainting = ::CreateWindow(TV_CAPTION2_WINDOW_CLASS, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, g_hinstDLL, this);
            if (m_hwndPainting) {
                m_videoPid = GetVideoPid();
                m_fResetPat = true;

                // コールバックの登録
                m_pApp->SetStreamCallback(0, StreamCallback, this);
                m_pApp->SetWindowMessageCallback(WindowMsgCallback, this);

                m_pApp->SetPluginCommandState(ID_COMMAND_SWITCH_LANG, 0);
                m_pApp->SetPluginCommandState(ID_COMMAND_SWITCH_SETTING, 0);
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

        m_pApp->SetPluginCommandState(ID_COMMAND_SWITCH_LANG, TVTest::PLUGIN_COMMAND_STATE_DISABLED);
        m_pApp->SetPluginCommandState(ID_COMMAND_SWITCH_SETTING, TVTest::PLUGIN_COMMAND_STATE_DISABLED);
        return true;
    }
}


// 設定の読み込み
void CTVCaption2::LoadSettings()
{
    std::vector<TCHAR> vbuf = GetPrivateProfileSectionBuffer(TEXT("Settings"), m_szIniPath);
    GetBufferedProfileString(&vbuf.front(), TEXT("CaptureFolder"), TEXT(""), m_szCaptureFolder, _countof(m_szCaptureFolder));
    GetBufferedProfileString(&vbuf.front(), TEXT("CaptureFileName"), TEXT("Capture"), m_szCaptureFileName, _countof(m_szCaptureFileName));
    m_settingsIndex = GetBufferedProfileInt(&vbuf.front(), TEXT("SettingsIndex"), 0);

    // ここからはセクション固有
    if (m_settingsIndex > 0) {
        TCHAR section[32];
        ::wsprintf(section, TEXT("Settings%d"), m_settingsIndex);
        vbuf = GetPrivateProfileSectionBuffer(section, m_szIniPath);
    }
    LPCTSTR buf = &vbuf.front();
    GetBufferedProfileString(buf, TEXT("FaceName"), TEXT(""), m_szFaceName, _countof(m_szFaceName));
    GetBufferedProfileString(buf, TEXT("GaijiFaceName"), TEXT(""), m_szGaijiFaceName, _countof(m_szGaijiFaceName));
    GetBufferedProfileString(buf, TEXT("GaijiTableName"), TEXT("!std"), m_szGaijiTableName, _countof(m_szGaijiTableName));
    m_paintingMethod    = GetBufferedProfileInt(buf, TEXT("Method"), 2);
    m_showFlags[STREAM_CAPTION]     = GetBufferedProfileInt(buf, TEXT("ShowFlags"), 65535);
    m_showFlags[STREAM_SUPERIMPOSE] = GetBufferedProfileInt(buf, TEXT("ShowFlagsSuper"), 65535);
    m_delayTime[STREAM_CAPTION]     = GetBufferedProfileInt(buf, TEXT("DelayTime"), 450);
    m_delayTime[STREAM_SUPERIMPOSE] = GetBufferedProfileInt(buf, TEXT("DelayTimeSuper"), 0);
    m_fIgnorePts        = GetBufferedProfileInt(buf, TEXT("IgnorePts"), 0) != 0;
    int textColor       = GetBufferedProfileInt(buf, TEXT("TextColor"), -1);
    int backColor       = GetBufferedProfileInt(buf, TEXT("BackColor"), -1);
    m_textOpacity       = GetBufferedProfileInt(buf, TEXT("TextOpacity"), -1);
    m_backOpacity       = GetBufferedProfileInt(buf, TEXT("BackOpacity"), -1);
    m_vertAntiAliasing  = GetBufferedProfileInt(buf, TEXT("VertAntiAliasing"), 22);
    m_rcAdjust.left     = GetBufferedProfileInt(buf, TEXT("FontXAdjust"), 0);
    m_rcAdjust.top      = GetBufferedProfileInt(buf, TEXT("FontYAdjust"), 0);
    m_rcAdjust.right    = GetBufferedProfileInt(buf, TEXT("FontSizeAdjust"), 100);
    m_rcAdjust.bottom   = GetBufferedProfileInt(buf, TEXT("FontRatioAdjust"), 100);
    m_rcGaijiAdjust.left = GetBufferedProfileInt(buf, TEXT("GaijiFontXAdjust"), 0);
    m_rcGaijiAdjust.top = GetBufferedProfileInt(buf, TEXT("GaijiFontYAdjust"), 0);
    m_rcGaijiAdjust.right = GetBufferedProfileInt(buf, TEXT("GaijiFontSizeAdjust"), 100);
    m_rcGaijiAdjust.bottom = GetBufferedProfileInt(buf, TEXT("GaijiFontRatioAdjust"), 100);
    m_strokeWidth       = GetBufferedProfileInt(buf, TEXT("StrokeWidth"), -5);
    m_strokeSmoothLevel = GetBufferedProfileInt(buf, TEXT("StrokeSmoothLevel"), 1);
    m_strokeByDilate    = GetBufferedProfileInt(buf, TEXT("StrokeByDilate"), 22);
    m_paddingWidth      = GetBufferedProfileInt(buf, TEXT("PaddingWidth"), 0);
    m_avoidHalfFlags    = GetBufferedProfileInt(buf, TEXT("AvoidHalfFlags"), 0);
    m_fIgnoreSmall      = GetBufferedProfileInt(buf, TEXT("IgnoreSmall"), 0) != 0;
    m_fShiftSmall       = GetBufferedProfileInt(buf, TEXT("ShiftSmall"), 0) != 0;
    m_fCentering        = GetBufferedProfileInt(buf, TEXT("Centering"), 0) != 0;
    m_fShrinkSDScale    = GetBufferedProfileInt(buf, TEXT("ShrinkSDScale"), 0) != 0;
    m_adjustViewX       = GetBufferedProfileInt(buf, TEXT("ViewXAdjust"), 0);
    m_adjustViewY       = GetBufferedProfileInt(buf, TEXT("ViewYAdjust"), 0);
    GetBufferedProfileString(buf, TEXT("RomSoundList"), ROMSOUND_EXAMPLE, m_szRomSoundList, _countof(m_szRomSoundList));

    m_fEnTextColor = textColor >= 0;
    m_textColor = RGB(textColor/1000000%1000, textColor/1000%1000, textColor%1000);
    m_fEnBackColor = backColor >= 0;
    m_backColor = RGB(backColor/1000000%1000, backColor/1000%1000, backColor%1000);
}


// 設定の保存
void CTVCaption2::SaveSettings() const
{
    WritePrivateProfileInt(TEXT("Settings"), TEXT("Version"), INFO_VERSION, m_szIniPath);
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
    WritePrivateProfileInt(section, TEXT("FontXAdjust"), m_rcAdjust.left, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("FontYAdjust"), m_rcAdjust.top, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("FontSizeAdjust"), m_rcAdjust.right, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("FontRatioAdjust"), m_rcAdjust.bottom, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("GaijiFontXAdjust"), m_rcGaijiAdjust.left, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("GaijiFontYAdjust"), m_rcGaijiAdjust.top, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("GaijiFontSizeAdjust"), m_rcGaijiAdjust.right, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("GaijiFontRatioAdjust"), m_rcGaijiAdjust.bottom, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("StrokeWidth"), m_strokeWidth, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("StrokeSmoothLevel"), m_strokeSmoothLevel, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("StrokeByDilate"), m_strokeByDilate, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("PaddingWidth"), m_paddingWidth, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("AvoidHalfFlags"), m_avoidHalfFlags, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("IgnoreSmall"), m_fIgnoreSmall, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("ShiftSmall"), m_fShiftSmall, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("Centering"), m_fCentering, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("ShrinkSDScale"), m_fShrinkSDScale, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("ViewXAdjust"), m_adjustViewX, m_szIniPath);
    WritePrivateProfileInt(section, TEXT("ViewYAdjust"), m_adjustViewY, m_szIniPath);
    ::WritePrivateProfileString(section, TEXT("RomSoundList"), m_szRomSoundList, m_szIniPath);
}


// 設定の切り替え
// specIndex<0のとき次の設定に切り替え
void CTVCaption2::SwitchSettings(int specIndex)
{
    m_settingsIndex = specIndex < 0 ? m_settingsIndex + 1 : specIndex;
    TCHAR section[32];
    ::wsprintf(section, TEXT("Settings%d"), m_settingsIndex);
    if (::GetPrivateProfileInt(section, TEXT("Method"), 0, m_szIniPath) == 0) {
        m_settingsIndex = 0;
    }
    WritePrivateProfileInt(TEXT("Settings"), TEXT("SettingsIndex"), m_settingsIndex, m_szIniPath);
    LoadSettings();
}


void CTVCaption2::AddSettings()
{
    WritePrivateProfileInt(TEXT("Settings"), TEXT("SettingsIndex"), GetSettingsCount(), m_szIniPath);
    LoadSettings();
    SaveSettings();
}


void CTVCaption2::DeleteSettings()
{
    int settingsCount = GetSettingsCount();
    if (settingsCount >= 2) {
        int lastIndex = m_settingsIndex;
        // 1つずつ手前にシフト
        for (int i = m_settingsIndex; i < settingsCount - 1; ++i) {
            WritePrivateProfileInt(TEXT("Settings"), TEXT("SettingsIndex"), i + 1, m_szIniPath);
            LoadSettings();
            m_settingsIndex = i;
            SaveSettings();
        }
        // 末尾セクションを削除
        TCHAR section[32];
        ::wsprintf(section, TEXT("Settings%d"), settingsCount - 1);
        ::WritePrivateProfileString(section, NULL, NULL, m_szIniPath);
        SwitchSettings(min(lastIndex, settingsCount - 2));
    }
}


int CTVCaption2::GetSettingsCount() const
{
    for (int i = 1; ; ++i) {
        TCHAR section[32];
        ::wsprintf(section, TEXT("Settings%d"), i);
        if (::GetPrivateProfileInt(section, TEXT("Method"), 0, m_szIniPath) == 0) {
            return i;
        }
    }
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
    case TVTest::EVENT_PLUGINSETTINGS:
        // プラグインの設定を行う
        return pThis->PluginSettings(reinterpret_cast<HWND>(lParam1));
    case TVTest::EVENT_FULLSCREENCHANGE:
        // 全画面表示状態が変化した
        if (pThis->m_pApp->IsPluginEnabled()) {
            // オーナーが変わるので破棄する必要がある
            ::SendMessage(pThis->m_hwndPainting, WM_RESET_OSDS, 0, 0);
        }
        break;
    case TVTest::EVENT_CHANNELCHANGE:
        // チャンネルが変更された
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
                pThis->HideAllOsds();
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
                    pThis->m_caption1Manager.ShowLang2(!pThis->m_caption1Manager.IsShowLang2());
                    pThis->m_caption2Manager.ShowLang2(!pThis->m_caption2Manager.IsShowLang2());
                    bool fShowLang2 = pThis->m_caption1Manager.IsShowLang2();
                    pThis->m_pApp->SetPluginCommandState(ID_COMMAND_SWITCH_LANG, fShowLang2 ? TVTest::PLUGIN_COMMAND_STATE_CHECKED : 0);
                    TCHAR str[32];
                    ::wsprintf(str, TEXT("第%d言語に切り替えました。"), fShowLang2 ? 2 : 1);
                    pThis->m_pApp->AddLog(str);
                }
                break;
            case ID_COMMAND_SWITCH_SETTING:
                pThis->HideAllOsds();
                pThis->SwitchSettings();
                if (!pThis->ConfigureGaijiTable(pThis->m_szGaijiTableName, &pThis->m_drcsStrMap,
                                                pThis->m_szGaijiFaceName[0]?pThis->m_customGaijiTable:NULL))
                {
                    pThis->m_pApp->AddLog(L"外字テーブルの設定に失敗しました。");
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
    case TVTest::EVENT_FILTERGRAPH_INITIALIZED:
        // フィルタグラフの初期化終了
        pThis->m_osdCompositor.OnFilterGraphInitialized(reinterpret_cast<const TVTest::FilterGraphInfo*>(lParam1)->pGraphBuilder);
        break;
    case TVTest::EVENT_FILTERGRAPH_FINALIZE:
        // フィルタグラフの終了処理開始
        pThis->m_osdCompositor.OnFilterGraphFinalize(reinterpret_cast<const TVTest::FilterGraphInfo*>(lParam1)->pGraphBuilder);
        break;
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

            RECT rc, rcVideo;
            if (GetVideoContainerLayout(m_hwndContainer, &rc, &rcVideo)) {
                BITMAPINFOHEADER bihRes = bih;
                bihRes.biWidth = rcVideo.right - rcVideo.left;
                bihRes.biHeight = rcVideo.bottom - rcVideo.top;
                // キャプチャ画像が表示中の動画サイズと異なるときは動画サイズのビットマップに変換する
                if (bih.biWidth < bihRes.biWidth-3 || bihRes.biWidth+3 < bih.biWidth ||
                    bih.biHeight < bihRes.biHeight-3 || bihRes.biHeight+3 < bih.biHeight)
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

                // ビットマップに表示中のOSDを合成
                HDC hdc = ::CreateCompatibleDC(NULL);
                HBITMAP hbmOld = static_cast<HBITMAP>(::SelectObject(hdc, hbm));
                for (int i = 0; i < STREAM_MAX; ++i) {
                    for (int j = 0; j < m_osdShowCount[i]; ++j) {
                        int left, top;
                        m_pOsdList[i][j]->GetPosition(&left, &top, NULL, NULL);
                        m_pOsdList[i][j]->Compose(hdc, left - rcVideo.left, top - rcVideo.top);
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
                            BITMAPFILEHEADER bmfHeader = {};
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

    for (; m_osdShowCount[index] > 0; --m_osdShowCount[index]) {
        m_pOsdList[index].front()->Hide();
        // 表示待ちOSDの直後に移動
        CPseudoOSD *p = m_pOsdList[index].front();
        m_pOsdList[index].insert(m_pOsdList[index].begin() + m_osdShowCount[index] + m_osdPrepareCount[index], p);
        m_pOsdList[index].erase(m_pOsdList[index].begin());
    }
}

void CTVCaption2::DeleteTextures()
{
    if (m_paintingMethod == 3) {
        if (m_osdCompositor.DeleteTexture(0, 0)) {
            m_osdCompositor.UpdateSurface();
        }
        // フラッシングタイマ終了
        ::KillTimer(m_hwndPainting, TIMER_ID_FLASHING_TEXTURE);
    }
}

void CTVCaption2::HideAllOsds()
{
    HideOsds(STREAM_CAPTION);
    HideOsds(STREAM_SUPERIMPOSE);
    DeleteTextures();
}

void CTVCaption2::DestroyOsds()
{
    DEBUG_OUT(TEXT(__FUNCTION__) TEXT("()\n"));

    for (int index = 0; index < STREAM_MAX; ++index) {
        while (!m_pOsdList[index].empty()) {
            delete m_pOsdList[index].back();
            m_pOsdList[index].pop_back();
        }
        m_osdShowCount[index] = 0;
        m_osdPrepareCount[index] = 0;
        m_fOsdClear[index] = false;
    }
    DeleteTextures();
}

static void AddOsdText(CPseudoOSD *pOsd, LPCTSTR text, int width, int charWidth, int charHeight,
                       const RECT &rcFontAdjust, LPCTSTR faceName, const CAPTION_CHAR_DATA_DLL &style)
{
    LOGFONT logFont;
    logFont.lfHeight         = -charHeight;
    logFont.lfWidth          = charWidth / 2;
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
    RECT rc = {charHeight * rcFontAdjust.left / 72, charHeight * rcFontAdjust.top / 72, rcFontAdjust.right * rcFontAdjust.bottom / 100, rcFontAdjust.right};
    pOsd->AddText(text, width, logFont, rc);
}

// 利用可能なOSDを1つだけ用意する
CPseudoOSD &CTVCaption2::CreateOsd(STREAM_INDEX index, HWND hwndContainer, int charHeight, int nomalHeight, const CAPTION_CHAR_DATA_DLL &style)
{
    DEBUG_OUT(TEXT(__FUNCTION__) TEXT("()\n"));

    while (m_osdShowCount[index] + m_osdPrepareCount[index] >= (int)m_pOsdList[index].size()) {
        m_pOsdList[index].push_back(new CPseudoOSD);
    }
    CPseudoOSD &osd = *m_pOsdList[index][m_osdShowCount[index] + m_osdPrepareCount[index]++];
    osd.ClearText();
    osd.SetTextColor(m_fEnTextColor ? m_textColor : RGB(style.stCharColor.ucR, style.stCharColor.ucG, style.stCharColor.ucB),
                     m_fEnBackColor ? m_backColor : RGB(style.stBackColor.ucR, style.stBackColor.ucG, style.stBackColor.ucB));

    int textOpacity = m_textOpacity>=0 ? min(m_textOpacity,100) : style.stCharColor.ucAlpha*100/255;
    int backOpacity = !m_fProfileC && style.stBackColor.ucAlpha==0 ? 0 :
                      m_backOpacity>=0 ? min(m_backOpacity,100) : style.stBackColor.ucAlpha*100/255;
    osd.SetOpacity(textOpacity, backOpacity);

    osd.SetStroke(m_strokeWidth < 0 ? max(-m_strokeWidth*(nomalHeight+charHeight)/2,1) : m_strokeWidth*72,
                  m_strokeSmoothLevel, charHeight<=m_strokeByDilate ? true : false);
    osd.SetHighlightingBlock((style.bHLC&0x80)!=0, (style.bHLC&0x40)!=0, (style.bHLC&0x20)!=0, (style.bHLC&0x10)!=0);
    osd.SetVerticalAntiAliasing(charHeight<=m_vertAntiAliasing ? false : true);
    osd.SetFlashingInterval(style.bFlushMode==1 ? FLASHING_INTERVAL : style.bFlushMode==2 ? -FLASHING_INTERVAL : 0);
    osd.Create(hwndContainer, m_paintingMethod==2 || m_paintingMethod==3);
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


// 字幕本文を予行で1行だけ処理する
void CTVCaption2::DryrunCaptionData(const CAPTION_DATA_DLL &caption, SHIFT_SMALL_STATE &ssState)
{
    int posY = caption.wPosY;

    if (posY < ssState.posY) {
        // 前回よりも上方なのでリセット
        ssState = SHIFT_SMALL_STATE();
    }
    if (posY > ssState.posY) {
        if (ssState.dirH > 0 && posY >= ssState.posY + ssState.dirH && ssState.fSmall) {
            // 前回の行を詰めることができる
            ssState.shiftH -= ssState.dirH;
        }
        ssState.dirH = 0;
        // 先頭行は詰めない
        ssState.fSmall = ssState.posY >= 0;
    }
    ssState.posY = posY;

    for (DWORD i = 0; i < caption.dwListCount; ++i) {
        const CAPTION_CHAR_DATA_DLL &charData = caption.pstCharList[i];
        int dirH;
        GetCharSize(NULL, NULL, NULL, &dirH, charData);
        if (ssState.dirH <= 0) {
            ssState.dirH = dirH;
        }
        ssState.fSmall = ssState.fSmall && dirH == ssState.dirH && m_fShiftSmall && charData.wCharSizeMode == CP_STR_SMALL;
    }
}


// 字幕本文を1行だけ処理する
void CTVCaption2::ShowCaptionData(STREAM_INDEX index, const CAPTION_DATA_DLL &caption, const DRCS_PATTERN_DLL *pDrcsList, DWORD drcsCount,
                                  SHIFT_SMALL_STATE &ssState, HWND hwndContainer, const RECT &rcVideo)
{
#ifdef DDEBUG_OUT
    DEBUG_OUT(TEXT(__FUNCTION__) TEXT("(): "));
    for (DWORD i = 0; i < caption.dwListCount; ++i) {
        DEBUG_OUT(static_cast<LPCTSTR>(caption.pstCharList[i].pszDecode));
    }
    DEBUG_OUT(TEXT("\n"));
#endif

    // 字幕プレーンから描画ウィンドウへの変換係数を計算
    double scaleX = (double)(rcVideo.right - rcVideo.left);
    double scaleY = (double)(rcVideo.bottom - rcVideo.top);
    int offsetX = rcVideo.left;
    int offsetY = rcVideo.top;
    bool shrinkScale = false;
    if (caption.wSWFMode==14) {
        // Cプロファイル
        scaleX /= 320+20/2;
        scaleY /= 180;
        offsetY += (int)((180-24*3)*scaleY);
    }
    else if (caption.wSWFMode==9 || caption.wSWFMode==10) {
        scaleX /= 720;
        scaleY /= 480;
        shrinkScale = m_fShrinkSDScale;
        if (shrinkScale) {
            // 16:9の描画ウィンドウに対して字幕プレーンのアスペクト比が不変となる係数をかける
            scaleX = scaleX * 27 / 32;
            offsetX += (rcVideo.right - rcVideo.left) * (32 - 27) / 32 / 2;
        }
    }
    else {
        scaleX /= 960;
        scaleY /= 540;
    }
    if (m_fCentering) {
        scaleX /= 1.5;
        scaleY /= 1.5;
        offsetX += (rcVideo.right - rcVideo.left) * (shrinkScale ? 27 : 32) / 32 / 6;
    }
    offsetX += (rcVideo.right - rcVideo.left) * min(max(m_adjustViewX, -99), 99) / 100;
    offsetY += (rcVideo.bottom - rcVideo.top) * min(max(m_adjustViewY, -99), 99) / 100;

    bool fDoneAntiPadding = false;
    int posX = caption.wPosX;
    int posY = caption.wPosY;

    if (posY < ssState.posY) {
        // 前回よりも上方なのでリセット
        ssState = SHIFT_SMALL_STATE();
    }
    if (posY > ssState.posY) {
        if (ssState.dirH > 0 && posY >= ssState.posY + ssState.dirH && ssState.fSmall) {
            // 前回の行を詰めることができる
            ssState.shiftH -= ssState.dirH;
        }
        ssState.dirH = 0;
        // 先頭行は詰めない
        ssState.fSmall = ssState.posY >= 0;
    }
    ssState.posY = posY;
    posY += ssState.shiftH;

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
        // 両端の余白幅は文字高さを基準にきめる
        int paddingW = (charData.wCharH * m_paddingWidth + 71) / 72;

        if (ssState.dirH <= 0) {
            ssState.dirH = dirH;
        }
        ssState.fSmall = ssState.fSmall && dirH == ssState.dirH && m_fShiftSmall && charData.wCharSizeMode == CP_STR_SMALL;

        LPCTSTR pszShow = m_fIgnoreSmall && charData.wCharSizeMode == CP_STR_SMALL ? TEXT("") :
                          pszCarry ? pszCarry : static_cast<LPCTSTR>(charData.pszDecode);
        pszCarry = NULL;

        // 文字列にDRCSか外字か半角置換可能文字が含まれるか調べる
        const DRCS_PATTERN_DLL *pDrcs = NULL;
        LPCTSTR pszDrcsStr = NULL;
        bool fSearchGaiji = m_szGaijiFaceName[0] != 0;
        WCHAR szGaiji[3] = {};
        bool fSearchHalf = charData.wCharSizeMode == CP_STR_MEDIUM;
        WCHAR szHalf[2] = {};
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
                        DRCS_PAIR e;
                        e.str[0] = 0;
                        if (!m_drcsStrMap.empty() && CalcMD5FromDRCSPattern(e.md5, pDrcs)) {
                            // 2分探索
                            std::vector<DRCS_PAIR>::const_iterator it =
                                std::lower_bound(m_drcsStrMap.begin(), m_drcsStrMap.end(), e, DRCS_PAIR::COMPARE());
                            if (it != m_drcsStrMap.end() && !DRCS_PAIR::COMPARE()(e, *it)) {
                                pszDrcsStr = it->str;
                            }
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
                    for (int k = 0; HALF_F_LIST[k]; ++k) {
                        TCHAR r = HALF_R_LIST[k];
                        if ((!(m_avoidHalfFlags & 1) || r != TEXT('A') && r != TEXT('a')) &&
                            (!(m_avoidHalfFlags & 2) || r != TEXT('0')) &&
                            (!(m_avoidHalfFlags & 4) || r == TEXT('A') || r == TEXT('a') || r == TEXT('0')) &&
                            HALF_F_LIST[k] <= pszShow[j] && pszShow[j] <= HALF_T_LIST[k])
                        {
                            // 半角置換可能文字
                            szHalf[0] = r + pszShow[j] - HALF_F_LIST[k];
                            szHalf[1] = 0;
                            pszCarry = &pszShow[j+1];
                            break;
                        }
                    }
                    if (pszCarry) break;
                }
            }
        }

        TCHAR szTmp[256];
        bool fDrawSymbol = pDrcs || szGaiji[0] || szHalf[0];
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
                       charScaleW, charScaleH, m_rcAdjust, m_szFaceName, charData);
            posX += dirW * lenWos;
            if (!fDrawSymbol && !fSameStyle) {
                if (paddingW > 0) {
                    AddOsdText(pOsdCarry, TEXT(""), (int)((posX+paddingW)*scaleX) - (int)(posX*scaleX),
                               charScaleW, charScaleH, m_rcAdjust, m_szFaceName, charData);
                    posX += paddingW;
                }
                if (m_paintingMethod != 3) pOsdCarry->PrepareWindow();
                pOsdCarry = NULL;
            }
        }
        else {
            if (fDrawSymbol || lenWos > 0) {
                if (paddingW > 0 && !fDoneAntiPadding) {
                    posX -= paddingW;
                    fDoneAntiPadding = true;
                }
                CPseudoOSD &osd = CreateOsd(index, hwndContainer, charScaleH, charNormalScaleH, charData);
                osd.SetPosition((int)(posX*scaleX) + offsetX, (int)((posY-dirH+1)*scaleY) + offsetY,
                                (int)((posY+1)*scaleY) - (int)((posY-dirH+1)*scaleY));
                if (paddingW > 0) {
                    AddOsdText(&osd, TEXT(""), (int)((posX+paddingW)*scaleX) - (int)(posX*scaleX),
                               charScaleW, charScaleH, m_rcAdjust, m_szFaceName, charData);
                    posX += paddingW;
                }
                AddOsdText(&osd, pszShow, (int)((posX+dirW*lenWos)*scaleX) - (int)(posX*scaleX),
                           charScaleW, charScaleH, m_rcAdjust, m_szFaceName, charData);
                posX += dirW * lenWos;
                if (fDrawSymbol || fSameStyle) {
                    pOsdCarry = &osd;
                }
                else {
                    if (paddingW > 0) {
                        AddOsdText(&osd, TEXT(""), (int)((posX+paddingW)*scaleX) - (int)(posX*scaleX),
                                   charScaleW, charScaleH, m_rcAdjust, m_szFaceName, charData);
                        posX += paddingW;
                    }
                    if (m_paintingMethod != 3) osd.PrepareWindow();
                }
            }
        }

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

            if (pOsdCarry) {
                void *pBits;
                HBITMAP hbm = ::CreateDIBSection(NULL, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
                if (hbm) {
                    ::SetDIBits(NULL, hbm, 0, bmi.bmiHeader.biHeight, pDrcs->pbBitmap, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);
                    RECT rc;
                    ::SetRect(&rc, (int)((dirW-charW)/2*scaleX), (int)((dirH-charH)/2*scaleY), charScaleW, charScaleH);
                    pOsdCarry->AddImage(hbm, (int)((posX+dirW)*scaleX) - (int)(posX*scaleX), rc);
                    posX += dirW;
                }
            }
        }
        else if (pDrcs && pszDrcsStr) {
            // DRCSを文字列で描画
            lenWos = StrlenWoLoSurrogate(pszDrcsStr);
            if (pOsdCarry && lenWos > 0) {
                // レイアウト維持のため、何文字であっても1文字幅に詰める
                AddOsdText(pOsdCarry, pszDrcsStr, (int)((posX+dirW)*scaleX) - (int)(posX*scaleX),
                           charScaleW / lenWos + 1, charScaleH,
                           m_szGaijiFaceName[0] ? m_rcGaijiAdjust : m_rcAdjust,
                           m_szGaijiFaceName[0] ? m_szGaijiFaceName : m_szFaceName, charData);
                posX += dirW;
            }
        }
        else if (szGaiji[0]) {
            // 外字を描画
            if (pOsdCarry) {
                AddOsdText(pOsdCarry, szGaiji, (int)((posX+dirW)*scaleX) - (int)(posX*scaleX),
                           charScaleW, charScaleH, m_rcGaijiAdjust, m_szGaijiFaceName, charData);
                posX += dirW;
            }
        }
        else if (szHalf[0]) {
            // 半角文字を描画
            if (pOsdCarry) {
                AddOsdText(pOsdCarry, szHalf, (int)((posX+dirW)*scaleX) - (int)(posX*scaleX),
                           -charNormalScaleW, charScaleH, m_rcAdjust, m_szFaceName, charData);
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
            ::PostMessage(hwnd, WM_RESET_OSDS, 0, 0);
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
        case TIMER_ID_FLASHING_TEXTURE:
            {
                bool fModified;
                fModified = pThis->m_osdCompositor.ShowTexture(pThis->m_fFlashingFlipFlop, 0, STREAM_CAPTION + TXGROUP_FLASHING);
                fModified = pThis->m_osdCompositor.ShowTexture(pThis->m_fFlashingFlipFlop, 0, STREAM_SUPERIMPOSE + TXGROUP_FLASHING) || fModified;
                fModified = pThis->m_osdCompositor.ShowTexture(!pThis->m_fFlashingFlipFlop, 0, STREAM_CAPTION + TXGROUP_IFLASHING) || fModified;
                fModified = pThis->m_osdCompositor.ShowTexture(!pThis->m_fFlashingFlipFlop, 0, STREAM_SUPERIMPOSE + TXGROUP_IFLASHING) || fModified;
                if (fModified) {
                    pThis->m_osdCompositor.UpdateSurface();
                }
                pThis->m_fFlashingFlipFlop = !pThis->m_fFlashingFlipFlop;
            }
            return 0;
        }
        break;
    case WM_RESET_CAPTION:
        pThis->HideAllOsds();
        {
            CBlockLock lock(&pThis->m_streamLock);
            if (wParam != 0) {
                // ワンセグ字幕をすばやく表示するため
                pThis->m_fProfileC = wParam != 1;
                pThis->m_caption1Manager.SetProfileC(pThis->m_fProfileC);
                pThis->m_caption2Manager.SetProfileC(pThis->m_fProfileC);
            }
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
            pThis->m_pOsdList[STREAM_CAPTION][i]->OnParentMove();
        }
        for (int i = 0; i < pThis->m_osdShowCount[STREAM_SUPERIMPOSE]; ++i) {
            pThis->m_pOsdList[STREAM_SUPERIMPOSE][i]->OnParentMove();
        }
        return 0;
    case WM_DONE_SIZE:
        pThis->OnSize(STREAM_CAPTION);
        pThis->OnSize(STREAM_SUPERIMPOSE);
        return 0;
    case WM_RESET_OSDS:
        DEBUG_OUT(TEXT(__FUNCTION__) TEXT("(): WM_RESET_OSDS\n"));
        pThis->DestroyOsds();
        pThis->m_hwndContainer = pThis->FindVideoContainer();
        if (pThis->m_hwndContainer) {
            for (int i = 0; i < OSD_PRE_CREATE_NUM; ++i) {
                pThis->m_pOsdList[STREAM_CAPTION].push_back(new CPseudoOSD);
                pThis->m_pOsdList[STREAM_CAPTION].back()->Create(pThis->m_hwndContainer, pThis->m_paintingMethod==2 || pThis->m_paintingMethod==3);
            }
            pThis->m_osdCompositor.SetContainerWindow(pThis->m_hwndContainer);
        }
        return 0;
    }
    return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


void CTVCaption2::ProcessCaption(CCaptionManager *pCaptionManager, const CAPTION_DATA_DLL *pCaptionForTest)
{
    STREAM_INDEX index = STREAM_CAPTION;
    DWORD pcr = 0;
    if (!pCaptionForTest) {
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

    // 表示タイミングに達した字幕本文を1つだけ取得する
    const CAPTION_DATA_DLL *pCaption = pCaptionForTest ? pCaptionForTest : pCaptionManager->PopCaption(pcr, m_fIgnorePts);
    if (!pCaption) {
        return;
    }
    else {
        int dmf = 10;
        if (!pCaptionForTest) {
            const LANG_TAG_INFO_DLL *pLangTag = pCaptionManager->GetLangTag();
            dmf = pLangTag ? pLangTag->ucDMF : 16;
        }
        if (m_fNeedtoShow && ((m_showFlags[index]>>dmf)&1)) {
            if (pCaption->bClear) {
                m_fOsdClear[index] = true;
                m_shiftSmallState[index] = SHIFT_SMALL_STATE();
            }
            else {
                RECT rcVideo;
                if (GetVideoContainerLayout(m_hwndContainer, NULL, NULL, &rcVideo)) {
                    const DRCS_PATTERN_DLL *pDrcsList = NULL;
                    DWORD drcsCount = 0;
                    if (!pCaptionForTest) {
                        pCaptionManager->GetDrcsList(&pDrcsList, &drcsCount);
                        if (m_shiftSmallState[index].posY < 0) {
                            // クリア後の最初の字幕本文
                            SHIFT_SMALL_STATE ssState;
                            for (int i = -1; ; ++i) {
                                const CAPTION_DATA_DLL *p = i < 0 ? pCaption : pCaptionManager->GetCaption(pcr, m_fIgnorePts, i);
                                if (!p || p->bClear) {
                                    break;
                                }
                                ssState.shiftH = -1;
                                DryrunCaptionData(*p, ssState);
                                if (ssState.shiftH >= 0) {
                                    // リセットされた
                                    break;
                                }
                                // 上に詰める高さを打ち消すことで実質的に下に詰める
                                m_shiftSmallState[index].shiftH -= ssState.shiftH + 1;
                            }
                        }
                    }
                    ShowCaptionData(index, *pCaption, pDrcsList, drcsCount, m_shiftSmallState[index], m_hwndContainer, rcVideo);
                }
            }
        }
        // 次行が取得できるかどうか
        if (!pCaptionForTest && pCaptionManager->GetCaption(pcr, m_fIgnorePts, 0)) {
            // 他のメッセージの遅延を減らすため次行の描画は後回しにする
            ::PostMessage(m_hwndPainting, WM_PROCESS_CAPTION, 0, 0);
            return;
        }
    }

    bool fTextureModified = false;
    if (m_fOsdClear[index]) {
        m_fOsdClear[index] = false;
        HideOsds(index);
        if (m_paintingMethod == 3) {
            bool fDeleted;
            fDeleted = m_osdCompositor.DeleteTexture(0, index + TXGROUP_NORMAL);
            fDeleted = m_osdCompositor.DeleteTexture(0, index + TXGROUP_FLASHING) || fDeleted;
            fDeleted = m_osdCompositor.DeleteTexture(0, index + TXGROUP_IFLASHING) || fDeleted;
            if (fDeleted) {
                fTextureModified = true;
            }
        }
    }
    // ちらつき防止のため表示処理をまとめる
    for (; m_osdPrepareCount[index] > 0; --m_osdPrepareCount[index], ++m_osdShowCount[index]) {
        if (m_paintingMethod == 3) {
            HBITMAP hbm = m_pOsdList[index][m_osdShowCount[index]]->CreateBitmap();
            if (hbm) {
                int left, top;
                m_pOsdList[index][m_osdShowCount[index]]->GetPosition(&left, &top, NULL, NULL);
                RECT rcVideo;
                if (GetVideoContainerLayout(m_hwndContainer, NULL, &rcVideo)) {
                    // 疑似OSD系から逆変換
                    left -= rcVideo.left;
                    top -= rcVideo.top;
                }
                int intv = m_pOsdList[index][m_osdShowCount[index]]->GetFlashingInterval();
                int group = intv > 0 ? TXGROUP_FLASHING : intv < 0 ? TXGROUP_IFLASHING : TXGROUP_NORMAL;
                if (m_osdCompositor.AddTexture(hbm, left, top, group!=TXGROUP_IFLASHING, index + group) != 0) {
                    fTextureModified = true;
                    if (group != TXGROUP_NORMAL) {
                        // フラッシングタイマ開始
                        m_fFlashingFlipFlop = false;
                        ::SetTimer(m_hwndPainting, TIMER_ID_FLASHING_TEXTURE, FLASHING_INTERVAL, NULL);
                    }
                }
                ::DeleteObject(hbm);
            }
        }
        else {
            m_pOsdList[index][m_osdShowCount[index]]->Show();
        }
    }
    if (fTextureModified) {
        m_osdCompositor.UpdateSurface();
    }
}


void CTVCaption2::OnSize(STREAM_INDEX index)
{
    if (m_osdShowCount[index] > 0) {
        RECT rc;
        if (GetVideoContainerLayout(m_hwndContainer, &rc, NULL)) {
            // とりあえずはみ出ないようにする
            for (int i = 0; i < m_osdShowCount[index]; ++i) {
                int left, top, width, height;
                m_pOsdList[index][i]->GetPosition(&left, &top, &width, &height);
                int adjLeft = left+width>=rc.right ? rc.right-width : left;
                int adjTop = top+height>=rc.bottom ? rc.bottom-height : top;
                if (adjLeft < 0 || adjTop < 0) {
                    m_pOsdList[index][i]->Hide();
                }
                else if (left != adjLeft || top != adjTop) {
                    m_pOsdList[index][i]->SetPosition(adjLeft, adjTop, height);
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


static void GetPidsFromVideoPmt(int *pPmtPid, int *pPcrPid, int *pCaption1Pid, int *pCaption2Pid, int videoPid, const PAT *pPat)
{
    for (size_t i = 0; i < pPat->pmt.size(); ++i) {
        const PMT *pPmt = &pPat->pmt[i];
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
            *pPmtPid = pPat->pmt[i].pmt_pid;
            *pPcrPid = pPmt->pcr_pid;
            *pCaption1Pid = privPid[0];
            *pCaption2Pid = privPid[1];
            return;
        }
    }
    *pPmtPid = -1;
    *pPcrPid = -1;
    *pCaption1Pid = -1;
    *pCaption2Pid = -1;
}


void CTVCaption2::ProcessPacket(BYTE *pPacket)
{
    if (m_fResetPat) {
        PAT zeroPat = {};
        m_pat = zeroPat;
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
        int pmtPid;
        if (header.pid == 0) {
            extract_pat(&m_pat, pPayload, payloadSize,
                        header.payload_unit_start_indicator,
                        header.continuity_counter);
            GetPidsFromVideoPmt(&pmtPid, &m_pcrPid, &m_caption1Pid, &m_caption2Pid, m_videoPid, &m_pat);
            if (pmtPid >= 0 && Is1SegPmtPid(pmtPid) != m_fProfileC) {
                ::SendNotifyMessage(m_hwndPainting, WM_RESET_CAPTION, Is1SegPmtPid(pmtPid) ? 2 : 1, 0);
            }
            return;
        }
        // PATリストにあるPMT監視
        for (size_t i = 0; i < m_pat.pmt.size(); ++i) {
            if (header.pid == m_pat.pmt[i].pmt_pid/* && header.pid != 0*/) {
                extract_pmt(&m_pat.pmt[i], pPayload, payloadSize,
                            header.payload_unit_start_indicator,
                            header.continuity_counter);
                GetPidsFromVideoPmt(&pmtPid, &m_pcrPid, &m_caption1Pid, &m_caption2Pid, m_videoPid, &m_pat);
                if (pmtPid >= 0 && Is1SegPmtPid(pmtPid) != m_fProfileC) {
                    ::SendNotifyMessage(m_hwndPainting, WM_RESET_CAPTION, Is1SegPmtPid(pmtPid) ? 2 : 1, 0);
                }
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


// プラグインの設定を行う
bool CTVCaption2::PluginSettings(HWND hwndOwner)
{
    if (!m_pApp->IsPluginEnabled()) {
        ::MessageBox(hwndOwner, TEXT("プラグインを有効にしてください。"), INFO_PLUGIN_NAME, MB_ICONERROR | MB_OK);
        return false;
    }
    ::DialogBoxParam(g_hinstDLL, MAKEINTRESOURCE(IDD_OPTIONS), hwndOwner, SettingsDlgProc, reinterpret_cast<LPARAM>(this));
    return true;
}


// 設定ダイアログプロシージャ
INT_PTR CALLBACK CTVCaption2::SettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG) {
        ::SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
    }
    CTVCaption2 *pThis = reinterpret_cast<CTVCaption2*>(::GetWindowLongPtr(hDlg, GWLP_USERDATA));
    return pThis ? pThis->ProcessSettingsDlg(hDlg, uMsg, wParam, lParam) : FALSE;
}


void CTVCaption2::InitializeSettingsDlg(HWND hDlg)
{
    ::SetProp(hDlg, TEXT("Ini"), reinterpret_cast<HANDLE>(1));

    ::CheckDlgButton(hDlg, IDC_CHECK_OSD,
        ::GetPrivateProfileInt(TEXT("Settings"), TEXT("EnOsdCompositor"), 0, m_szIniPath) != 0 ? BST_CHECKED : BST_UNCHECKED);

    ::SetDlgItemText(hDlg, IDC_EDIT_CAPFOLDER, m_szCaptureFolder);
    ::SendDlgItemMessage(hDlg, IDC_EDIT_CAPFOLDER, EM_LIMITTEXT, _countof(m_szCaptureFolder) - 1, 0);

    int settingsCount = GetSettingsCount();
    ::SendDlgItemMessage(hDlg, IDC_COMBO_SETTINGS, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i < settingsCount; ++i) {
        TCHAR text[32];
        ::wsprintf(text, TEXT("設定%d"), i);
        ::SendDlgItemMessage(hDlg, IDC_COMBO_SETTINGS, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text));
    }
    ::SendDlgItemMessage(hDlg, IDC_COMBO_SETTINGS, CB_SETCURSEL, m_settingsIndex, 0);

    HWND hItem = ::GetDlgItem(hDlg, IDC_COMBO_FACE);
    ::SendMessage(hItem, CB_RESETCONTENT, 0, 0);
    ::SendMessage(hItem, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT(" (指定なし)")));
    AddFaceNameToComboBoxList(hDlg, IDC_COMBO_FACE);
    if (!m_szFaceName[0]) {
        ::SendMessage(hItem, CB_SETCURSEL, 0, 0);
    }
    else if (::SendMessage(hItem, CB_SELECTSTRING, 0, reinterpret_cast<LPARAM>(m_szFaceName)) == CB_ERR) {
        ::SendMessage(hItem, CB_SETCURSEL, ::SendMessage(hItem, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(m_szFaceName)), 0);
    }

    hItem = ::GetDlgItem(hDlg, IDC_COMBO_GAIJI_FACE);
    ::SendMessage(hItem, CB_RESETCONTENT, 0, 0);
    ::SendMessage(hItem, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT(" (ベースフォントを使う)")));
    AddFaceNameToComboBoxList(hDlg, IDC_COMBO_GAIJI_FACE);
    if (!m_szGaijiFaceName[0]) {
        ::SendMessage(hItem, CB_SETCURSEL, 0, 0);
    }
    else if (::SendMessage(hItem, CB_SELECTSTRING, 0, reinterpret_cast<LPARAM>(m_szGaijiFaceName)) == CB_ERR) {
        ::SendMessage(hItem, CB_SETCURSEL, ::SendMessage(hItem, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(m_szGaijiFaceName)), 0);
    }

    static const LPCTSTR GAIJI_TABLE_LIST[] = {
        TEXT("!std"), TEXT("!typebank"), TEXT("arphic"), TEXT("dyna"), TEXT("estima"), TEXT("ricoh"),
        TEXT("ryobi"), TEXT("std"), TEXT("std19"), TEXT("stdpua"), TEXT("typebank"), NULL
    };
    ::SendDlgItemMessage(hDlg, IDC_COMBO_GAIJI_TABLE, CB_RESETCONTENT, 0, 0);
    AddToComboBoxList(hDlg, IDC_COMBO_GAIJI_TABLE, GAIJI_TABLE_LIST);
    ::SetDlgItemText(hDlg, IDC_COMBO_GAIJI_TABLE, m_szGaijiTableName);
    ::SendDlgItemMessage(hDlg, IDC_COMBO_GAIJI_TABLE, CB_LIMITTEXT, _countof(m_szGaijiTableName) - 1, 0);
    ::ShowWindow(::GetDlgItem(hDlg, IDC_STATIC_GAIJI_TABLE), SW_HIDE);

    static const LPCTSTR METHOD_LIST[] = { TEXT("1-通常ウィンドウ"), TEXT("2-レイヤードウィンドウ"), TEXT("3-映像と合成"), NULL };
    ::SendDlgItemMessage(hDlg, IDC_COMBO_METHOD, CB_RESETCONTENT, 0, 0);
    AddToComboBoxList(hDlg, IDC_COMBO_METHOD, METHOD_LIST);
    ::SendDlgItemMessage(hDlg, IDC_COMBO_METHOD, CB_SETCURSEL, min(max(m_paintingMethod-1,0),2), 0);

    ::CheckDlgButton(hDlg, IDC_CHECK_CAPTION, m_showFlags[STREAM_CAPTION] ? BST_CHECKED : BST_UNCHECKED);
    ::CheckDlgButton(hDlg, IDC_CHECK_SUPERIMPOSE, m_showFlags[STREAM_SUPERIMPOSE] ? BST_CHECKED : BST_UNCHECKED);
    ::SetDlgItemInt(hDlg, IDC_EDIT_DELAY, m_delayTime[STREAM_CAPTION], TRUE);
    ::CheckDlgButton(hDlg, IDC_CHECK_IGNORE_PTS, m_fIgnorePts ? BST_CHECKED : BST_UNCHECKED);

    ::CheckDlgButton(hDlg, IDC_CHECK_TEXTOPA, m_textOpacity >= 0 ? BST_CHECKED : BST_UNCHECKED);
    ::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_TEXTOPA), m_textOpacity >= 0);
    ::SetDlgItemInt(hDlg, IDC_EDIT_TEXTOPA, max(m_textOpacity, 0), FALSE);

    ::CheckDlgButton(hDlg, IDC_CHECK_BACKOPA, m_backOpacity >= 0 ? BST_CHECKED : BST_UNCHECKED);
    ::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_BACKOPA), m_backOpacity >= 0);
    ::SetDlgItemInt(hDlg, IDC_EDIT_BACKOPA, max(m_backOpacity, 0), FALSE);

    ::SetDlgItemInt(hDlg, IDC_EDIT_ADJUST_X, m_rcAdjust.left, TRUE);
    ::SetDlgItemInt(hDlg, IDC_EDIT_ADJUST_Y, m_rcAdjust.top, TRUE);
    ::SetDlgItemInt(hDlg, IDC_EDIT_ADJUST_SIZE, m_rcAdjust.right, FALSE);
    ::SetDlgItemInt(hDlg, IDC_EDIT_ADJUST_RATIO, m_rcAdjust.bottom, FALSE);

    ::CheckDlgButton(hDlg, IDC_CHECK_STROKE_WIDTH, m_strokeWidth < 0 ? BST_CHECKED : BST_UNCHECKED);
    ::SetDlgItemInt(hDlg, IDC_EDIT_STROKE_WIDTH, m_strokeWidth < 0 ? -m_strokeWidth : m_strokeWidth, FALSE);

    static const LPCTSTR STROKE_LEVEL_LIST[] = { TEXT("0"), TEXT("1"), TEXT("2"), TEXT("3"), TEXT("4"), TEXT("5"), NULL };
    ::SendDlgItemMessage(hDlg, IDC_COMBO_STROKE_LEVEL, CB_RESETCONTENT, 0, 0);
    AddToComboBoxList(hDlg, IDC_COMBO_STROKE_LEVEL, STROKE_LEVEL_LIST);
    ::SendDlgItemMessage(hDlg, IDC_COMBO_STROKE_LEVEL, CB_SETCURSEL, min(max(m_strokeSmoothLevel,0),5), 0);

    ::SetDlgItemInt(hDlg, IDC_EDIT_BY_DILATE, m_strokeByDilate, FALSE);
    ::SetDlgItemInt(hDlg, IDC_EDIT_VERT_ANTI, m_vertAntiAliasing, FALSE);
    ::CheckDlgButton(hDlg, IDC_CHECK_ADD_PADDING, m_paddingWidth > 0 ? BST_CHECKED : BST_UNCHECKED);

    static const LPCTSTR AVOID_HALF_LIST[] = { TEXT(""), TEXT("A"), TEXT("1"), TEXT("1A"), TEXT("@"), TEXT("@A"), TEXT("@1"), TEXT("@1A"), NULL };
    ::SendDlgItemMessage(hDlg, IDC_COMBO_AVOID_HALF, CB_RESETCONTENT, 0, 0);
    AddToComboBoxList(hDlg, IDC_COMBO_AVOID_HALF, AVOID_HALF_LIST);
    ::SendDlgItemMessage(hDlg, IDC_COMBO_AVOID_HALF, CB_SETCURSEL, m_avoidHalfFlags & 7, 0);

    ::CheckDlgButton(hDlg, IDC_CHECK_IGNORE_SMALL, m_fIgnoreSmall ? BST_CHECKED : BST_UNCHECKED);
    ::EnableWindow(::GetDlgItem(hDlg, IDC_CHECK_SHIFT_SMALL), m_fIgnoreSmall);
    ::CheckDlgButton(hDlg, IDC_CHECK_SHIFT_SMALL, m_fShiftSmall ? BST_CHECKED : BST_UNCHECKED);
    ::CheckDlgButton(hDlg, IDC_CHECK_CENTERING, m_fCentering ? BST_CHECKED : BST_UNCHECKED);
    ::CheckDlgButton(hDlg, IDC_CHECK_SHRINK_SD_SCALE, m_fShrinkSDScale ? BST_CHECKED : BST_UNCHECKED);
    ::SetDlgItemInt(hDlg, IDC_EDIT_ADJUST_VIEW_X, m_adjustViewX, TRUE);
    ::SetDlgItemInt(hDlg, IDC_EDIT_ADJUST_VIEW_Y, m_adjustViewY, TRUE);
    ::CheckDlgButton(hDlg, IDC_CHECK_ROMSOUND, m_szRomSoundList[0] && m_szRomSoundList[0] != TEXT(';') ? BST_CHECKED : BST_UNCHECKED);

    ::RemoveProp(hDlg, TEXT("Ini"));
}


INT_PTR CTVCaption2::ProcessSettingsDlg(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    bool fSave = false;
    bool fConfigureGaiji = false;
    bool fReDisp = false;

    switch (uMsg) {
    case WM_INITDIALOG:
        InitializeSettingsDlg(hDlg);
        return TRUE;
    case WM_COMMAND:
        // 初期化中の無駄な再帰を省く
        if (::GetProp(hDlg, TEXT("Ini"))) {
            break;
        }
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            HideAllOsds();
            ::EndDialog(hDlg, LOWORD(wParam));
            break;
        case IDC_CHECK_OSD:
            WritePrivateProfileInt(TEXT("Settings"), TEXT("EnOsdCompositor"),
                ::IsDlgButtonChecked(hDlg, IDC_CHECK_OSD) != BST_UNCHECKED, m_szIniPath);
            break;
        case IDC_EDIT_CAPFOLDER:
            if (HIWORD(wParam) == EN_CHANGE) {
                ::GetDlgItemText(hDlg, IDC_EDIT_CAPFOLDER, m_szCaptureFolder, _countof(m_szCaptureFolder));
                fSave = true;
            }
            break;
        case IDC_CAPFOLDER_BROWSE:
            {
                TCHAR dir[MAX_PATH], title[64];
                ::GetDlgItemText(hDlg, IDC_EDIT_CAPFOLDER, dir, _countof(dir));
                ::GetDlgItemText(hDlg, IDC_STATIC_CAPFOLDER, title, _countof(title));
                if (BrowseFolderDialog(hDlg, dir, title)) {
                    ::SetDlgItemText(hDlg, IDC_EDIT_CAPFOLDER, dir);
                }
            }
            break;
        case IDC_COMBO_SETTINGS:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                HideAllOsds();
                SwitchSettings(static_cast<int>(::SendDlgItemMessage(hDlg, IDC_COMBO_SETTINGS, CB_GETCURSEL, 0, 0)));
                InitializeSettingsDlg(hDlg);
                fConfigureGaiji = fReDisp = true;
            }
            break;
        case IDC_SETTINGS_ADD:
            HideAllOsds();
            AddSettings();
            InitializeSettingsDlg(hDlg);
            fConfigureGaiji = fReDisp = true;
            break;
        case IDC_SETTINGS_DELETE:
            HideAllOsds();
            DeleteSettings();
            InitializeSettingsDlg(hDlg);
            fConfigureGaiji = fReDisp = true;
            break;
        case IDC_CHECK_SETTINGS_TEST:
            if (::IsDlgButtonChecked(hDlg, IDC_CHECK_SETTINGS_TEST) == BST_UNCHECKED) {
                HideAllOsds();
            }
            fReDisp = true;
            break;
        case IDC_COMBO_FACE:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                ::GetDlgItemText(hDlg, IDC_COMBO_FACE, m_szFaceName, _countof(m_szFaceName));
                if (m_szFaceName[0] == TEXT(' ') && m_szFaceName[1] == TEXT('(')) {
                    m_szFaceName[0] = 0;
                }
                fSave = fReDisp = true;
            }
            break;
        case IDC_COMBO_GAIJI_FACE:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                ::GetDlgItemText(hDlg, IDC_COMBO_GAIJI_FACE, m_szGaijiFaceName, _countof(m_szGaijiFaceName));
                if (m_szGaijiFaceName[0] == TEXT(' ') && m_szGaijiFaceName[1] == TEXT('(')) {
                    m_szGaijiFaceName[0] = 0;
                }
                fSave = fConfigureGaiji = fReDisp = true;
            }
            break;
        case IDC_COMBO_GAIJI_TABLE:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                ::PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(LOWORD(wParam), CBN_EDITCHANGE), 0);
            }
            else if (HIWORD(wParam) == CBN_EDITCHANGE) {
                ::GetDlgItemText(hDlg, IDC_COMBO_GAIJI_TABLE, m_szGaijiTableName, _countof(m_szGaijiTableName));
                ::ShowWindow(::GetDlgItem(hDlg, IDC_STATIC_GAIJI_TABLE), SW_HIDE);
                if (m_szGaijiTableName[0] != TEXT('!')) {
                    // 外字ファイルの存在確認
                    TCHAR gaijiPath[MAX_PATH + LF_FACESIZE + 32];
                    if (GetLongModuleFileName(g_hinstDLL, gaijiPath, MAX_PATH)) {
                        ::PathRemoveExtension(gaijiPath);
                        ::wsprintf(gaijiPath + ::lstrlen(gaijiPath), TEXT("_Gaiji_%s.txt"), m_szGaijiTableName);
                        if (!::PathFileExists(gaijiPath)) {
                            ::ShowWindow(::GetDlgItem(hDlg, IDC_STATIC_GAIJI_TABLE), SW_SHOW);
                        }
                    }
                }
                fSave = fConfigureGaiji = fReDisp = true;
            }
            break;
        case IDC_COMBO_METHOD:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                HideAllOsds();
                m_paintingMethod = static_cast<int>(::SendDlgItemMessage(hDlg, IDC_COMBO_METHOD, CB_GETCURSEL, 0, 0));
                m_paintingMethod = min(max(m_paintingMethod,0),2) + 1;
                fSave = fReDisp = true;
            }
            break;
        case IDC_CHECK_CAPTION:
            HideAllOsds();
            m_showFlags[STREAM_CAPTION] = ::IsDlgButtonChecked(hDlg, IDC_CHECK_CAPTION) != BST_UNCHECKED ? 65535 : 0;
            fSave = fReDisp = true;
            break;
        case IDC_CHECK_SUPERIMPOSE:
            HideAllOsds();
            m_showFlags[STREAM_SUPERIMPOSE] = ::IsDlgButtonChecked(hDlg, IDC_CHECK_SUPERIMPOSE) != BST_UNCHECKED ? 65535 : 0;
            fSave = fReDisp = true;
            break;
        case IDC_EDIT_DELAY:
            if (HIWORD(wParam) == EN_CHANGE) {
                m_delayTime[STREAM_CAPTION] = ::GetDlgItemInt(hDlg, IDC_EDIT_DELAY, NULL, TRUE);
                fSave = true;
            }
            break;
        case IDC_CHECK_IGNORE_PTS:
            m_fIgnorePts = ::IsDlgButtonChecked(hDlg, IDC_CHECK_IGNORE_PTS) != BST_UNCHECKED;
            fSave = true;
            break;
        case IDC_CHECK_TEXTOPA:
            m_textOpacity = ::IsDlgButtonChecked(hDlg, IDC_CHECK_TEXTOPA) != BST_UNCHECKED ? 0 : -1;
            ::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_TEXTOPA), m_textOpacity >= 0);
            ::SetDlgItemInt(hDlg, IDC_EDIT_TEXTOPA, max(m_textOpacity, 0), FALSE);
            fSave = fReDisp = true;
            break;
        case IDC_CHECK_BACKOPA:
            m_backOpacity = ::IsDlgButtonChecked(hDlg, IDC_CHECK_BACKOPA) != BST_UNCHECKED ? 0 : -1;
            ::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_BACKOPA), m_backOpacity >= 0);
            ::SetDlgItemInt(hDlg, IDC_EDIT_BACKOPA, max(m_backOpacity, 0), FALSE);
            fSave = fReDisp = true;
            break;
        case IDC_EDIT_TEXTOPA:
            if (HIWORD(wParam) == EN_CHANGE && ::IsWindowEnabled(::GetDlgItem(hDlg, IDC_EDIT_TEXTOPA))) {
                m_textOpacity = ::GetDlgItemInt(hDlg, IDC_EDIT_TEXTOPA, NULL, FALSE);
                m_textOpacity = min(m_textOpacity, 100);
                fSave = fReDisp = true;
            }
            break;
        case IDC_EDIT_BACKOPA:
            if (HIWORD(wParam) == EN_CHANGE && ::IsWindowEnabled(::GetDlgItem(hDlg, IDC_EDIT_BACKOPA))) {
                m_backOpacity = ::GetDlgItemInt(hDlg, IDC_EDIT_BACKOPA, NULL, FALSE);
                m_backOpacity = min(m_backOpacity, 100);
                fSave = fReDisp = true;
            }
            break;
        case IDC_EDIT_ADJUST_X:
            if (HIWORD(wParam) == EN_CHANGE) {
                m_rcAdjust.left = ::GetDlgItemInt(hDlg, IDC_EDIT_ADJUST_X, NULL, TRUE);
                fSave = fReDisp = true;
            }
            break;
        case IDC_EDIT_ADJUST_Y:
            if (HIWORD(wParam) == EN_CHANGE) {
                m_rcAdjust.top = ::GetDlgItemInt(hDlg, IDC_EDIT_ADJUST_Y, NULL, TRUE);
                fSave = fReDisp = true;
            }
            break;
        case IDC_EDIT_ADJUST_SIZE:
            if (HIWORD(wParam) == EN_CHANGE) {
                m_rcAdjust.right = ::GetDlgItemInt(hDlg, IDC_EDIT_ADJUST_SIZE, NULL, FALSE);
                fSave = fReDisp = true;
            }
            break;
        case IDC_EDIT_ADJUST_RATIO:
            if (HIWORD(wParam) == EN_CHANGE) {
                m_rcAdjust.bottom = ::GetDlgItemInt(hDlg, IDC_EDIT_ADJUST_RATIO, NULL, FALSE);
                fSave = fReDisp = true;
            }
            break;
        case IDC_EDIT_STROKE_WIDTH:
            if (HIWORD(wParam) != EN_CHANGE) break;
            // FALL THROUGH!
        case IDC_CHECK_STROKE_WIDTH:
            m_strokeWidth = ::GetDlgItemInt(hDlg, IDC_EDIT_STROKE_WIDTH, NULL, FALSE);
            m_strokeWidth = ::IsDlgButtonChecked(hDlg, IDC_CHECK_STROKE_WIDTH) != BST_UNCHECKED ? -m_strokeWidth : m_strokeWidth;
            fSave = fReDisp = true;
            break;
        case IDC_COMBO_STROKE_LEVEL:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                m_strokeSmoothLevel = static_cast<int>(::SendDlgItemMessage(hDlg, IDC_COMBO_STROKE_LEVEL, CB_GETCURSEL, 0, 0));
                m_strokeSmoothLevel = min(max(m_strokeSmoothLevel,0),5);
                fSave = fReDisp = true;
            }
            break;
        case IDC_EDIT_BY_DILATE:
            if (HIWORD(wParam) == EN_CHANGE) {
                m_strokeByDilate = ::GetDlgItemInt(hDlg, IDC_EDIT_BY_DILATE, NULL, FALSE);
                fSave = fReDisp = true;
            }
            break;
        case IDC_EDIT_VERT_ANTI:
            if (HIWORD(wParam) == EN_CHANGE) {
                m_vertAntiAliasing = ::GetDlgItemInt(hDlg, IDC_EDIT_VERT_ANTI, NULL, FALSE);
                fSave = fReDisp = true;
            }
            break;
        case IDC_CHECK_ADD_PADDING:
            m_paddingWidth = ::IsDlgButtonChecked(hDlg, IDC_CHECK_ADD_PADDING) != BST_UNCHECKED ? 5 : 0;
            fSave = fReDisp = true;
            break;
        case IDC_COMBO_AVOID_HALF:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                m_avoidHalfFlags = static_cast<int>(::SendDlgItemMessage(hDlg, IDC_COMBO_AVOID_HALF, CB_GETCURSEL, 0, 0)) & 7;
                fSave = fReDisp = true;
            }
            break;
        case IDC_CHECK_IGNORE_SMALL:
            m_fIgnoreSmall = ::IsDlgButtonChecked(hDlg, IDC_CHECK_IGNORE_SMALL) != BST_UNCHECKED;
            ::EnableWindow(::GetDlgItem(hDlg, IDC_CHECK_SHIFT_SMALL), m_fIgnoreSmall);
            // FALL THROUGH!
        case IDC_CHECK_SHIFT_SMALL:
            m_fShiftSmall = m_fIgnoreSmall && ::IsDlgButtonChecked(hDlg, IDC_CHECK_SHIFT_SMALL) != BST_UNCHECKED;
            fSave = fReDisp = true;
            break;
        case IDC_CHECK_CENTERING:
            m_fCentering = ::IsDlgButtonChecked(hDlg, IDC_CHECK_CENTERING) != BST_UNCHECKED;
            fSave = fReDisp = true;
            break;
        case IDC_CHECK_SHRINK_SD_SCALE:
            m_fShrinkSDScale = ::IsDlgButtonChecked(hDlg, IDC_CHECK_SHRINK_SD_SCALE) != BST_UNCHECKED;
            fSave = fReDisp = true;
            break;
        case IDC_EDIT_ADJUST_VIEW_X:
            if (HIWORD(wParam) == EN_CHANGE) {
                m_adjustViewX = ::GetDlgItemInt(hDlg, IDC_EDIT_ADJUST_VIEW_X, NULL, TRUE);
                fSave = fReDisp = true;
            }
            break;
        case IDC_EDIT_ADJUST_VIEW_Y:
            if (HIWORD(wParam) == EN_CHANGE) {
                m_adjustViewY = ::GetDlgItemInt(hDlg, IDC_EDIT_ADJUST_VIEW_Y, NULL, TRUE);
                fSave = fReDisp = true;
            }
            break;
        case IDC_CHECK_ROMSOUND:
            ::lstrcpy(m_szRomSoundList, ::IsDlgButtonChecked(hDlg, IDC_CHECK_ROMSOUND) != BST_UNCHECKED ? ROMSOUND_ENABLED : ROMSOUND_EXAMPLE);
            fSave = true;
            break;
        default:
            return FALSE;
        }
        if (fSave) {
            SaveSettings();
        }
        if (fConfigureGaiji) {
            if (!ConfigureGaijiTable(m_szGaijiTableName, &m_drcsStrMap, m_szGaijiFaceName[0]?m_customGaijiTable:NULL)) {
                m_pApp->AddLog(L"外字テーブルの設定に失敗しました。");
            }
        }
        if (fReDisp && ::IsDlgButtonChecked(hDlg, IDC_CHECK_SETTINGS_TEST) != BST_UNCHECKED) {
            WCHAR gaijiTable[GAIJI_TABLE_SIZE * 2];
            DWORD tableSize = GAIJI_TABLE_SIZE * 2;
            if (m_captionDll.GetGaiji(1, gaijiTable, &tableSize)) {
                // 例示用の外字を適当にえらぶ
                TCHAR testGaiji[64];
                ::lstrcpy(testGaiji, TEXT("外字"));
                for (int i = 0, j = 0; i < GAIJI_TABLE_SIZE; ++i) {
                    WCHAR wc[3] = {};
                    wc[0] = gaijiTable[j++];
                    wc[1] = (wc[0] & 0xFC00) == 0xD800 ? gaijiTable[j++] : 0;
                    if (i % G_CELL_SIZE == 0 || i % (G_CELL_SIZE + 1) == 9) ::lstrcat(testGaiji, wc);
                }
                // テスト字幕を構成
                CAPTION_CHAR_DATA_DLL testCapCharGaiji = TESTCAP_CHAR_LIST[0];
                CAPTION_DATA_DLL testCapGaiji = TESTCAP_LIST[0];
                testCapCharGaiji.pszDecode = testGaiji;
                testCapGaiji.pstCharList = &testCapCharGaiji;
                HideAllOsds();
                ProcessCaption(NULL, &testCapGaiji);
                for (int i = 1; i < _countof(TESTCAP_LIST); ++i) {
                    ProcessCaption(NULL, &TESTCAP_LIST[i]);
                }
            }
        }
        return TRUE;
    }
    return FALSE;
}


TVTest::CTVTestPlugin *CreatePluginClass()
{
    return new CTVCaption2;
}
