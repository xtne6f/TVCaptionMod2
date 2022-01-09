#include <Windows.h>
#include "CaptionDef.h"
#include "ARIB8CharDecode.h"
#include <algorithm>

#ifndef ASSERT
#include <cassert>
#define ASSERT assert
#endif

#if 0//#ifdef _DEBUG
#define DEBUG_OUT(x) ::OutputDebugString(x)
#define DDEBUG_OUT
#else
#define DEBUG_OUT(x)
#endif

#include "ColorDef.h"
#include "GaijiTable.h"

wchar_t CDRCMap::GetUCS(WORD cc) const
{
	if( 0xEC00 <= cc && cc <= 0xEFFF ){
		return (wchar_t)cc;
	}
	for( int i = 0; i < m_used; i++ ){
		if( m_wCodeMap[i] == cc ){
			return (wchar_t)(0xEC00 + i);
		}
	}
	return L'\0';
}

wchar_t CDRCMap::MapUCS(WORD cc)
{
	wchar_t wc = GetUCS(cc);
	if( wc != L'\0' ){
		return wc;
	}
	//新規登録
	wc = (wchar_t)(0xEC00 + m_front);
	m_wCodeMap[m_front] = cc;
	if( m_front >= m_used ){
		m_used++;
	}
	m_front = (m_front + 1) % (sizeof(m_wCodeMap) / sizeof(m_wCodeMap[0]));
	return wc;
}

CARIB8CharDecode::CARIB8CharDecode(void)
{
	DWORD dwRet;
	ResetGaijiTable(&dwRet);
	m_UCSToGaijiTable[0].first = 0;
}

//STD-B24で規定される初期化動作をおこなう
BOOL CARIB8CharDecode::InitCaption(void)
{
	if( m_wSWFMode==14 ){
		m_G0.iMF = MF_DRCS_1;
		m_G0.iMode = MF_MODE_DRCS;
		m_G0.iByte = 1;

		m_G2.iMF = MF_KANJI;
		m_G2.iMode = MF_MODE_G;
		m_G2.iByte = 2;
	}else{
		m_G0.iMF = MF_KANJI;
		m_G0.iMode = MF_MODE_G;
		m_G0.iByte = 2;

		m_G2.iMF = MF_HIRA;
		m_G2.iMode = MF_MODE_G;
		m_G2.iByte = 1;
	}
	m_G1.iMF = MF_ASCII;
	m_G1.iMode = MF_MODE_G;
	m_G1.iByte = 1;

	m_G3.iMF = MF_MACRO;
	m_G3.iMode = MF_MODE_DRCS;
	m_G3.iByte = 1;

	m_GL = &m_G0;
	m_GR = &m_G2;

	m_strDecode = L"";
	m_emStrSize = CP_STR_NORMAL;

	m_bCharColorIndex = 7;
	m_bBackColorIndex = 8;
	m_bRasterColorIndex = 8;
	m_bORNColorIndex = 8;
	m_bDefPalette = 0;

	m_bUnderLine = FALSE;
	m_bBold = FALSE;
	m_bItalic = FALSE;
	m_bFlushMode = 0;
	m_bHLC = 0;
	m_bPRA = 0;
	m_bORN = 0;

	switch(m_wSWFMode){
	case 7:
		//960x540横
		m_wClientW = 960;
		m_wClientH = 540;
		m_wCharW = 36;
		m_wCharH = 36;
		m_wCharHInterval = 4;
		m_wCharVInterval = 24;
		break;
	case 8:
		//960x540縦
		m_wClientW = 960;
		m_wClientH = 540;
		m_wCharW = 36;
		m_wCharH = 36;
		m_wCharHInterval = 12;
		m_wCharVInterval = 24;
		break;
	case 9:
		//720x480横
		m_wClientW = 720;
		m_wClientH = 480;
		m_wCharW = 36;
		m_wCharH = 36;
		m_wCharHInterval = 4;
		m_wCharVInterval = 16;
		break;
	case 10:
		//720x480縦
		m_wClientW = 720;
		m_wClientH = 480;
		m_wCharW = 36;
		m_wCharH = 36;
		m_wCharHInterval = 8;
		m_wCharVInterval = 24;
		break;
	case 14:
		//Cプロファイル
		//表示領域320x180、表示区画20x24で固定
		m_wClientW = 320;
		m_wClientH = 180;
		m_wCharW = 18;
		m_wCharH = 18;
		m_wCharHInterval = 2;
		m_wCharVInterval = 6;
		break;
	default:
		return FALSE;
	}
	m_wClientX = 0;
	m_wClientY = 0;
	m_wPosStartX = m_wPosX = m_wClientX;
	m_wPosY = m_wClientY + GetLineDirSize() - 1;
	m_bPosInit = FALSE;
	m_dwWaitTime = 0;

	// mark10als
	m_bRPC = FALSE;
	m_wRPC = 0;
	// mark10als

#ifdef DDEBUG_OUT
	TCHAR debug[256];
	wsprintf(debug, TEXT(__FUNCTION__) TEXT("(): (cw,ch)=(%d,%d), (dx,dy)=(%d,%d)\n"),
	         m_wClientW, m_wClientH, GetCharDirSize(), GetLineDirSize());
	DEBUG_OUT(debug);
#endif
	return TRUE;
}

//戻り値がFALSEのときpCaptionListは中途半端に更新されているので、可能ならすべてを破棄する
BOOL CARIB8CharDecode::Caption( const BYTE* pbSrc, DWORD dwSrcSize, vector<CAPTION_DATA>* pCaptionList, CDRCMap* pDRCMap, WORD wInitSWFMode, BOOL bUCS )
{
	if( pbSrc == NULL || dwSrcSize == 0 || pCaptionList == NULL || pDRCMap == NULL ){
		return FALSE;
	}
	m_bSpacing = FALSE;
	m_pCaptionList = pCaptionList;
	m_pDRCMap = pDRCMap;
	m_wSWFMode = m_wInitSWFMode = wInitSWFMode;
	m_bUCS = bUCS;
	InitCaption();

	DWORD dwReadSize = 0;
	BOOL bRet = Analyze( pbSrc, dwSrcSize, &dwReadSize );
	if( bRet == TRUE ){
		CheckModify();
	}
	return bRet;
}

//戻り値がFALSEのとき*pdwReadSizeは不定、TRUEのとき*pdwReadSize<=dwSrcSize (C0 C1ほかメソッドも同様)
BOOL CARIB8CharDecode::Analyze( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize )
{
	if( dwSrcSize == 0 ){
		return FALSE;
	}
	DWORD dwReadSize = 0;

	while( dwReadSize < dwSrcSize ){
		BOOL bRet = FALSE;
		DWORD dwReadBuff = 0;
		BOOL bHoldCharacter = m_bRPC;

		BYTE b = pbSrc[dwReadSize];
		if( m_bUCS ){
			if( b <= 0x20 ){
				//C0制御コード
				bRet = C0(pbSrc + dwReadSize, dwSrcSize - dwReadSize, &dwReadBuff);
			}else if( b == 0x7F ){
				//C1制御コード
				bRet = C1(pbSrc + dwReadSize, dwSrcSize - dwReadSize, &dwReadBuff);
			}else if( b == 0xC2 ){
				if( dwReadSize + 1 < dwSrcSize ){
					if( 0x80 <= pbSrc[dwReadSize + 1] && pbSrc[dwReadSize + 1] <= 0xA0 ){
						//C1制御コード
						bRet = C1(pbSrc + dwReadSize, dwSrcSize - dwReadSize, &dwReadBuff);
					}else{
						//非制御符号
						bRet = G_UCS(pbSrc + dwReadSize, dwSrcSize - dwReadSize, &dwReadBuff);
					}
				}
			}else if( b < 0xFE ){
				//非制御符号
				bRet = G_UCS(pbSrc + dwReadSize, dwSrcSize - dwReadSize, &dwReadBuff);
			}
		}else{
			if( b <= 0x20 ){
				//C0制御コード
				bRet = C0(pbSrc + dwReadSize, dwSrcSize - dwReadSize, &dwReadBuff);
			}else if( b < 0x7F ){
				//GL符号領域
				bRet = GL_GR(pbSrc + dwReadSize, dwSrcSize - dwReadSize, &dwReadBuff, m_GL);
			}else if( b <= 0xA0 ){
				//C1制御コード
				bRet = C1(pbSrc + dwReadSize, dwSrcSize - dwReadSize, &dwReadBuff);
			}else if( b < 0xFF ){
				//GR符号領域
				bRet = GL_GR(pbSrc + dwReadSize, dwSrcSize - dwReadSize, &dwReadBuff, m_GR);
			}
		}
		if( !bRet ){
			return FALSE;
		}
		if( m_bSpacing && bHoldCharacter ){
			if( m_wRPC != 0 && --m_wRPC == 0 ){
				//文字繰り返し終了
				m_bRPC = FALSE;
				dwReadSize += dwReadBuff;
			}
		}else{
			dwReadSize += dwReadBuff;
		}
	}

	*pdwReadSize = dwReadSize;
	ASSERT( dwReadSize == dwSrcSize );
	return TRUE;
}

BOOL CARIB8CharDecode::C0( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize )
{
	if( dwSrcSize == 0 ){
		return FALSE;
	}

	DWORD dwReadSize = 1;
	m_bSpacing = FALSE;

	switch(pbSrc[0]){
	case 0x20:
		//SP 背景色空白
		m_strDecode += L'　';
		ActivePositionForward(1);
		m_bSpacing = TRUE;
		break;
	case 0x0D:
		//APR 改行
		CheckModify();
		m_bPosInit = TRUE;
		m_wPosX = m_wClientX;
		m_wPosY += GetLineDirSize();
		if( m_wPosY >= m_wClientY + m_wClientH ){
			m_wPosY = m_wClientY + GetLineDirSize() - 1;
		}
		m_wPosStartX = m_wPosX;
		break;
	case 0x0E:
		//LS1 GLにG1セット
		m_GL = &m_G1;
		break;
	case 0x0F:
		//LS0 GLにG0セット
		m_GL = &m_G0;
		break;
	case 0x19:
		//SS2 シングルシフト
		if( m_bUCS == FALSE ){
			//G2で呼ぶ(マクロ展開を考慮してGLは入れ替えない)
			DWORD dwRead;
			if( GL_GR(pbSrc + dwReadSize, dwSrcSize - dwReadSize, &dwRead, &m_G2) == FALSE ){
				return FALSE;
			}
			dwReadSize += dwRead;
		}
		break;
	case 0x1D:
		//SS3 シングルシフト
		if( m_bUCS == FALSE ){
			//G3で呼ぶ(マクロ展開を考慮してGLは入れ替えない)
			DWORD dwRead;
			if( GL_GR(pbSrc + dwReadSize, dwSrcSize - dwReadSize, &dwRead, &m_G3) == FALSE ){
				return FALSE;
			}
			dwReadSize += dwRead;
		}
		break;
	case 0x1B:
		//ESC エスケープシーケンス
		{
			DWORD dwRead;
			if( ESC(pbSrc + dwReadSize, dwSrcSize - dwReadSize, &dwRead) == FALSE ){
				return FALSE;
			}
			dwReadSize += dwRead;
		}
		break;
	case 0x08:
		//APB 動作位置後退
		{
			CheckModify();
			m_bPosInit = TRUE;
			WORD wDirX = GetCharDirSize();
			if( m_wPosX < m_wClientX + wDirX ){
				WORD wDirY = GetLineDirSize();
				m_wPosX = m_wClientX + m_wClientW / wDirX * wDirX;
				if( m_wPosY < m_wClientY + wDirY*2-1 ){
					m_wPosY = m_wClientY + (m_wClientH/wDirY+1) * wDirY - 1;
				}
				m_wPosY -= wDirY;
			}
			m_wPosX -= wDirX;
			m_wPosStartX = m_wPosX;
		}
		break;
	case 0x09:
		//APF 動作位置前進
		CheckModify();
		ActivePositionForward(1);
		//文字出力がないのでm_wPosStartXを更新しないと進行に矛盾がおこる
		m_wPosStartX = m_wPosX;
		break;
	case 0x0A:
		//APD 動作行前進
		CheckModify();
		m_bPosInit = TRUE;
		m_wPosY += GetLineDirSize();
		if( m_wPosY >= m_wClientY + m_wClientH ){
			m_wPosY = m_wClientY + GetLineDirSize() - 1;
		}
		m_wPosStartX = m_wPosX;
		break;
	case 0x0B:
		//APU 動作行後退
		{
			CheckModify();
			m_bPosInit = TRUE;
			WORD wDirY = GetLineDirSize();
			if( m_wPosY < m_wClientY + wDirY*2-1 ){
				m_wPosY = m_wClientY + (m_wClientH/wDirY+1) * wDirY - 1;
			}
			m_wPosY -= wDirY;
			m_wPosStartX = m_wPosX;
		}
		break;
	case 0x16:
		//PAPF 指定動作位置前進
		if( dwSrcSize <= dwReadSize ){
			return FALSE;
		}
		CheckModify();
		ActivePositionForward(pbSrc[dwReadSize++] - 0x40);
		m_wPosStartX = m_wPosX;
		break;
	case 0x1C:
		//APS 動作位置指定
		{
			if( dwSrcSize <= dwReadSize + 1 ){
				return FALSE;
			}
			CheckModify();
			m_bPosInit = TRUE;
			//動作位置基準点は左下なので1行前進しておく(参考:mark10als)
			WORD wPosY = m_wClientY + GetLineDirSize() * (pbSrc[dwReadSize++] - 0x40 + 1) - 1;
			WORD wPosX = m_wClientX + GetCharDirSize() * (pbSrc[dwReadSize++] - 0x40);
			//無意味な動作位置指定は移動とみなさない
			if( wPosY != m_wPosY || wPosX != m_wPosX ){
				m_wPosY = wPosY;
				m_wPosX = wPosX;
				m_wPosStartX = m_wPosX;
			}
		}
		break;
	case 0x0C:
		//CS
		{
			CheckModify();
			CAPTION_DATA Item;
			Item.bClear = TRUE;
			Item.dwWaitTime = m_dwWaitTime;
			m_pCaptionList->push_back(Item);

			m_wSWFMode = m_wInitSWFMode;
			InitCaption();
		}
		break;
	default:
		break;
	}

	*pdwReadSize = dwReadSize;

	//DEBUG_OUT(pbSrc[0]==0x0E ? TEXT(__FUNCTION__) TEXT("(): GL<-G1\n") :
	//          pbSrc[0]==0x0F ? TEXT(__FUNCTION__) TEXT("(): GL<-G0\n") : NULL);
	return TRUE;
}

BOOL CARIB8CharDecode::C1( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize )
{
	if( dwSrcSize == 0 ){
		return FALSE;
	}

	BYTE b = pbSrc[0];
	DWORD dwReadSize = 1;
	if( m_bUCS && b == 0xC2 ){
		if( dwSrcSize <= 1 ){
			return FALSE;
		}
		b = pbSrc[dwReadSize++];
	}
	m_bSpacing = FALSE;

	if( b != 0x7F ){
		//基本的に文字修飾に変更を伴うので本文をリストに移しておく
		CheckModify();
	}
	switch(b){
	case 0x7F:
		//DEL 前景色空白
		m_strDecode += L'■';
		ActivePositionForward(1);
		m_bSpacing = TRUE;
		break;
	case 0x89:
		//MSZ 半角指定
		m_emStrSize = CP_STR_MEDIUM;
		break;
	case 0x8A:
		//NSZ 全角指定
		m_emStrSize = CP_STR_NORMAL;
		break;
	case 0x80:
		//BKF 文字黒
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x00;
		break;
	case 0x81:
		//RDF 文字赤
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x01;
		break;
	case 0x82:
		//GRF 文字緑
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x02;
		break;
	case 0x83:
		//YLF 文字黄
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x03;
		break;
	case 0x84:
		//BLF 文字青
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x04;
		break;
	case 0x85:
		//MGF 文字マゼンタ
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x05;
		break;
	case 0x86:
		//CNF 文字シアン
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x06;
		break;
	case 0x87:
		//WHF 文字白
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x07;
		break;
	case 0x88:
		//SSZ 小型サイズ
		m_emStrSize = CP_STR_SMALL;
		break;
	case 0x8B:
		//SZX 指定サイズ
		if( dwSrcSize <= dwReadSize ) return FALSE;
		if( pbSrc[dwReadSize] == 0x60 ){
			m_emStrSize = CP_STR_MICRO;
		}else if( pbSrc[dwReadSize] == 0x41 ){
			m_emStrSize = CP_STR_HIGH_W;
		}else if( pbSrc[dwReadSize] == 0x44 ){
			m_emStrSize = CP_STR_WIDTH_W;
		}else if( pbSrc[dwReadSize] == 0x45 ){
			m_emStrSize = CP_STR_W;
		}else if( pbSrc[dwReadSize] == 0x6B ){
			m_emStrSize = CP_STR_SPECIAL_1;
		}else if( pbSrc[dwReadSize] == 0x64 ){
			m_emStrSize = CP_STR_SPECIAL_2;
		}
		dwReadSize++;
		break;
	case 0x90:
		//COL 色指定
		if( dwSrcSize <= dwReadSize ) return FALSE;
		if( pbSrc[dwReadSize] == 0x20 ){
			dwReadSize++;
			if( dwSrcSize <= dwReadSize ) return FALSE;
			//規定によりパレットは0から127まで使われる
			m_bDefPalette = pbSrc[dwReadSize++] & 0x07;
		}else{
			switch( pbSrc[dwReadSize] & 0xF0 ){
				case 0x40:
					m_bCharColorIndex = (m_bDefPalette<<4) | (pbSrc[dwReadSize]&0x0F);
					break;
				case 0x50:
					m_bBackColorIndex = (m_bDefPalette<<4) | (pbSrc[dwReadSize]&0x0F);
					break;
				case 0x60:
					//未サポート
					break;
				case 0x70:
					//未サポート
					break;
				default:
					break;
			}
			dwReadSize++;
		}
		break;
	case 0x91:
		//FLC フラッシング制御
		if( dwSrcSize <= dwReadSize ) return FALSE;
		if( pbSrc[dwReadSize] == 0x40 ){
			m_bFlushMode = 1;
		}else if( pbSrc[dwReadSize] == 0x47 ){
			m_bFlushMode = 2;
		}else if( pbSrc[dwReadSize] == 0x4F ){
			m_bFlushMode = 0;
		}
		dwReadSize++;
		break;
	case 0x93:
		//POL パターン極性
		//未サポート
		if( dwSrcSize <= dwReadSize ) return FALSE;
		dwReadSize++;
		break;
	case 0x94:
		//WMM 書き込みモード変更
		//未サポート
		if( dwSrcSize <= dwReadSize ) return FALSE;
		dwReadSize++;
		break;
	case 0x95:
		//MACRO マクロ定義
		//未サポート(MACRO 0x4Fまで送る)
		dwReadSize++;
		do{
			if( ++dwReadSize > dwSrcSize ){
				return FALSE;
			}
		}while( pbSrc[dwReadSize-2] != 0x95 || pbSrc[dwReadSize-1] != 0x4F );
		break;
	case 0x97:
		//HLC 囲み制御
		if( dwSrcSize <= dwReadSize ) return FALSE;
		m_bHLC = pbSrc[dwReadSize++] & 0x0F;
		break;
	case 0x98:
		//RPC 文字繰り返し
		if( dwSrcSize <= dwReadSize || m_bRPC ) return FALSE;
		m_bRPC = TRUE;
		m_wRPC = pbSrc[dwReadSize] - 0x40 == 0 ? 0xFF : pbSrc[dwReadSize] - 0x40;
		dwReadSize++;
		break;
	case 0x99:
		//SPL アンダーライン モザイクの終了
		m_bUnderLine = FALSE;
		break;
	case 0x9A:
		//STL アンダーライン モザイクの開始
		m_bUnderLine = TRUE;
		break;
	case 0x9D:
		//TIME 時間制御
		if( dwSrcSize <= dwReadSize + 1 ) return FALSE;
		if( pbSrc[dwReadSize] == 0x20 ){
			dwReadSize++;
			m_dwWaitTime += (pbSrc[dwReadSize++] - 0x40) * 100;
		}else{
			//未サポート
			do{
				if( ++dwReadSize > dwSrcSize ){
					return FALSE;
				}
			}while( pbSrc[dwReadSize-1] < 0x40 || 0x43 < pbSrc[dwReadSize-1] );
		}
		break;
	case 0x9B:
		//CSI コントロールシーケンス
		{
			DWORD dwRead;
			if( CSI(pbSrc + dwReadSize, dwSrcSize - dwReadSize, &dwRead) == FALSE ){
				return FALSE;
			}
			dwReadSize += dwRead;
		}
		break;
	default:
		//未サポートの制御コード
		break;
	}

	*pdwReadSize = dwReadSize;

	return TRUE;
}

BOOL CARIB8CharDecode::GL_GR( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize, const MF_MODE* mode )
{
	if( dwSrcSize == 0 ){
		return FALSE;
	}
	BYTE b = pbSrc[0] & 0x7F;
	if( b < 0x21 || 0x7F <= b ){
		return FALSE;
	}

	if( mode->iMode == MF_MODE_G ){
		//文字コード
		m_bSpacing = TRUE;
		switch( mode->iMF ){
			case MF_JISX_KANA:
				{
				//JIS X0201片仮名
				m_strDecode += JisXKanaTable[b - 0x21];
				ActivePositionForward(1);
				}
				break;
			case MF_ASCII:
			case MF_PROP_ASCII:
				{
				//全角なのでテーブルからコード取得
				m_strDecode += AsciiTable[b - 0x21];
				ActivePositionForward(1);
				}
				break;
			case MF_HIRA:
			case MF_PROP_HIRA:
				{
				//Gセットのひらがな系集合
				m_strDecode += HiraTable[b - 0x21];
				ActivePositionForward(1);
				}
				break;
			case MF_KANA:
			case MF_PROP_KANA:
				{
				//Gセットのカタカナ系集合
				m_strDecode += KanaTable[b - 0x21];
				ActivePositionForward(1);
				}
				break;
			case MF_MACRO:
				{
				ASSERT(FALSE);
				goto IGNORE_EXIT;
				}
			case MF_KANJI:
			case MF_JIS_KANJI1:
			case MF_JIS_KANJI2:
			case MF_KIGOU:
				{
				//漢字
				if( dwSrcSize < 2 ){
					return FALSE;
				}
				unsigned char ucFirst = b;
				unsigned char ucSecond = pbSrc[1]&0x7F;
				if( ToSJIS(&ucFirst, &ucSecond) == FALSE ){
					AddGaijiToString(b, pbSrc[1]&0x7F);
				}else{
					AddSJISToString(ucFirst, ucSecond);
				}
				}
				break;
			default:
				//モザイク等
				goto IGNORE_EXIT;
		}
	}else if( mode->iMode == MF_MODE_DRCS ){
		if( MF_DRCS_0 <= mode->iMF && mode->iMF <= MF_DRCS_15 ){
			//DRCS
			WORD cc;
			if( mode->iMF == MF_DRCS_0 ){
				if( dwSrcSize < 2 ){
					return FALSE;
				}
				cc = b<<8 | (pbSrc[1]&0x7F);
			}else{
				//1バイトDRCSは0x01xxから0x0Fxxに符号シフト
				cc = (BYTE)(mode->iMF-MF_DRCS_0)<<8 | b;
			}
			m_strDecode += m_pDRCMap->MapUCS(cc);
			ActivePositionForward(1);
			m_bSpacing = TRUE;
		}else if( mode->iMF == MF_MACRO ){
			DWORD dwTemp=0;
			//マクロ
			if( 0x60 <= b && b <= 0x6F ){
				if( Analyze(DefaultMacro[b & 0x0F], sizeof(DefaultMacro[0]), &dwTemp) == FALSE ){
					return FALSE;
				}
				m_bSpacing = FALSE;
			}else{
				ASSERT(FALSE);
				goto IGNORE_EXIT;
			}
		}else{
			ASSERT(FALSE);
			goto IGNORE_EXIT;
		}
	}else{
		ASSERT(FALSE);
		goto IGNORE_EXIT;
	}

	*pdwReadSize = mode->iByte;
	return TRUE;

IGNORE_EXIT:
	if( (int)dwSrcSize < mode->iByte ) return FALSE;
	m_bSpacing = FALSE;
	*pdwReadSize = mode->iByte;
	return TRUE;
}

BOOL CARIB8CharDecode::G_UCS( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize )
{
	if( dwSrcSize == 0 ){
		return FALSE;
	}

	//UTF-8をコードポイントに変換
	int code = pbSrc[0];
	*pdwReadSize = 1;
	if( 0xC2 <= code && code < 0xE0 && dwSrcSize >= 2 &&
	    0x80 <= pbSrc[1] && pbSrc[1] < 0xC0 ){
		code = ((code & 0x1F) << 6) | (pbSrc[1] & 0x3F);
		*pdwReadSize += 1;
	}else if( 0xE0 <= code && code < 0xF0 && dwSrcSize >= 3 &&
	          0x80 <= pbSrc[1] && pbSrc[1] < 0xC0 && ((code & 0x0F) || (pbSrc[1] & 0x20)) &&
	          0x80 <= pbSrc[2] && pbSrc[2] < 0xC0 ){
		code = ((code & 0x0F) << 12) | ((pbSrc[1] & 0x3F) << 6) | (pbSrc[2] & 0x3F);
		*pdwReadSize += 2;
		if( 0xD800 <= code && code < 0xE000 ){
			code = 0xFFFD;
		}
	}else if( 0xF0 <= code && code < 0xF8 && dwSrcSize >= 4 &&
	          0x80 <= pbSrc[1] && pbSrc[1] < 0xC0 && ((code & 0x07) || (pbSrc[1] & 0x30)) &&
	          0x80 <= pbSrc[2] && pbSrc[2] < 0xC0 &&
	          0x80 <= pbSrc[3] && pbSrc[3] < 0xC0 ){
		code = ((code & 0x07) << 18) | ((pbSrc[1] & 0x3F) << 12) | ((pbSrc[2] & 0x3F) << 6) | (pbSrc[3] & 0x3F);
		*pdwReadSize += 3;
		if( code >= 0x110000 ){
			code = 0xFFFD;
		}
	}else if( code >= 0x80 ){
		code = 0xFFFD;
	}

	if( m_UCSToGaijiTable[0].first == 0 ){
		//変換テーブルを初期化
		for( int i = 0, j = 0; i < _countof(m_UCSToGaijiTable); i++ ){
			std::pair<int, int> codePair(0x110000, 0);
			if( j < _countof(GaijiTable) ){
				codePair.first = GaijiTable[j++];
				if( (codePair.first & 0xFC00) == 0xD800 && j < _countof(GaijiTable) ){
					//サロゲートペア
					codePair.first = 0x10000 + (codePair.first - 0xD800) * 0x400 + (GaijiTable[j++] - 0xDC00);
				}
				codePair.second = (i < G_CELL_SIZE ? 0x7A21 :
				                   i < G_CELL_SIZE * 2 ? 0x7B21 - G_CELL_SIZE :
				                   i < G_CELL_SIZE * 3 ? 0x7C21 - G_CELL_SIZE * 2 :
				                   i < G_CELL_SIZE * 4 ? 0x7D21 - G_CELL_SIZE * 3 :
				                   i < G_CELL_SIZE * 5 ? 0x7E21 - G_CELL_SIZE * 4 :
				                   i < G_CELL_SIZE * 6 ? 0x7521 - G_CELL_SIZE * 5 :
				                   0x7621 - G_CELL_SIZE * 6) + i;
			}
			m_UCSToGaijiTable[i] = codePair;
		}

		//GaijiTableは外字を相応しいUnicodeに対応付けたものなので、逆変換用途には微調整が必要
		//91区8点(郵便局)->2区9点と区別できないので削除
		m_UCSToGaijiTable[G_CELL_SIZE + 7] = std::make_pair(0x110000, 0);
		//92区7-10点(年月日円)->PUA
		m_UCSToGaijiTable[G_CELL_SIZE * 2 + 6] = std::make_pair(0xE28B, 0x7C21 + 6);
		m_UCSToGaijiTable[G_CELL_SIZE * 2 + 7] = std::make_pair(0xE28C, 0x7C21 + 7);
		m_UCSToGaijiTable[G_CELL_SIZE * 2 + 8] = std::make_pair(0xE28D, 0x7C21 + 8);
		m_UCSToGaijiTable[G_CELL_SIZE * 2 + 9] = std::make_pair(0xE28E, 0x7C21 + 9);
		//92区26-31点(氏副元故前新)->PUA
		m_UCSToGaijiTable[G_CELL_SIZE * 2 + 25] = std::make_pair(0xE290, 0x7C21 + 25);
		m_UCSToGaijiTable[G_CELL_SIZE * 2 + 26] = std::make_pair(0xE291, 0x7C21 + 26);
		m_UCSToGaijiTable[G_CELL_SIZE * 2 + 27] = std::make_pair(0xE292, 0x7C21 + 27);
		m_UCSToGaijiTable[G_CELL_SIZE * 2 + 28] = std::make_pair(0xE293, 0x7C21 + 28);
		m_UCSToGaijiTable[G_CELL_SIZE * 2 + 29] = std::make_pair(0xE294, 0x7C21 + 29);
		m_UCSToGaijiTable[G_CELL_SIZE * 2 + 30] = std::make_pair(0xE295, 0x7C21 + 30);
		//93区91点(電話)->PUA
		m_UCSToGaijiTable[G_CELL_SIZE * 3 + 90] = std::make_pair(0xE2FB, 0x7D21 + 90);

		//コードポイント順にソート
		std::sort(m_UCSToGaijiTable, m_UCSToGaijiTable + _countof(m_UCSToGaijiTable));
	}

	std::pair<int, int>* p = std::lower_bound(m_UCSToGaijiTable, m_UCSToGaijiTable + _countof(m_UCSToGaijiTable), std::make_pair(code, 0));
	if( p != m_UCSToGaijiTable + _countof(m_UCSToGaijiTable) && p->first == code ){
		//追加記号集合として処理
		AddGaijiToString((BYTE)(p->second >> 8), (BYTE)p->second);
	}else{
		if( code >= 0x10000 ){
			//サロゲートペア
			m_strDecode += (WCHAR)((code - 0x10000) / 0x400 + 0xD800);
			m_strDecode += (WCHAR)((code - 0x10000) % 0x400 + 0xDC00);
		}else{
			m_strDecode += (WCHAR)code;
		}
		ActivePositionForward(1);
	}

	m_bSpacing = TRUE;
	return TRUE;
}

BOOL CARIB8CharDecode::ToSJIS( unsigned char *pucFirst, unsigned char *pucSecond )
{
	unsigned char &ucFirst = *pucFirst;
	unsigned char &ucSecond = *pucSecond;

	if( ucFirst < 0x21 || ucFirst >= 0x21 + 84 || ucSecond < 0x21 || ucSecond >= 0x21 + 94 ){
		return FALSE;
	}
	ucFirst = ucFirst - 0x21;
	if( ( ucFirst & 0x01 ) == 0 ){
		ucSecond += 0x1F;
		if( ucSecond >= 0x7F ){
			ucSecond += 0x01;
		}
	}else{
		ucSecond += 0x7E;
	}
	ucFirst = ucFirst>>1;
	if( ucFirst >= 0x1F ){
		ucFirst += 0xC1;
	}else{
		ucFirst += 0x81;
	}

	return TRUE;
}

void CARIB8CharDecode::AddGaijiToString( const BYTE bFirst, const BYTE bSecond )
{
	unsigned short usSrc = (unsigned short)(bFirst<<8) | bSecond;
	unsigned short usRow;
	const wchar_t *pszSrc;

	// ARIB 追加漢字
	if( (usRow=0x7521) <= usSrc && usSrc < usRow + G_CELL_SIZE ){
		pszSrc = m_GaijiTable[5*G_CELL_SIZE + usSrc-usRow];
	}else if( (usRow=0x7621) <= usSrc && usSrc < usRow + G_CELL_SIZE ){
		pszSrc = m_GaijiTable[6*G_CELL_SIZE + usSrc-usRow];
	// ARIB 追加記号
	}else if( (usRow=0x7A21) <= usSrc && usSrc < usRow + G_CELL_SIZE ){
		pszSrc = m_GaijiTable[0*G_CELL_SIZE + usSrc-usRow];
	}else if( (usRow=0x7B21) <= usSrc && usSrc < usRow + G_CELL_SIZE ){
		pszSrc = m_GaijiTable[1*G_CELL_SIZE + usSrc-usRow];
	}else if( (usRow=0x7C21) <= usSrc && usSrc < usRow + G_CELL_SIZE ){
		pszSrc = m_GaijiTable[2*G_CELL_SIZE + usSrc-usRow];
	}else if( (usRow=0x7D21) <= usSrc && usSrc < usRow + G_CELL_SIZE ){
		pszSrc = m_GaijiTable[3*G_CELL_SIZE + usSrc-usRow];
	}else if( (usRow=0x7E21) <= usSrc && usSrc < usRow + G_CELL_SIZE ){
		pszSrc = m_GaijiTable[4*G_CELL_SIZE + usSrc-usRow];
	}else{
		pszSrc = L"・";
	}
	wchar_t szDest[3] = { pszSrc[0], pszSrc[1], L'\0' };
	m_strDecode += szDest;

	//文字列に置換するときは文字数を指定
	ActivePositionForward(1);
}

void CARIB8CharDecode::AddSJISToString( unsigned char ucFirst, unsigned char ucSecond )
{
	// CP 932 to UTF-16
	unsigned char szSrc[] = { ucFirst, ucSecond, '\0' };
	wchar_t szDest[3];
	int nSize = MultiByteToWideChar(932, 0, (char*)szSrc, -1, szDest, 3);
	if( nSize <= 1 ){
		m_strDecode += L'・';
	}else{
		m_strDecode += szDest;
	}
	ActivePositionForward(1);
}

BOOL CARIB8CharDecode::ESC( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize )
{
	if( dwSrcSize == 0 ){
		return FALSE;
	}

	DWORD dwReadSize = 0;
	if( pbSrc[0] == 0x24 ){
		if( dwSrcSize < 2 ) return FALSE;

		if( pbSrc[1] >= 0x28 && pbSrc[1] <= 0x2B ){
			if( dwSrcSize < 3 ) return FALSE;

			if( pbSrc[2] == 0x20 ){
				if( dwSrcSize < 4 ) return FALSE;
				//2バイトDRCS
				switch(pbSrc[1]){
					case 0x28:
						m_G0.iMF = pbSrc[3];
						m_G0.iMode = MF_MODE_DRCS;
						m_G0.iByte = 2;
						break;
					case 0x29:
						m_G1.iMF = pbSrc[3];
						m_G1.iMode = MF_MODE_DRCS;
						m_G1.iByte = 2;
						break;
					case 0x2A:
						m_G2.iMF = pbSrc[3];
						m_G2.iMode = MF_MODE_DRCS;
						m_G2.iByte = 2;
						break;
					case 0x2B:
						m_G3.iMF = pbSrc[3];
						m_G3.iMode = MF_MODE_DRCS;
						m_G3.iByte = 2;
						break;
					default:
						break;
				}
				dwReadSize = 4;
			}else{
				//2バイトGセット
				switch(pbSrc[1]){
					case 0x29:
						m_G1.iMF = pbSrc[2];
						m_G1.iMode = MF_MODE_G;
						m_G1.iByte = 2;
						break;
					case 0x2A:
						m_G2.iMF = pbSrc[2];
						m_G2.iMode = MF_MODE_G;
						m_G2.iByte = 2;
						break;
					case 0x2B:
						m_G3.iMF = pbSrc[2];
						m_G3.iMode = MF_MODE_G;
						m_G3.iByte = 2;
						break;
					default:
						break;
				}
				dwReadSize = 3;
			}
		}else{
			//2バイトGセット
			m_G0.iMF = pbSrc[1];
			m_G0.iMode = MF_MODE_G;
			m_G0.iByte = 2;
			dwReadSize = 2;
		}
	}else if( pbSrc[0] >= 0x28 && pbSrc[0] <= 0x2B ){
		if( dwSrcSize < 2 ) return FALSE;

		if( pbSrc[1] == 0x20 ){
			if( dwSrcSize < 3 ) return FALSE;
			//1バイトDRCS
			switch(pbSrc[0]){
				case 0x28:
					m_G0.iMF = pbSrc[2];
					m_G0.iMode = MF_MODE_DRCS;
					m_G0.iByte = 1;
					break;
				case 0x29:
					m_G1.iMF = pbSrc[2];
					m_G1.iMode = MF_MODE_DRCS;
					m_G1.iByte = 1;
					break;
				case 0x2A:
					m_G2.iMF = pbSrc[2];
					m_G2.iMode = MF_MODE_DRCS;
					m_G2.iByte = 1;
					break;
				case 0x2B:
					m_G3.iMF = pbSrc[2];
					m_G3.iMode = MF_MODE_DRCS;
					m_G3.iByte = 1;
					break;
				default:
					break;
			}
			dwReadSize = 3;
		}else{
			//1バイトGセット
			switch(pbSrc[0]){
				case 0x28:
					m_G0.iMF = pbSrc[1];
					m_G0.iMode = MF_MODE_G;
					m_G0.iByte = 1;
					break;
				case 0x29:
					m_G1.iMF = pbSrc[1];
					m_G1.iMode = MF_MODE_G;
					m_G1.iByte = 1;
					break;
				case 0x2A:
					m_G2.iMF = pbSrc[1];
					m_G2.iMode = MF_MODE_G;
					m_G2.iByte = 1;
					break;
				case 0x2B:
					m_G3.iMF = pbSrc[1];
					m_G3.iMode = MF_MODE_G;
					m_G3.iByte = 1;
					break;
				default:
					break;
			}
			dwReadSize = 2;
		}
	}else if( pbSrc[0] == 0x6E ){
		//GLにG2セット
		m_GL = &m_G2;
		dwReadSize = 1;
	}else if( pbSrc[0] == 0x6F ){
		//GLにG3セット
		m_GL = &m_G3;
		dwReadSize = 1;
	}else if( pbSrc[0] == 0x7C ){
		//GRにG3セット
		m_GR = &m_G3;
		dwReadSize = 1;
	}else if( pbSrc[0] == 0x7D ){
		//GRにG2セット
		m_GR = &m_G2;
		dwReadSize = 1;
	}else if( pbSrc[0] == 0x7E ){
		//GRにG1セット
		m_GR = &m_G1;
		dwReadSize = 1;
	}else{
		//未サポート
		return FALSE;
	}

	*pdwReadSize = dwReadSize;

#if 0//#ifdef DDEBUG_OUT
	TCHAR debug[256];
	wsprintf(debug, TEXT(__FUNCTION__) TEXT("(): GL<-G%d,GR<-G%d, G0=%#02x,G1=%#02x,G2=%#02x,G3=%#02x\n"),
	         m_GL==&m_G1 ? 1 : m_GL==&m_G2 ? 2 : m_GL==&m_G3 ? 3 : 0,
	         m_GR==&m_G1 ? 1 : m_GR==&m_G2 ? 2 : m_GR==&m_G3 ? 3 : 0,
	         m_G0.iMF, m_G1.iMF, m_G2.iMF, m_G3.iMF);
	DEBUG_OUT(debug);
#endif
	return TRUE;
}

BOOL CARIB8CharDecode::CSI( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize )
{
	DWORD dwReadSize = 0;

	//中間文字0x20まで移動
	WORD wP1 = 0;
	WORD wP2 = 0;
	int nParam = 0;
	for( ; dwReadSize+1<dwSrcSize; dwReadSize++ ){
		if( pbSrc[dwReadSize] == 0x20 ){
			if( nParam==0 ){
				wP1 = wP2;
			}
			nParam++;
			break;
		}else if( pbSrc[dwReadSize] == 0x3B ){
			if( nParam==0 ){
				wP1 = wP2;
				wP2 = 0;
			}
			nParam++;
		}else if( 0x30<=pbSrc[dwReadSize] && pbSrc[dwReadSize]<=0x39 ){
			if( nParam<=1 ){
				wP2 = wP2*10+(pbSrc[dwReadSize]&0x0F);
			}
		}
	}
	//終端文字に移動
	if( ++dwReadSize >= dwSrcSize ){
		return FALSE;
	}

	switch(pbSrc[dwReadSize]){
		case 0x53:
			//SWF
			if( nParam==1 ){
				m_wSWFMode = wP1;
			}else{
				//未サポート
			}
			if( InitCaption() == FALSE ){
				return FALSE;
			}
			break;
		case 0x6E:
			//RCS
			m_bRasterColorIndex = (BYTE)(wP1&0x7F);
			break;
		case 0x61:
			//ACPS
			m_wPosStartX = m_wPosX = wP1;
			if( nParam>=2 ){
				m_wPosY = wP2;
			}
			break;
		case 0x56:
			//SDF
			m_wClientW = wP1;
			if( nParam>=2 ){
				m_wClientH = wP2;
			}
			break;
		case 0x5F:
			//SDP
			m_wClientX = wP1;
			if( nParam>=2 ){
				m_wClientY = wP2;
			}
			if( m_bPosInit == FALSE ){
				m_wPosStartX = m_wPosX = m_wClientX;
				m_wPosY = m_wClientY + GetLineDirSize() - 1;
			}
			break;
		case 0x57:
			//SSM
			m_wCharW = wP1;
			if( nParam>=2 ){
				m_wCharH = wP2;
			}
			if( m_bPosInit == FALSE ){
				m_wPosY = m_wClientY + GetLineDirSize() - 1;
			}
			break;
		case 0x58:
			//SHS
			m_wCharHInterval = wP1;
			break;
		case 0x59:
			//SVS
			m_wCharVInterval = wP1;
			break;
		case 0x42:
			//GSM
			//未サポート
			break;
		case 0x5D:
			//GAA
			//未サポート
			break;
		case 0x5E:
			//SRC
			//未サポート
			break;
		case 0x62:
			//TCC
			//未サポート
			break;
		case 0x65:
			//CFS
			//未サポート
			break;
		case 0x63:
			//ORN
			if( wP1 <= 3 ){
				BYTE bIndex = (BYTE)((nParam >= 2 && (wP1 == 1 || wP1 == 2) ? (wP2 / 100) << 4 | (wP2 % 100) : 8) & 0x7F);
				if( wP1 != m_bORN || bIndex != m_bORNColorIndex ){
					CheckModify();
				}
				m_bORN = (BYTE)wP1;
				m_bORNColorIndex = bIndex;
			}
			break;
		case 0x64:
			//MDF
			if( wP1 <= 3 ){
				if( ((wP1 & 1) != 0) != m_bBold ||
				    ((wP1 & 2) != 0) != m_bItalic ){
					CheckModify();
				}
				m_bBold = (wP1 & 1) != 0;
				m_bItalic = (wP1 & 2) != 0;
			}
			break;
		case 0x66:
			//XCS
			//未サポート
			break;
		case 0x68:
			//PRA
			m_bPRA = (BYTE)(wP1 + 1);
			CheckModify(TRUE);
			m_bPRA = 0;
			break;
		case 0x54:
			//CCC
			//未サポート
			break;
		case 0x67:
			//SCR
			//未サポート
			break;
		default:
			break;
	}
	dwReadSize++;

	*pdwReadSize = dwReadSize;

	return TRUE;
}

//変換済みの本文をリストに移す
//CAPTION_CHAR_DATAやCAPTION_DATAのプロパティ変更が必要な処理の直前によぶ
void CARIB8CharDecode::CheckModify(BOOL bForce)
{
	if( bForce || !m_strDecode.empty() ){
		if( IsCaptionPropertyChanged() ){
#ifdef DDEBUG_OUT
			TCHAR debug[256];
			wsprintf(debug, TEXT(__FUNCTION__) TEXT("(): ++CaptionList (cx,cy)=(%d,%d) (cw,ch)=(%d,%d) (px,py)=(%d,%d)\n"),
			         m_wClientX, m_wClientY, m_wClientW, m_wClientH, m_wPosStartX, m_wPosY);
			DEBUG_OUT(debug);
#endif
			CAPTION_DATA Item;
			CreateCaptionData(&Item);
			m_pCaptionList->push_back(Item);
		}
		DEBUG_OUT(TEXT(__FUNCTION__) TEXT("(): ++ "));
		DEBUG_OUT(m_strDecode.c_str());
		DEBUG_OUT(TEXT("\n"));

		CAPTION_CHAR_DATA CharItem;
		CreateCaptionCharData(&CharItem);
		m_pCaptionList->back().CharList.push_back(CharItem);
		m_strDecode = L"";
	}
}

void CARIB8CharDecode::CreateCaptionData(CAPTION_DATA* pItem) const
{
	pItem->bClear = FALSE;
	pItem->dwWaitTime = m_dwWaitTime;
	pItem->wSWFMode = m_wSWFMode;
	pItem->wClientX = m_wClientX;
	pItem->wClientY = m_wClientY;
	pItem->wClientW = m_wClientW;
	pItem->wClientH = m_wClientH;
	pItem->wPosX = m_wPosStartX;
	pItem->wPosY = m_wPosY;
}

void CARIB8CharDecode::CreateCaptionCharData(CAPTION_CHAR_DATA* pItem) const
{
	pItem->strDecode = m_strDecode;

	pItem->stCharColor = DefClut[m_bCharColorIndex];
	pItem->stBackColor = DefClut[m_bBackColorIndex];
	pItem->stRasterColor = DefClut[m_bRasterColorIndex];
	pItem->stORNColor = DefClut[m_bORNColorIndex];

	pItem->bUnderLine = m_bUnderLine;
	pItem->bBold = m_bBold;
	pItem->bItalic = m_bItalic;
	pItem->bFlushMode = m_bFlushMode;
	pItem->bHLC = m_bHLC<<4;
	pItem->bPRA = m_bPRA;
	pItem->bORN = m_bORN;

	pItem->wCharW = m_wCharW;
	pItem->wCharH = m_wCharH;
	pItem->wCharHInterval = m_wCharHInterval;
	pItem->wCharVInterval = m_wCharVInterval;
	pItem->emCharSizeMode = m_emStrSize;
}

//本文の表示区画や表示タイミングが移動したかどうか
const BOOL CARIB8CharDecode::IsCaptionPropertyChanged(void) const
{
	if( m_pCaptionList->empty() ||
		m_pCaptionList->back().bClear ||
		m_pCaptionList->back().dwWaitTime != m_dwWaitTime ||
		m_pCaptionList->back().wSWFMode != m_wSWFMode ||
		m_pCaptionList->back().wClientX != m_wClientX ||
		m_pCaptionList->back().wClientY != m_wClientY ||
		m_pCaptionList->back().wClientW != m_wClientW ||
		m_pCaptionList->back().wClientH != m_wClientH ||
		m_pCaptionList->back().wPosX != m_wPosStartX ||
		m_pCaptionList->back().wPosY != m_wPosY )
	{
		return TRUE;
	}

	return FALSE;
}

//動作位置前進する
void CARIB8CharDecode::ActivePositionForward( int nCount )
{
	m_bPosInit = TRUE;

	WORD wPosX = m_wPosX;
	WORD wPosY = m_wPosY;
	while( --nCount >= 0 ){
		wPosX += GetCharDirSize();
		if( wPosX >= m_wClientX + m_wClientW ){
			//"表示領域の端を越えることとなる場合、動作位置を表示領域の逆の端へ移動"
			wPosX = m_wClientX;
			wPosY += GetLineDirSize();
			if( wPosY >= m_wClientY + m_wClientH ){
				wPosY = m_wClientY + GetLineDirSize() - 1;
			}
		}
	}

	if( wPosY != m_wPosY ){
		//改行を伴うときはこれまでの本文をリストに移す
		CheckModify();
		//行末まで文字繰り返しのときは繰り返しを終了する
		if( m_bRPC && m_wRPC >= 0x40 ){
			m_wRPC = 1;
		}
		m_wPosStartX = wPosX;
	}
	m_wPosX = wPosX;
	m_wPosY = wPosY;
#ifdef DDEBUG_OUT
	TCHAR debug[256];
	wsprintf(debug, TEXT(__FUNCTION__) TEXT("(): (px,py)=(%d,%d)\n"), m_wPosX, m_wPosY);
	DEBUG_OUT(debug);
#endif
}

WORD CARIB8CharDecode::GetCharDirSize(void) const
{
	WORD wSize = m_wCharW + m_wCharHInterval;
	if( m_emStrSize==CP_STR_SMALL || m_emStrSize==CP_STR_MEDIUM ){
		wSize /= 2;
	}else if( m_emStrSize==CP_STR_W || m_emStrSize==CP_STR_WIDTH_W ){
		wSize *= 2;
	}
	return wSize==0 ? 1 : wSize;
}

WORD CARIB8CharDecode::GetLineDirSize(void) const
{
	WORD wSize = m_wCharH + m_wCharVInterval;
	if( m_emStrSize==CP_STR_SMALL ){
		wSize /= 2;
	}else if( m_emStrSize==CP_STR_W || m_emStrSize==CP_STR_HIGH_W ){
		wSize *= 2;
	}
	return wSize==0 ? 1 : wSize;
}

#define GET_PIXEL_1(b,x) ( ((b)[(x)/8]>>(7-(x)%8)) & 0x1 )
#define GET_PIXEL_2(b,x) ( ((b)[(x)/4]>>((3-(x)%4)*2)) & 0x3 )

BOOL CARIB8CharDecode::DRCSHeaderparse( const BYTE* pbSrc, DWORD dwSrcSize, vector<DRCS_PATTERN>* pDRCList, BOOL bDRCS_0 )
{
	if( pbSrc == NULL || dwSrcSize == 0 || pDRCList == NULL ){
		return FALSE;
	}

	BYTE bNumberOfCode = pbSrc[0];
	DWORD dwRead = 1;
	for( int i=0; i<bNumberOfCode; i++ ){
		if( dwSrcSize < dwRead + 3 ){
			return FALSE;
		}
		WORD wDRCCode = bDRCS_0 ? (pbSrc[dwRead]<<8) | pbSrc[dwRead+1] :
		                         ((pbSrc[dwRead]-MF_DRCS_0)<<8) | pbSrc[dwRead+1];
		BYTE bNumberOfFont = pbSrc[dwRead+2];
		dwRead += 3;
		for( int j=0; j<bNumberOfFont; j++ ){
			if( dwSrcSize < dwRead + 4 ){
				return FALSE;
			}
			BYTE bMode = pbSrc[dwRead] & 0x0F;
			BYTE bDepth = pbSrc[dwRead+1];
			BYTE bPixPerByte = bDepth==0 ? 8 : 4;
			BYTE bWidth = pbSrc[dwRead+2];
			BYTE bHeight = pbSrc[dwRead+3];
			if( !(bMode==0 && bDepth==0 || bMode==1 && bDepth==2) || bWidth>DRCS_SIZE_MAX || bHeight>DRCS_SIZE_MAX ){
				//未サポート(運用規定外)
				return FALSE;
			}
			dwRead += 4;
			if( dwSrcSize < dwRead + (bHeight*bWidth+bPixPerByte-1) / bPixPerByte ){
				return FALSE;
			}
			//ここで新規追加
			pDRCList->resize(pDRCList->size() + 1);
			DRCS_PATTERN &drcs = pDRCList->back();
			drcs.wDRCCode = wDRCCode;
			drcs.wGradation = bDepth + 2;

			DWORD dwSizeImage = 0;
			//ビットマップの仕様により左下から走査
			for( int y=bHeight-1; y>=0; y-- ){
				for( int x=0; x<bWidth; x++ ){
					int nPix = bDepth==0 ? GET_PIXEL_1(pbSrc+dwRead, y*bWidth+x) * 3 :
					                       GET_PIXEL_2(pbSrc+dwRead, y*bWidth+x);
					if( x%2==0 ){
						drcs.bBitmap[dwSizeImage++] = (BYTE)(nPix<<4);
					}else{
						drcs.bBitmap[dwSizeImage-1] |= (BYTE)nPix;
					}
				}
				//ビットマップの仕様によりストライドを4バイト境界にする
				dwSizeImage = (dwSizeImage + 3) / 4 * 4;
			}
			BITMAPINFOHEADER bmiHeader = {0};
			bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmiHeader.biWidth = bWidth;
			bmiHeader.biHeight = bHeight;
			bmiHeader.biPlanes = 1;
			bmiHeader.biBitCount = 4;
			bmiHeader.biCompression = BI_RGB;
			bmiHeader.biSizeImage = dwSizeImage;
			drcs.bmiHeader = bmiHeader;

			dwRead += (bHeight*bWidth+bPixPerByte-1) / bPixPerByte;
		}
	}
	return TRUE;
}

BOOL CARIB8CharDecode::GetGaijiTable(WCHAR* pTable, DWORD* pdwTableSize) const
{
	if( pTable==NULL || pdwTableSize==NULL ){
		return FALSE;
	}
	DWORD dwSize = *pdwTableSize;
	DWORD dwCopy = 0;
	for( int i = 0; dwCopy < dwSize && i < _countof(m_GaijiTable); i++ ){
		if( m_GaijiTable[i][0] ){
			pTable[dwCopy++] = m_GaijiTable[i][0];
			if( m_GaijiTable[i][1] && dwCopy < dwSize ){
				pTable[dwCopy++] = m_GaijiTable[i][1];
			}
		}
	}
	*pdwTableSize = dwCopy;
	return TRUE;
}

BOOL CARIB8CharDecode::SetGaijiTable(const WCHAR* pTable, DWORD* pdwTableSize)
{
	if( pTable==NULL || pdwTableSize==NULL ){
		return FALSE;
	}
	DWORD dwSize = *pdwTableSize;
	DWORD dwCopy = 0;
	for( int i = 0; dwCopy < dwSize && i < _countof(m_GaijiTable); i++ ){
		m_GaijiTable[i][0] = pTable[dwCopy++];
		if( (m_GaijiTable[i][0] & 0xFC00) == 0xD800 && dwCopy < dwSize ){
			//上位サロゲート
			m_GaijiTable[i][1] = pTable[dwCopy++];
		}else{
			m_GaijiTable[i][1] = L'\0';
		}
	}
	*pdwTableSize = dwCopy;
	return TRUE;
}

BOOL CARIB8CharDecode::ResetGaijiTable(DWORD* pdwTableSize)
{
	if( pdwTableSize==NULL ){
		return FALSE;
	}
	*pdwTableSize = sizeof(GaijiTable) / sizeof(WCHAR);
	return SetGaijiTable(GaijiTable, pdwTableSize);
}

const WCHAR CARIB8CharDecode::AsciiTable[94] = {
	L'！', L'”', L'＃', L'＄', L'％', L'＆', L'’',
	L'（', L'）', L'＊', L'＋', L'，', L'－', L'．', L'／',
	L'０', L'１', L'２', L'３', L'４', L'５', L'６', L'７',
	L'８', L'９', L'：', L'；', L'＜', L'＝', L'＞', L'？',
	L'＠', L'Ａ', L'Ｂ', L'Ｃ', L'Ｄ', L'Ｅ', L'Ｆ', L'Ｇ',
	L'Ｈ', L'Ｉ', L'Ｊ', L'Ｋ', L'Ｌ', L'Ｍ', L'Ｎ', L'Ｏ',
	L'Ｐ', L'Ｑ', L'Ｒ', L'Ｓ', L'Ｔ', L'Ｕ', L'Ｖ', L'Ｗ',
	L'Ｘ', L'Ｙ', L'Ｚ', L'［', L'￥', L'］', L'＾', L'＿',
	L'‘', L'ａ', L'ｂ', L'ｃ', L'ｄ', L'ｅ', L'ｆ', L'ｇ',
	L'ｈ', L'ｉ', L'ｊ', L'ｋ', L'ｌ', L'ｍ', L'ｎ', L'ｏ',
	L'ｐ', L'ｑ', L'ｒ', L'ｓ', L'ｔ', L'ｕ', L'ｖ', L'ｗ',
	L'ｘ', L'ｙ', L'ｚ', L'｛', L'｜', L'｝', L'￣'
};

const WCHAR CARIB8CharDecode::HiraTable[94] = {
	L'ぁ', L'あ', L'ぃ', L'い', L'ぅ', L'う', L'ぇ',
	L'え', L'ぉ', L'お', L'か', L'が', L'き', L'ぎ', L'く',
	L'ぐ', L'け', L'げ', L'こ', L'ご', L'さ', L'ざ', L'し',
	L'じ', L'す', L'ず', L'せ', L'ぜ', L'そ', L'ぞ', L'た',
	L'だ', L'ち', L'ぢ', L'っ', L'つ', L'づ', L'て', L'で',
	L'と', L'ど', L'な', L'に', L'ぬ', L'ね', L'の', L'は',
	L'ば', L'ぱ', L'ひ', L'び', L'ぴ', L'ふ', L'ぶ', L'ぷ',
	L'へ', L'べ', L'ぺ', L'ほ', L'ぼ', L'ぽ', L'ま', L'み',
	L'む', L'め', L'も', L'ゃ', L'や', L'ゅ', L'ゆ', L'ょ',
	L'よ', L'ら', L'り', L'る', L'れ', L'ろ', L'ゎ', L'わ',
	L'ゐ', L'ゑ', L'を', L'ん', L'　', L'　', L'　', L'ゝ',
	L'ゞ', L'ー', L'。', L'「', L'」', L'、', L'・'
};

const WCHAR CARIB8CharDecode::KanaTable[94] = {
	L'ァ', L'ア', L'ィ', L'イ', L'ゥ', L'ウ', L'ェ',
	L'エ', L'ォ', L'オ', L'カ', L'ガ', L'キ', L'ギ', L'ク',
	L'グ', L'ケ', L'ゲ', L'コ', L'ゴ', L'サ', L'ザ', L'シ',
	L'ジ', L'ス', L'ズ', L'セ', L'ゼ', L'ソ', L'ゾ', L'タ',
	L'ダ', L'チ', L'ヂ', L'ッ', L'ツ', L'ヅ', L'テ', L'デ',
	L'ト', L'ド', L'ナ', L'ニ', L'ヌ', L'ネ', L'ノ', L'ハ',
	L'バ', L'パ', L'ヒ', L'ビ', L'ピ', L'フ', L'ブ', L'プ',
	L'ヘ', L'ベ', L'ペ', L'ホ', L'ボ', L'ポ', L'マ', L'ミ',
	L'ム', L'メ', L'モ', L'ャ', L'ヤ', L'ュ', L'ユ', L'ョ',
	L'ヨ', L'ラ', L'リ', L'ル', L'レ', L'ロ', L'ヮ', L'ワ',
	L'ヰ', L'ヱ', L'ヲ', L'ン', L'ヴ', L'ヵ', L'ヶ', L'ヽ',
	L'ヾ', L'ー', L'。', L'「', L'」', L'、', L'・'
};

const WCHAR CARIB8CharDecode::JisXKanaTable[94] = {
	L'。', L'「', L'」', L'、', L'・', L'ヲ', L'ァ',
	L'ィ', L'ゥ', L'ェ', L'ォ', L'ャ', L'ュ', L'ョ', L'ッ',
	L'ー', L'ア', L'イ', L'ウ', L'エ', L'オ', L'カ', L'キ',
	L'ク', L'ケ', L'コ', L'サ', L'シ', L'ス', L'セ', L'ソ',
	L'タ', L'チ', L'ツ', L'テ', L'ト', L'ナ', L'ニ', L'ヌ',
	L'ネ', L'ノ', L'ハ', L'ヒ', L'フ', L'ヘ', L'ホ', L'マ',
	L'ミ', L'ム', L'メ', L'モ', L'ヤ', L'ユ', L'ヨ', L'ラ',
	L'リ', L'ル', L'レ', L'ロ', L'ワ', L'ン', L'゛', L'゜',
	L'・', L'・', L'・', L'・', L'・', L'・', L'・', L'・',
	L'・', L'・', L'・', L'・', L'・', L'・', L'・', L'・',
	L'・', L'・', L'・', L'・', L'・', L'・', L'・', L'・',
	L'・', L'・', L'・', L'・', L'・', L'・', L'・'
};

//デフォルトマクロ文(NULは効果がないと規定されている)
const BYTE CARIB8CharDecode::DefaultMacro[16][20] = {
	{0x1B, 0x24, 0x42, 0x1B, 0x29, 0x4A, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x24, 0x42, 0x1B, 0x29, 0x31, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x24, 0x42, 0x1B, 0x29, 0x20, 0x41, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x32, 0x1B, 0x29, 0x34, 0x1B, 0x2A, 0x35, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x32, 0x1B, 0x29, 0x33, 0x1B, 0x2A, 0x35, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x32, 0x1B, 0x29, 0x20, 0x41, 0x1B, 0x2A, 0x35, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x20, 0x41, 0x1B, 0x29, 0x20, 0x42, 0x1B, 0x2A, 0x20, 0x43, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x20, 0x44, 0x1B, 0x29, 0x20, 0x45, 0x1B, 0x2A, 0x20, 0x46, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x20, 0x47, 0x1B, 0x29, 0x20, 0x48, 0x1B, 0x2A, 0x20, 0x49, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x20, 0x4A, 0x1B, 0x29, 0x20, 0x4B, 0x1B, 0x2A, 0x20, 0x4C, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x20, 0x4D, 0x1B, 0x29, 0x20, 0x4E, 0x1B, 0x2A, 0x20, 0x4F, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x24, 0x42, 0x1B, 0x29, 0x20, 0x42, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x24, 0x42, 0x1B, 0x29, 0x20, 0x43, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x24, 0x42, 0x1B, 0x29, 0x20, 0x44, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x31, 0x1B, 0x29, 0x30, 0x1B, 0x2A, 0x4A, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x4A, 0x1B, 0x29, 0x32, 0x1B, 0x2A, 0x20, 0x41, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D}
};
