#ifndef INCLUDE_TV_CAPTION2_H
#define INCLUDE_TV_CAPTION2_H

// プラグインクラス
class CTVCaption2 : public TVTest::CTVTestPlugin
{
    static const int OSD_LIST_MAX = 96;
    // 事前に作成しておくOSDの数(作成時にウィンドウが前面にくるので、気になるなら増やす)
    static const int OSD_PRE_CREATE_NUM = 12;
    static const int GAIJI_TABLE_SIZE = G_CELL_SIZE * 7;
    static const int FLASHING_INTERVAL = 500;
    static const int TXGROUP_NORMAL = 1;
    static const int TXGROUP_FLASHING = 3;
    static const int TXGROUP_IFLASHING = 5;
    static const int STREAM_MAX = 2;
    enum STREAM_INDEX {
        STREAM_CAPTION,
        STREAM_SUPERIMPOSE,
    };
    struct DRCS_PAIR {
        BYTE md5[16];
        TCHAR str[4];
    };
public:
    // CTVTestPlugin
    CTVCaption2();
    ~CTVCaption2();
    bool GetPluginInfo(TVTest::PluginInfo *pInfo);
    bool Initialize();
    bool Finalize();
private:
    HWND GetFullscreenWindow();
    HWND FindVideoContainer();
    bool GetVideoContainerLayout(HWND hwndContainer, RECT *pRect, RECT *pVideoRect = NULL, RECT *pExVideoRect = NULL);
    int GetVideoPid();
    bool ConfigureGaijiTable(LPCTSTR tableName, std::vector<DRCS_PAIR> *pDrcsStrMap, WCHAR (*pCustomTable)[2]);
    bool EnablePlugin(bool fEnable);
    void LoadSettings();
    void SaveSettings() const;
    void SwitchSettings(int specIndex = -1);
    void AddSettings();
    void DeleteSettings();
    int GetSettingsCount() const;
    static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
    void OnCapture(bool fSaveToFile);
    static BOOL CALLBACK WindowMsgCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult, void *pUserData);
    void HideOsds(STREAM_INDEX index);
    void DeleteTextures();
    void HideAllOsds();
    void DestroyOsds();
    CPseudoOSD &CreateOsd(STREAM_INDEX index, HWND hwndContainer, int charHeight, int nomalHeight, const CAPTION_CHAR_DATA_DLL &style);
    void ShowCaptionData(STREAM_INDEX index, const CAPTION_DATA_DLL &caption, const DRCS_PATTERN_DLL *pDrcsList, DWORD drcsCount,
                         HWND hwndContainer, const RECT &rcVideo);
    static LRESULT CALLBACK PaintingWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void ProcessCaption(CCaptionManager *pCaptionManager, const CAPTION_DATA_DLL *pCaptionForTest = NULL);
    void OnSize(STREAM_INDEX index);
    static BOOL CALLBACK StreamCallback(BYTE *pData, void *pClientData);
    void ProcessPacket(BYTE *pPacket);
    bool PluginSettings(HWND hwndOwner);
    static INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void InitializeSettingsDlg(HWND hDlg);
    INT_PTR ProcessSettingsDlg(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    bool m_fTVH264;
    bool m_fSettingsDlgInitializing;

    // 設定
    TCHAR m_szIniPath[MAX_PATH];
    TCHAR m_szCaptionDllPath[MAX_PATH];
    TCHAR m_szCaptureFolder[MAX_PATH];
    TCHAR m_szCaptureFileName[64];
    TCHAR m_szFaceName[LF_FACESIZE];
    TCHAR m_szGaijiFaceName[LF_FACESIZE];
    TCHAR m_szGaijiTableName[LF_FACESIZE];
    WCHAR m_customGaijiTable[GAIJI_TABLE_SIZE][2];
    std::vector<DRCS_PAIR> m_drcsStrMap; 
    int m_settingsIndex;
    int m_paintingMethod;
    int m_showFlags[STREAM_MAX];
    int m_delayTime[STREAM_MAX];
    bool m_fIgnorePts;
    bool m_fEnTextColor;
    bool m_fEnBackColor;
    COLORREF m_textColor;
    COLORREF m_backColor;
    int m_textOpacity;
    int m_backOpacity;
    int m_vertAntiAliasing;
    RECT m_rcAdjust;
    RECT m_rcGaijiAdjust;
    int m_strokeWidth;
    int m_strokeSmoothLevel;
    int m_strokeByDilate;
    int m_paddingWidth;
    bool m_fIgnoreSmall;
    bool m_fCentering;
    TCHAR m_szRomSoundList[32 * 20];

    // 字幕描画
    HWND m_hwndPainting;
    HWND m_hwndContainer;
    CPseudoOSD m_osdList[OSD_LIST_MAX];
    int m_osdUsedCount;
    std::vector<CPseudoOSD*> m_pOsdUsingList[STREAM_MAX];
    int m_osdShowCount[STREAM_MAX];
    int m_osdPrepareCount[STREAM_MAX];
    bool m_fOsdClear[STREAM_MAX];
    bool m_fNeedtoShow;
    bool m_fFlashingFlipFlop;

    // 字幕解析
    CCaptionDll m_captionDll;
    CCaptionManager m_caption1Manager;
    CCaptionManager m_caption2Manager;

    // ストリーム解析
    CCriticalLock m_streamLock;
    DWORD m_pcr;
    DWORD m_procCapTick;
    bool m_fResetPat;
    PAT m_pat;
    int m_videoPid;
    int m_pcrPid;
    int m_caption1Pid;
    int m_caption2Pid;

    // レンダラで合成する(疑似でない)OSD
    COsdCompositor m_osdCompositor;
};

#endif // INCLUDE_TV_CAPTION2_H
