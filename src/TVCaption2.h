#ifndef INCLUDE_TV_CAPTION2_H
#define INCLUDE_TV_CAPTION2_H

// プラグインクラス
class CTVCaption2 : public TVTest::CTVTestPlugin
{
    // 作成できるOSDの最大数
    static const size_t OSD_MAX_CREATE_NUM = 50;
    // 事前に作成しておくOSDの数(作成時にウィンドウが前面にくるので、気になるなら増やす)
    static const size_t OSD_PRE_CREATE_NUM = 12;
    // 設定値の最大読み込み文字数
    static const int SETTING_VALUE_MAX = 2048;
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
        struct COMPARE {
            bool operator()(const DRCS_PAIR &l, const DRCS_PAIR &r) { return ::memcmp(l.md5, r.md5, 16) < 0; }
        };
    };
    struct SHIFT_SMALL_STATE {
        int posY;
        int shiftH;
        int dirH;
        bool fSmall;
        SHIFT_SMALL_STATE() : posY(-1), shiftH(0), dirH(0) {}
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
    bool GetVideoContainerLayout(HWND hwndContainer, RECT *pRect, RECT *pVideoRect = nullptr, RECT *pExVideoRect = nullptr);
    bool GetVideoSurfaceRect(HWND hwndContainer, RECT *pVideoRect = nullptr, RECT *pExVideoRect = nullptr);
    int GetVideoPid();
    bool ConfigureGaijiTable(LPCTSTR tableName, std::vector<DRCS_PAIR> *pDrcsStrMap, WCHAR (*pCustomTable)[2]);
    bool EnablePlugin(bool fEnable);
    void LoadSettings();
    void SaveSettings() const;
    void SwitchSettings(int specIndex = -1);
    void AddSettings();
    void DeleteSettings();
    int GetSettingsCount() const;
    bool PlayRomSound(int index) const;
    static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
    bool LoadTVTestImageLibrary();
    void OnCapture(bool fSaveToFile);
    static BOOL CALLBACK WindowMsgCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult, void *pUserData);
    void HideOsds(STREAM_INDEX index);
    void DeleteTextures();
    void HideAllOsds();
    void DestroyOsds();
    void AddOsdText(CPseudoOSD *pOsd, LPCTSTR text, int width, int originalWidth, int charWidth, int charHeight,
                    const RECT &rcFontAdjust, LPCTSTR faceName, const CAPTION_CHAR_DATA_DLL &style) const;
    CPseudoOSD &CreateOsd(STREAM_INDEX index, HWND hwndContainer, int charHeight, int nomalHeight, const CAPTION_CHAR_DATA_DLL &style);
    void DryrunCaptionData(const CAPTION_DATA_DLL &caption, SHIFT_SMALL_STATE &ssState);
    void SetOsdWindowOffsetAndScale(CPseudoOSD *pOsd, const RECT &rcVideo) const;
    void ShowCaptionData(STREAM_INDEX index, const CAPTION_DATA_DLL &caption, bool fLangCodeJpn,
                         const DRCS_PATTERN_DLL *pDrcsList, DWORD drcsCount,
                         SHIFT_SMALL_STATE &ssState, HWND hwndContainer, const RECT &rcVideo);
    void ShowBitmapData(STREAM_INDEX index, const BITMAP_DATA_DLL &bitmapData, HWND hwndContainer, const RECT &rcVideo);
    static LRESULT CALLBACK PaintingWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void ProcessCaption(CCaptionManager *pCaptionManager, const CAPTION_DATA_DLL *pCaptionForTest = nullptr);
    void OnSize(STREAM_INDEX index, bool fFast = false);
    void OnFullscreenChange();
    static BOOL CALLBACK StreamCallback(BYTE *pData, void *pClientData);
    static LRESULT CALLBACK VideoStreamCallback(DWORD Format, const void *pData, SIZE_T Size, void *pClientData);
    void ProcessPacket(BYTE *pPacket);
    bool PluginSettings(HWND hwndOwner);
    static INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK TVTestSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, void *pClientData);
    void InitializeSettingsDlg(HWND hDlg);
    INT_PTR ProcessSettingsDlg(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // 設定
    tstring m_iniPath;
    tstring m_captureFolder;
    tstring m_captureFileName;
    tstring m_captureFileNameFormat;
    TCHAR m_szCaptureSaveFormat[8];
    int m_jpegQuality;
    int m_pngCompressionLevel;
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
    int m_ornStrokeWidth;
    int m_strokeSmoothLevel;
    int m_strokeByDilate;
    int m_paddingWidth;
    int m_avoidHalfFlags;
    bool m_fIgnoreSmall;
    bool m_fShiftSmall;
    bool m_fCentering;
    bool m_fShrinkSDScale;
    int m_adjustViewX;
    int m_adjustViewY;
    tstring m_romSoundList;
    bool m_fInitializeSettingsDlg;

    // コマンド
    HBITMAP m_hbmSwitchLang;
    HBITMAP m_hbmSwitchSetting;

    // キャプチャなど
    bool m_fDoneLoadTVTestImage;
    HMODULE m_hTVTestImage;

    // 字幕描画
    HWND m_hwndPainting;
    HWND m_hwndContainer;
    std::vector<std::unique_ptr<CPseudoOSD>> m_pOsdList[STREAM_MAX];
    size_t m_osdShowCount[STREAM_MAX];
    size_t m_osdPrepareCount[STREAM_MAX];
    bool m_fOsdClear[STREAM_MAX];
    bool m_fNeedtoShow;
    bool m_fFlashingFlipFlop;
    bool m_fProfileC;
    SHIFT_SMALL_STATE m_shiftSmallState[STREAM_MAX];

    // 字幕解析
    CCaptionDll m_captionDll;
    CCaptionManager m_caption1Manager;
    CCaptionManager m_caption2Manager;

    // ストリーム解析
    recursive_mutex_ m_streamLock;
    DWORD m_procCapTick;
    bool m_fResetPat;
    PAT m_pat;
    int m_videoPid;
    int m_pcrPid;
    int m_caption1Pid;
    int m_caption2Pid;

    // レンダラで合成する(疑似でない)OSD
    COsdCompositor m_osdCompositor;

    // ビューアPCRの推定用
    CViewerClockEstimator m_viewerClockEstimator;
};

#endif // INCLUDE_TV_CAPTION2_H
