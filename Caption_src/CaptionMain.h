#pragma once

//運用規定により2言語まで
#define LANG_TAG_MAX 2

class CCaptionMain
{
public:
	CCaptionMain(void);

	DWORD AddTSPacket(LPCBYTE pbPacket);
	DWORD Clear();

	DWORD GetTagInfo(LANG_TAG_INFO_DLL** ppList, DWORD* pdwListCount);
	DWORD GetCaptionData(unsigned char ucLangTag, CAPTION_DATA_DLL** ppList, DWORD* pdwListCount);
	BOOL GetDRCSPattern(unsigned char ucLangTag, DRCS_PATTERN_DLL** ppList, DWORD* pdwListCount);
	BOOL GetBitmapData(unsigned char ucLangTag, BITMAP_DATA_DLL** ppList, DWORD* pdwListCount);
	BOOL GetGaijiTable(WCHAR* pTable, DWORD* pdwTableSize) const { return m_cDec.GetGaijiTable(pTable, pdwTableSize); }
	BOOL SetGaijiTable(const WCHAR* pTable, DWORD* pdwTableSize) { return m_cDec.SetGaijiTable(pTable, pdwTableSize); }
	BOOL ResetGaijiTable(DWORD* pdwTableSize) { return m_cDec.ResetGaijiTable(pdwTableSize); }

protected:
	struct PAYLOAD_DATA{
		BYTE bBuff[188];
		WORD wSize;
		PAYLOAD_DATA() {}
	};
	vector<PAYLOAD_DATA> m_PayloadList;

	struct BITMAP_DATA{
		WORD wAppearanceOrder;
		WORD wSWFMode;
		WORD wClientX;
		WORD wClientY;
		WORD wClientW;
		WORD wClientH;
		int iPosX;
		int iPosY;
		vector<CLUT_DAT_DLL> FlushColor;
		vector<BYTE> Image;
	};

	struct LANG_CONTEXT{
		LANG_TAG_INFO_DLL LangTag;
		vector<CAPTION_DATA> CaptionList;
		BOOL bHasStatementBody;
		CAPTION_DATA LastCaptionDataFields;
		vector<DRCS_PATTERN> DRCList;
		CDRCMap DRCMap;
		vector<BITMAP_DATA> BitmapDataList;
	};
	LANG_CONTEXT m_LangContext[LANG_TAG_MAX];
	LANG_CONTEXT m_ManagementContext;

	unsigned char m_ucDgiGroup;

	int m_iLastCounter;
	BOOL m_bAnalyz; //PES解析中はFALSE
	DWORD m_dwNowReadSize; //読み込んだペイロード長
	DWORD m_dwNeedSize; //PESの解析に必要なペイロード長

	LANG_TAG_INFO_DLL m_LangTagDllList[LANG_TAG_MAX];
	vector<CAPTION_DATA_DLL> m_pCapList;
	vector<CAPTION_CHAR_DATA_DLL> m_pCapCharPool;
	vector<DRCS_PATTERN_DLL> m_pDRCList;
	vector<BITMAP_DATA_DLL> m_pBitmapDataList;
	vector<BYTE> m_pbBuff;

	CARIB8CharDecode m_cDec;

protected:
	DWORD ParseListData();
	DWORD ParseCaption(LPCBYTE pbBuff, DWORD dwSize);
	DWORD ParseCaptionManagementData(LPCBYTE pbBuff, DWORD dwSize);
	DWORD ParseCaptionData(LPCBYTE pbBuff, DWORD dwSize, LANG_CONTEXT* pLangContext);
	DWORD ParseUnitData(LPCBYTE pbBuff, DWORD dwSize, DWORD* pdwReadSize, LANG_CONTEXT* pLangContext);
};
