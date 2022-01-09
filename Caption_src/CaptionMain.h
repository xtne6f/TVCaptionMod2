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

	LANG_TAG_INFO_DLL m_LangTagList[LANG_TAG_MAX];
	vector<CAPTION_DATA> m_CaptionList[LANG_TAG_MAX + 1];

	vector<DRCS_PATTERN> m_DRCList[LANG_TAG_MAX + 1];
	CDRCMap m_DRCMap[LANG_TAG_MAX + 1];

	unsigned char m_ucDgiGroup;

	int m_iLastCounter;
	BOOL m_bAnalyz; //PES解析中はFALSE
	DWORD m_dwNowReadSize; //読み込んだペイロード長
	DWORD m_dwNeedSize; //PESの解析に必要なペイロード長

	LANG_TAG_INFO_DLL m_LangTagDllList[LANG_TAG_MAX];
	vector<CAPTION_DATA_DLL> m_pCapList;
	vector<CAPTION_CHAR_DATA_DLL> m_pCapCharPool;
	vector<DRCS_PATTERN_DLL> m_pDRCList;
	vector<BYTE> m_pbBuff;

	CARIB8CharDecode m_cDec;

protected:
	DWORD ParseListData();
	DWORD ParseCaption(LPCBYTE pbBuff, DWORD dwSize);
	DWORD ParseCaptionManagementData(LPCBYTE pbBuff, DWORD dwSize, vector<CAPTION_DATA>* pCaptionList, vector<DRCS_PATTERN>* pDRCList, CDRCMap* pDRCMap);
	DWORD ParseCaptionData(LPCBYTE pbBuff, DWORD dwSize, vector<CAPTION_DATA>* pCaptionList,
	                       vector<DRCS_PATTERN>* pDRCList, CDRCMap* pDRCMap, WORD wSWFMode, const char* pszLang, BOOL bUCS);
	DWORD ParseUnitData(LPCBYTE pbBuff, DWORD dwSize, DWORD* pdwReadSize, vector<CAPTION_DATA>* pCaptionList,
	                    vector<DRCS_PATTERN>* pDRCList, CDRCMap* pDRCMap, WORD wSWFMode, const char* pszLang, BOOL bUCS);
};
