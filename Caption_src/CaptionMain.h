#pragma once

#define ERR_INIT		10
#define ERR_NOT_INIT	11
//#define ERR_SIZE		12 //これを返すことはない

#define ERR_NEED_NEXT_PACKET 13 //次のTSパケット入れないと解析できない
#define ERR_CAN_NOT_ANALYZ 14 //本当にTSパケット？解析不可能
#define ERR_NOT_FIRST 15 //最初のTSパケット未入力
#define ERR_INVALID_PACKET 16 //本当にTSパケット？パケット飛んで壊れてるかも

#define CHANGE_VERSION 20
#define NO_ERR_TAG_INFO 21
#define NO_ERR_CAPTION_1 22
#define NO_ERR_CAPTION_8 29

#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define RESET_JD_SYSTEMTIME(p)  { if(p) { *p.wYear = 1991; *p.wMonth = 12; *p.wDay = 31; *p.wHour = 0; *p.wMinute = 0; *p.wSecond = 0; *p.wMilliseconds = 0; *p.wDayOfWeek = 0; } }
#define RESET_MJD_SYSTEMTIME(p)  { if(p) { *p.wYear = 1858; *p.wMonth = 11; *p.wDay = 17; *p.wHour = 0; *p.wMinute = 0; *p.wSecond = 0; *p.wMilliseconds = 0; *p.wDayOfWeek = 0; } }

//運用規定により2言語まで
#define LANG_TAG_MAX 2

class CCaptionMain
{
public:
	CCaptionMain(void);
	~CCaptionMain(void);

	DWORD AddTSPacket(BYTE* pbPacket);
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
	CAPTION_DATA_DLL *m_pCapList;
	CAPTION_CHAR_DATA_DLL *m_pCapCharPool;
	DRCS_PATTERN_DLL *m_pDRCList;
	BYTE *m_pbBuff;

	DWORD m_dwCapListSize;
	DWORD m_dwCapCharPoolSize;
	DWORD m_dwDRCListSize;
	DWORD m_dwBuffSize;

	CARIB8CharDecode m_cDec;

protected:
	DWORD ParseListData();
	DWORD ParseCaption(BYTE* pbBuff, DWORD dwSize);
	DWORD ParseCaptionManagementData(BYTE* pbBuff, DWORD dwSize, vector<CAPTION_DATA>* pCaptionList, vector<DRCS_PATTERN>* pDRCList, CDRCMap* pDRCMap);
	DWORD ParseCaptionData(BYTE* pbBuff, DWORD dwSize, vector<CAPTION_DATA>* pCaptionList, vector<DRCS_PATTERN>* pDRCList, CDRCMap* pDRCMap, WORD wSWFMode);
	DWORD ParseUnitData(BYTE* pbBuff, DWORD dwSize, DWORD* pdwReadSize, vector<CAPTION_DATA>* pCaptionList, vector<DRCS_PATTERN>* pDRCList, CDRCMap* pDRCMap, WORD wSWFMode);
};
