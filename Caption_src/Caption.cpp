// Caption.cpp : DLL アプリケーションのエントリ ポイントを定義します。
//
#include <Windows.h>
#include "CaptionDef.h"
#include "ARIB8CharDecode.h"
#include "CaptionMain.h"
#include "Caption.h"

#ifdef CAPTION_EXPORTS
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    return TRUE;
}
#endif

typedef enum{
	MODE_ACP, MODE_UTF16, MODE_UTF8
} CP_MODE;

#define SWITCH_STREAM_MAX 8

static DWORD g_dwIndex;
static CCaptionMain* g_sysList[SWITCH_STREAM_MAX];
static char* g_pPoolList[SWITCH_STREAM_MAX];
static DWORD g_dwPoolSizeList[SWITCH_STREAM_MAX];
static CP_MODE g_charModeList[SWITCH_STREAM_MAX];

static CCaptionMain* g_sys;
static char* g_pPool;
static DWORD g_dwPoolSize;
static CP_MODE g_charMode;

DWORD WINAPI SwitchStreamCP(DWORD dwIndex)
{
	if( dwIndex >= SWITCH_STREAM_MAX ){
		return FALSE;
	}
	g_sysList[g_dwIndex] = g_sys;
	g_pPoolList[g_dwIndex] = g_pPool;
	g_dwPoolSizeList[g_dwIndex] = g_dwPoolSize;
	g_charModeList[g_dwIndex] = g_charMode;

	g_sys = g_sysList[dwIndex];
	g_pPool = g_pPoolList[dwIndex];
	g_dwPoolSize = g_dwPoolSizeList[dwIndex];
	g_charMode = g_charModeList[dwIndex];

	g_dwIndex = dwIndex;
	return TRUE;
}

//DLLの初期化
//戻り値：エラーコード
DWORD WINAPI InitializeCP()
{
	if( g_sys != NULL ){
		return CP_ERR_INIT;
	}
	g_sys = new CCaptionMain;
	g_charMode = MODE_ACP;
	return TRUE;
}

//DLLの初期化(ワイド文字版)
//戻り値：エラーコード
DWORD WINAPI InitializeCPW()
{
	if( g_sys != NULL ){
		return CP_ERR_INIT;
	}
	g_sys = new CCaptionMain;
	g_charMode = MODE_UTF16;
	return TRUE;
}

//DLLの初期化(UTF-8 mark10als互換)
//戻り値：エラーコード
DWORD WINAPI InitializeUNICODE()
{
	if( g_sys != NULL ){
		return CP_ERR_INIT;
	}
	g_sys = new CCaptionMain;
	g_charMode = MODE_UTF8;
	return TRUE;
}

//DLLの開放
//戻り値：エラーコード
DWORD WINAPI UnInitializeCP()
{
	if( g_sys != NULL ){
		delete g_sys;
		g_sys = NULL;
	}
	if( g_pPool != NULL ){
		delete[] g_pPool;
		g_pPool = NULL;
		g_dwPoolSize = 0;
	}
	return TRUE;
}

DWORD WINAPI AddTSPacketCP(BYTE* pbPacket)
{
	if( g_sys == NULL ){
		return CP_ERR_NOT_INIT;
	}
	return g_sys->AddTSPacket(pbPacket);
}

DWORD WINAPI ClearCP()
{
	if( g_sys == NULL ){
		return CP_ERR_NOT_INIT;
	}
	return g_sys->Clear();
}

DWORD WINAPI GetTagInfoCP(LANG_TAG_INFO_DLL** ppList, DWORD* pdwListCount)
{
	if( g_sys == NULL ){
		return CP_ERR_NOT_INIT;
	}
	return g_sys->GetTagInfo(ppList, pdwListCount);
}

DWORD WINAPI GetCaptionDataCP(unsigned char ucLangTag, CAPTION_DATA_DLL** ppList, DWORD* pdwListCount)
{
	if( g_sys == NULL || (g_charMode != MODE_ACP && g_charMode != MODE_UTF8) ){
		return CP_ERR_NOT_INIT;
	}
	DWORD dwRet = g_sys->GetCaptionData(ucLangTag, ppList, pdwListCount);

	if( dwRet != FALSE ){
		//変換後の文字列を格納できるだけの領域を確保
		DWORD dwNeedSize = 0;
		for( size_t i=0; i<*pdwListCount; i++ ){
			for( size_t j=0; j<(*ppList)[i].dwListCount; j++ ){
				dwNeedSize += lstrlenW( static_cast<LPCWSTR>((*ppList)[i].pstCharList[j].pszDecode) ) + 1;
			}
		}
		if( g_charMode == MODE_ACP ){
			if( g_dwPoolSize < dwNeedSize*2+1 ){
				delete[] g_pPool;
				g_dwPoolSize = dwNeedSize*2+1;
				g_pPool = new char[g_dwPoolSize];
			}
		}else{
			if( g_dwPoolSize < dwNeedSize*4+1 ){
				delete[] g_pPool;
				g_dwPoolSize = dwNeedSize*4+1;
				g_pPool = new char[g_dwPoolSize];
			}
		}

		//必要なコードページに変換してバッファを入れ替える
		DWORD dwPoolCount = 0;
		for( DWORD i=0; i<*pdwListCount; i++ ){
			for( DWORD j=0; j<(*ppList)[i].dwListCount; j++ ){
				if( dwPoolCount >= g_dwPoolSize ){
					dwPoolCount = g_dwPoolSize - 1;
				}
				LPCWSTR pszSrc = static_cast<LPCWSTR>((*ppList)[i].pstCharList[j].pszDecode);
				char *pszDest = g_pPool + dwPoolCount;
				int nWritten = WideCharToMultiByte(g_charMode==MODE_ACP?CP_ACP:CP_UTF8, 0, pszSrc, -1, pszDest, g_dwPoolSize - dwPoolCount, NULL, NULL);

				if( nWritten > 0 ){
					dwPoolCount += nWritten;
				}else{
					pszDest[0] = '\0';
					dwPoolCount++;
				}
				(*ppList)[i].pstCharList[j].pszDecode = pszDest;
			}
		}
	}

	return dwRet;
}

DWORD WINAPI GetCaptionDataCPW(unsigned char ucLangTag, CAPTION_DATA_DLL** ppList, DWORD* pdwListCount)
{
	if( g_sys == NULL || g_charMode != MODE_UTF16 ){
		return CP_ERR_NOT_INIT;
	}
	return g_sys->GetCaptionData(ucLangTag, ppList, pdwListCount);
}

DWORD WINAPI GetDRCSPatternCP(unsigned char ucLangTag, DRCS_PATTERN_DLL** ppList, DWORD* pdwListCount)
{
	if( g_sys == NULL ){
		return CP_ERR_NOT_INIT;
	}
	return g_sys->GetDRCSPattern(ucLangTag, ppList, pdwListCount);
}

DWORD WINAPI SetGaijiCP(DWORD dwCommand, const WCHAR* pTable, DWORD* pdwTableSize)
{
	if( g_sys == NULL ){
		return CP_ERR_NOT_INIT;
	}
	if( dwCommand == 0 ){
		return g_sys->ResetGaijiTable(pdwTableSize);
	}else if( dwCommand == 1 ){
		return g_sys->SetGaijiTable(pTable, pdwTableSize);
	}
	return FALSE;
}

DWORD WINAPI GetGaijiCP(DWORD dwCommand, WCHAR* pTable, DWORD* pdwTableSize)
{
	if( g_sys == NULL ){
		return CP_ERR_NOT_INIT;
	}
	if( dwCommand == 1 ){
		return g_sys->GetGaijiTable(pTable, pdwTableSize);
	}
	return FALSE;
}
