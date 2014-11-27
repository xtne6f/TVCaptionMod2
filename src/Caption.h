#ifndef INCLUDE_CAPTION_H
#define INCLUDE_CAPTION_H

// 名前衝突的に危ういので接頭辞CP_を付けさせてもらう

#define CP_ERR_INIT             10
#define CP_ERR_NOT_INIT         11

#define CP_ERR_NEED_NEXT_PACKET 13      // 次のTSパケット入れないと解析できない
#define CP_ERR_CAN_NOT_ANALYZ   14      // 本当にTSパケット？解析不可能
#define CP_ERR_NOT_FIRST        15      // 最初のTSパケット未入力
#define CP_ERR_INVALID_PACKET   16      // 本当にTSパケット？パケット飛んで壊れてるかも

#define CP_CHANGE_VERSION       20
#define CP_NO_ERR_TAG_INFO      21
#define CP_NO_ERR_CAPTION_1     22
#define CP_NO_ERR_CAPTION_8     29

#define G_CELL_SIZE 94

typedef enum {
    CP_STR_SMALL = 0,   // SSZ
    CP_STR_MEDIUM,      // MSZ
    CP_STR_NORMAL,      // NSZ
    CP_STR_MICRO,       // SZX 0x60
    CP_STR_HIGH_W,      // SZX 0x41
    CP_STR_WIDTH_W,     // SZX 0x44
    CP_STR_W,           // SZX 0x45
    CP_STR_SPECIAL_1,   // SZX 0x6B
    CP_STR_SPECIAL_2,   // SZX 0x64
} CP_STRING_SIZE;

typedef struct _CLUT_DAT_DLL{
	unsigned char ucR;
	unsigned char ucG;
	unsigned char ucB;
	unsigned char ucAlpha;
	bool operator==(const struct _CLUT_DAT_DLL &other) const {
		return ucR==other.ucR && ucG==other.ucG && ucB==other.ucB && ucAlpha==other.ucAlpha;
	}
} CLUT_DAT_DLL;

typedef struct _CAPTION_CHAR_DATA_DLL{
	const WCHAR* pszDecode;
	DWORD wCharSizeMode;

	CLUT_DAT_DLL stCharColor;
	CLUT_DAT_DLL stBackColor;
	CLUT_DAT_DLL stRasterColor;

	BOOL bUnderLine;
	BOOL bShadow;
	BOOL bBold;
	BOOL bItalic;
	BYTE bFlushMode;
	BYTE bHLC; //must ignore low 4bits

	WORD wCharW;
	WORD wCharH;
	WORD wCharHInterval;
	WORD wCharVInterval;
	BYTE bPRA; //PRA+1
	BYTE bAlignment; //zero cleared
} CAPTION_CHAR_DATA_DLL;

typedef struct _CAPTION_DATA_DLL{
	BOOL bClear;
	WORD wSWFMode;
	WORD wClientX;
	WORD wClientY;
	WORD wClientW;
	WORD wClientH;
	WORD wPosX;
	WORD wPosY;
	WORD wAlignment; //zero cleared
	DWORD dwListCount;
	CAPTION_CHAR_DATA_DLL* pstCharList;
	DWORD dwWaitTime;
} CAPTION_DATA_DLL;

typedef struct _LANG_TAG_INFO_DLL{
	unsigned char ucLangTag;
	unsigned char ucDMF;
	unsigned char ucDC;
	char szISOLangCode[4];
	unsigned char ucFormat;
	unsigned char ucTCS;
	unsigned char ucRollupMode;
}LANG_TAG_INFO_DLL;

typedef struct _DRCS_PATTERN_DLL{
	DWORD dwDRCCode;
	DWORD dwUCS;
	WORD wGradation;
	WORD wReserved; //zero cleared
	DWORD dwReserved; //zero cleared
	BITMAPINFOHEADER bmiHeader;
	const BYTE* pbBitmap;
}DRCS_PATTERN_DLL;

typedef DWORD (WINAPI SwitchStreamCP)(DWORD dwIndex);
typedef DWORD (WINAPI InitializeCP)();
typedef DWORD (WINAPI InitializeCPW)();
typedef DWORD (WINAPI InitializeUNICODE)();
typedef DWORD (WINAPI UnInitializeCP)();
typedef DWORD (WINAPI AddTSPacketCP)(BYTE* pbPacket);
typedef DWORD (WINAPI ClearCP)();
typedef DWORD (WINAPI GetTagInfoCP)(LANG_TAG_INFO_DLL** ppList, DWORD* pdwListCount);
typedef DWORD (WINAPI GetCaptionDataCP)(unsigned char ucLangTag, CAPTION_DATA_DLL** ppList, DWORD* pdwListCount);
typedef DWORD (WINAPI GetCaptionDataCPW)(unsigned char ucLangTag, CAPTION_DATA_DLL** ppList, DWORD* pdwListCount);
typedef DWORD (WINAPI GetDRCSPatternCP)(unsigned char ucLangTag, DRCS_PATTERN_DLL** ppList, DWORD* pdwListCount);
typedef DWORD (WINAPI SetGaijiCP)(DWORD dwCommand, const WCHAR* ppTable, DWORD* pdwTableSize);
typedef DWORD (WINAPI GetGaijiCP)(DWORD dwCommand, WCHAR* ppTable, DWORD* pdwTableSize);

BOOL CalcMD5FromDRCSPattern(BYTE *pbHash, const DRCS_PATTERN_DLL *pPattern);

// 2ストリーム(字幕+文字スーパーなど)をまとめて扱うラッパークラス

class CCaptionDll {
public:
    CCaptionDll() : m_hDll(NULL) {}
    bool IsInitialized() const { return m_hDll != NULL; }
    bool Initialize(LPCTSTR pszPath) {
        if (m_hDll) return true;
        if ((m_hDll = ::LoadLibrary(pszPath)) == NULL) return false;
        InitializeCPW *pfnInitializeCPW;
        if ((pfnInitializeCPW       = reinterpret_cast<InitializeCPW*>(::GetProcAddress(m_hDll, "InitializeCPW"))) != NULL &&
            (m_pfnSwitchStreamCP    = reinterpret_cast<SwitchStreamCP*>(::GetProcAddress(m_hDll, "SwitchStreamCP"))) != NULL &&
            (m_pfnUnInitializeCP    = reinterpret_cast<UnInitializeCP*>(::GetProcAddress(m_hDll, "UnInitializeCP"))) != NULL &&
            (m_pfnAddTSPacketCP     = reinterpret_cast<AddTSPacketCP*>(::GetProcAddress(m_hDll, "AddTSPacketCP"))) != NULL &&
            (m_pfnClearCP           = reinterpret_cast<ClearCP*>(::GetProcAddress(m_hDll, "ClearCP"))) != NULL &&
            (m_pfnGetTagInfoCP      = reinterpret_cast<GetTagInfoCP*>(::GetProcAddress(m_hDll, "GetTagInfoCP"))) != NULL &&
            (m_pfnGetCaptionDataCPW = reinterpret_cast<GetCaptionDataCPW*>(::GetProcAddress(m_hDll, "GetCaptionDataCPW"))) != NULL &&
            (m_pfnGetDRCSPatternCP  = reinterpret_cast<GetDRCSPatternCP*>(::GetProcAddress(m_hDll, "GetDRCSPatternCP"))) != NULL &&
            (m_pfnSetGaijiCP        = reinterpret_cast<SetGaijiCP*>(::GetProcAddress(m_hDll, "SetGaijiCP"))) != NULL &&
            (m_pfnGetGaijiCP        = reinterpret_cast<GetGaijiCP*>(::GetProcAddress(m_hDll, "GetGaijiCP"))) != NULL &&
            SwitchStream(1) && pfnInitializeCPW() == TRUE &&
            SwitchStream(0) && pfnInitializeCPW() == TRUE)
        {
            return true;
        }
        ::FreeLibrary(m_hDll);
        m_hDll = NULL;
        return false;
    }
    void UnInitialize() {
        if (m_hDll) {
            if (SwitchStream(1)) m_pfnUnInitializeCP();
            if (SwitchStream(0)) m_pfnUnInitializeCP();
            ::FreeLibrary(m_hDll);
            m_hDll = NULL;
        }
    }
    DWORD AddTSPacket(DWORD dwIndex, BYTE *pbPacket) const {
        return SwitchStream(dwIndex) ? m_pfnAddTSPacketCP(pbPacket) : FALSE;
    }
    DWORD Clear(DWORD dwIndex) const {
        return SwitchStream(dwIndex) ? m_pfnClearCP() : FALSE;
    }
    DWORD GetTagInfo(DWORD dwIndex, LANG_TAG_INFO_DLL** ppList, DWORD* pdwListCount) const {
        return SwitchStream(dwIndex) ? m_pfnGetTagInfoCP(ppList, pdwListCount) : FALSE;
    }
    DWORD GetCaptionData(DWORD dwIndex, unsigned char ucLangTag, CAPTION_DATA_DLL** ppList, DWORD* pdwListCount) const {
        return SwitchStream(dwIndex) ? m_pfnGetCaptionDataCPW(ucLangTag, ppList, pdwListCount) : FALSE;
    }
    DWORD GetDRCSPattern(DWORD dwIndex, unsigned char ucLangTag, DRCS_PATTERN_DLL** ppList, DWORD* pdwListCount) const {
        return SwitchStream(dwIndex) ? m_pfnGetDRCSPatternCP(ucLangTag, ppList, pdwListCount) : FALSE;
    }
    bool SetGaiji(DWORD dwCommand, const WCHAR* ppTable, DWORD* pdwTableSize) const {
        DWORD dwTableSize = *pdwTableSize;
        return SwitchStream(1) && m_pfnSetGaijiCP(dwCommand, ppTable, &dwTableSize) == TRUE &&
               SwitchStream(0) && m_pfnSetGaijiCP(dwCommand, ppTable, pdwTableSize) == TRUE;
    }
    bool GetGaiji(DWORD dwCommand, WCHAR* ppTable, DWORD* pdwTableSize) const {
        return SwitchStream(0) && m_pfnGetGaijiCP(dwCommand, ppTable, pdwTableSize) == TRUE;
    }
private:
    bool SwitchStream(DWORD dwIndex) const {
        return m_hDll && dwIndex < 2 && m_pfnSwitchStreamCP(dwIndex) == TRUE;
    }
    HMODULE m_hDll;
    SwitchStreamCP *m_pfnSwitchStreamCP;
    UnInitializeCP *m_pfnUnInitializeCP;
    AddTSPacketCP *m_pfnAddTSPacketCP;
    ClearCP *m_pfnClearCP;
    GetTagInfoCP *m_pfnGetTagInfoCP;
    GetCaptionDataCPW *m_pfnGetCaptionDataCPW;
    GetDRCSPatternCP *m_pfnGetDRCSPatternCP;
    SetGaijiCP *m_pfnSetGaijiCP;
    GetGaijiCP *m_pfnGetGaijiCP;
};

#endif // INCLUDE_CAPTION_H
