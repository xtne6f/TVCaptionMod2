#include <Windows.h>
#include "Caption.h"

BOOL CalcMD5FromDRCSPattern(BYTE (&bHash)[16], const DRCS_PATTERN_DLL &drcs, int nIndex)
{
	int nWidth = drcs.bmiHeader.biWidth;
	int nHeight = drcs.bmiHeader.biHeight;
	if( nIndex < 0 || (drcs.wGradation != 2 && drcs.wGradation != 4) || nWidth > DRCS_SIZE_MAX || nHeight > DRCS_SIZE_MAX ||
	    /* 標準サイズを縦横2倍角に拡大(横に-2,0,2ずれ)した値も返す */
	    (drcs.wGradation == 4 && (nIndex > 3 || (nIndex > 0 && (nWidth != DRCS_SIZE_MAX / 2 - 2 || nHeight != DRCS_SIZE_MAX / 2)))) ||
	    /* 2階調を4階調に変換した値も返す */
	    (drcs.wGradation == 2 && (nIndex > 7 || (nIndex > 1 && (nWidth != DRCS_SIZE_MAX / 2 - 2 || nHeight != DRCS_SIZE_MAX / 2)))) ){
		return FALSE;
	}
	BYTE bData[(DRCS_SIZE_MAX*DRCS_SIZE_MAX + 3) / 4] = {};

	int nExpand = (nIndex > 1 || (drcs.wGradation == 4 && nIndex > 0) ? 2 : 1);
	int nPaddingL = (nExpand == 2 ? 2 * (drcs.wGradation == 4 ? nIndex - 1 : nIndex / 2 - 1) : 0);
	int nPaddingR = (nExpand == 2 ? 4 - nPaddingL : 0);
	int nGradationBits = (drcs.wGradation == 4 || (nIndex % 2) ? 2 : 1);
	DWORD dwDataBits = 0;
	for( int y=nHeight-1; y>=0; y-- ){
		for( int j=0; j<nExpand; j++ ){
			dwDataBits += nGradationBits * nPaddingL;
			for( int x=0; x<nWidth; x++ ){
				for( int i=0; i<nExpand; i++ ){
					int nPix = drcs.pbBitmap[y * ((nWidth + 7) / 8 * 4) + x / 2] >> (x % 2 ? 0 : 4) & 0x0F;
					if( nGradationBits == 1 ){
						bData[dwDataBits / 8] |= (BYTE)((nPix / 3) << (7 - dwDataBits % 8));
					}else{
						bData[dwDataBits / 8] |= (BYTE)(nPix << (6 - dwDataBits % 8));
					}
					dwDataBits += nGradationBits;
				}
			}
			dwDataBits += nGradationBits * nPaddingR;
		}
	}

	HCRYPTPROV hProv;
	BOOL bRet = FALSE;
	if( ::CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) ){
		HCRYPTHASH hHash;
		if( ::CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash) ){
			DWORD dwHashLen = sizeof(bHash);
			if( ::CryptHashData(hHash, bData, (dwDataBits + 7) / 8, 0) &&
			    ::CryptGetHashParam(hHash, HP_HASHVAL, bHash, &dwHashLen, 0) ){
				bRet = TRUE;
			}
			::CryptDestroyHash(hHash);
		}
		::CryptReleaseContext(hProv, 0);
	}
	return bRet;
}
