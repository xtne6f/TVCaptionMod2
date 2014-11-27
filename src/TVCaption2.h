#ifndef INCLUDE_TV_CAPTION2_H
#define INCLUDE_TV_CAPTION2_H

// プラグインクラス
class CTVCaption2 : public TVTest::CTVTestPlugin
{
    static const int OSD_LIST_MAX = 64;
    // 事前に作成しておくOSDの数(作成時にウィンドウが前面にくるので、気になるなら増やす)
    static const int OSD_PRE_CREATE_NUM = 8;
    static const int GAIJI_TABLE_SIZE = G_CELL_SIZE * 7;
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
    int GetVideoPid();
    bool ConfigureGaijiTable(LPCTSTR tableName, std::vector<DRCS_PAIR> *pDrcsStrMap, WCHAR *pCustomTable);
    bool EnablePlugin(bool fEnable);
    void LoadSettings();
    void SaveSettings() const;
    void SwitchSettings();
    static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
    void OnCapture(bool fSaveToFile);
    static BOOL CALLBACK WindowMsgCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult, void *pUserData);
    void HideOsds(STREAM_INDEX index);
    void DestroyOsds();
    CPseudoOSD &CreateOsd(STREAM_INDEX index, HWND hwndContainer, int charHeight, int nomalHeight, const CAPTION_CHAR_DATA_DLL &style);
    void ShowCaptionData(STREAM_INDEX index, const CAPTION_DATA_DLL &caption, const DRCS_PATTERN_DLL *pDrcsList, DWORD drcsCount, HWND hwndContainer);
    static LRESULT CALLBACK PaintingWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void ProcessCaption(CCaptionManager *pCaptionManager);
    void OnSize(STREAM_INDEX index);
    static BOOL CALLBACK StreamCallback(BYTE *pData, void *pClientData);
    void ProcessPacket(BYTE *pPacket);

    bool m_fTVH264;

    // 設定
    TCHAR m_szIniPath[MAX_PATH];
    TCHAR m_szCaptionDllPath[MAX_PATH];
    TCHAR m_szCaptureFolder[MAX_PATH];
    TCHAR m_szCaptureFileName[64];
    TCHAR m_szFaceName[LF_FACESIZE];
    TCHAR m_szGaijiFaceName[LF_FACESIZE];
    TCHAR m_szGaijiTableName[LF_FACESIZE];
    WCHAR m_customGaijiTable[GAIJI_TABLE_SIZE];
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
    int m_fontSizeAdjust;
    int m_strokeWidth;
    int m_strokeSmoothLevel;
    int m_strokeByDilate;
    bool m_fCentering;
    bool m_fFixRatio;
    TCHAR m_szRomSoundList[32 * 20];

    // 字幕描画
    HWND m_hwndPainting;
    CPseudoOSD m_osdList[OSD_LIST_MAX];
    int m_osdUsedCount;
    CPseudoOSD *m_pOsdUsingList[STREAM_MAX][OSD_LIST_MAX];
    int m_osdUsingCount[STREAM_MAX];
    int m_osdShowCount[STREAM_MAX];
    bool m_fNeedtoShow;

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
};

#endif // INCLUDE_TV_CAPTION2_H
