#ifndef INCLUDE_TV_CAPTION2_H
#define INCLUDE_TV_CAPTION2_H

// プラグインクラス
class CTVCaption2 : public TVTest::CTVTestPlugin
{
    static const int OSD_LIST_MAX = 64;
    static const int PCR_PER_MSEC = 45;
    static const int PCR_PIDS_MAX = 8;
    static const int PACKET_QUEUE_SIZE = 1024;
public:
    // CTVTestPlugin
    CTVCaption2();
    bool GetPluginInfo(TVTest::PluginInfo *pInfo);
    bool Initialize();
    bool Finalize();
private:
    HWND GetFullscreenWindow();
    HWND FindVideoContainer();
    int GetSubtitlePid();
    bool InitializeCaptionDll();
    bool ConfigureGaijiTable(LPCTSTR tableName);
    bool EnablePlugin(bool fEnable);
    void LoadSettings();
    void SaveSettings() const;
    void SwitchSettings();
    static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
    static BOOL CALLBACK WindowMsgCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult, void *pUserData);
    void HideOsds();
    void DestroyOsds();
    CPseudoOSD &CreateOsd(HWND hwndContainer, int charHeight, int nomalHeight, const CAPTION_CHAR_DATA_DLL &style);
    void ShowCaptionData(const CAPTION_DATA_DLL &caption, const DRCS_PATTERN_DLL *pDrcsList, DWORD drcsCount, HWND hwndContainer);
    static LRESULT CALLBACK PaintingWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void GetNextCaption(CAPTION_DATA_DLL **ppCapList, DWORD *pCapCount, DRCS_PATTERN_DLL **ppDrcsList, DWORD *pDrcsCount);
    static BOOL CALLBACK StreamCallback(BYTE *pData, void *pClientData);
    void ProcessPacket(BYTE *pPacket);

    // 設定
    TCHAR m_szIniPath[MAX_PATH];
    TCHAR m_szCaptionDllPath[MAX_PATH];
    TCHAR m_szFaceName[LF_FACESIZE];
    TCHAR m_szGaijiTableName[LF_FACESIZE];
    int m_settingsIndex;
    int m_paintingMethod;
    int m_delayTime;
    bool m_fEnTextColor;
    bool m_fEnBackColor;
    COLORREF m_textColor;
    COLORREF m_backColor;
    int m_textOpacity;
    int m_backOpacity;
    int m_strokeWidth;
    int m_strokeSmoothLevel;
    int m_strokeByDilate;
    bool m_fCentering;
    bool m_fFixRatio;

    // 字幕描画
    HWND m_hwndPainting;
    CPseudoOSD m_osdList[OSD_LIST_MAX];
    int m_osdShowCount;
    bool m_fNeedtoShow;
    bool m_fShowLang2;

    // 字幕解析
    bool m_fEnCaptionPts;
    DWORD m_captionPts;
    CAPTION_DATA_DLL *m_pCapList;
    DRCS_PATTERN_DLL *m_pDrcsList;
    DWORD m_capCount;
    DWORD m_drcsCount;
    LANG_TAG_INFO_DLL m_lang1, m_lang2;

    // ストリーム解析
    CCriticalLock m_streamLock;
    DWORD m_pcr;
    DWORD m_procCapTick;
    int m_pcrPid, m_pcrPids[PCR_PIDS_MAX];
    int m_pcrPidCounts[PCR_PIDS_MAX];
    int m_pcrPidsLen;
    BYTE m_packetQueue[PACKET_QUEUE_SIZE][188];
    int m_packetQueueFront;
    int m_packetQueueRear;
    int m_captionPid;

    // DLL
    HMODULE m_hCaptionDll;
    UnInitializeCP *m_pfnUnInitializeCP;
    AddTSPacketCP *m_pfnAddTSPacketCP;
    ClearCP *m_pfnClearCP;
    GetTagInfoCP *m_pfnGetTagInfoCP;
    GetCaptionDataCPW *m_pfnGetCaptionDataCPW;
    GetDRCSPatternCP *m_pfnDRCSPatternCP;
    SetGaijiCP *m_pfnSetGaijiCP;
};

#endif // INCLUDE_TV_CAPTION2_H
