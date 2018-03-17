#include <Windows.h>
#include "Caption.h"

BOOL CalcMD5FromDRCSPattern(BYTE *pbHash, const DRCS_PATTERN_DLL *pPattern)
{
	WORD wGradation = pPattern->wGradation;
	int nWidth = pPattern->bmiHeader.biWidth;
	int nHeight = pPattern->bmiHeader.biHeight;
	if( !(wGradation==2 || wGradation==4) || nWidth>DRCS_SIZE_MAX || nHeight>DRCS_SIZE_MAX ){
		return FALSE;
	}
	BYTE bData[(DRCS_SIZE_MAX*DRCS_SIZE_MAX + 3) / 4] = {};
	const BYTE *pbBitmap = pPattern->pbBitmap;

	DWORD dwDataLen = wGradation==2 ? (nWidth*nHeight+7)/8 : (nWidth*nHeight+3)/4;
	DWORD dwSizeImage = 0;
	for( int y=nHeight-1; y>=0; y-- ){
		for( int x=0; x<nWidth; x++ ){
			int nPix = x%2==0 ? pbBitmap[dwSizeImage++]>>4 :
			                    pbBitmap[dwSizeImage-1]&0x0F;
			int nPos = y*nWidth+x;
			if( wGradation==2 ){
				bData[nPos/8] |= (BYTE)((nPix/3)<<(7-nPos%8));
			}else{
				bData[nPos/4] |= (BYTE)(nPix<<((3-nPos%4)*2));
			}
		}
		dwSizeImage = (dwSizeImage + 3) / 4 * 4;
	}

	HCRYPTPROV hProv;
	BOOL bRet = FALSE;
	if( ::CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) ){
		HCRYPTHASH hHash;
		if( ::CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash) ){
			DWORD dwHashLen = 16;
			if( ::CryptHashData(hHash, bData, dwDataLen, 0) &&
			    ::CryptGetHashParam(hHash, HP_HASHVAL, pbHash, &dwHashLen, 0) ){
				bRet = TRUE;
			}
			::CryptDestroyHash(hHash);
		}
		::CryptReleaseContext(hProv, 0);
	}
	return bRet;
}
