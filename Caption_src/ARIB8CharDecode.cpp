#include <Windows.h>
#include "CaptionDef.h"
#include "ARIB8CharDecode.h"

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

#if 1 //Table
//コンパイル単位で実行ファイルサイズやメモリを消費していたのでヘッダから移動

#include "ColorDef.h"
#include "GaijiTable.h"

//From: up0511mod
static const wchar_t AsciiTable[]={
	L'！',L'”',L'＃',L'＄',L'％',L'＆',L'’',
	L'（',L'）',L'＊',L'＋',L'，',L'－',L'．',L'／',
	L'０',L'１',L'２',L'３',L'４',L'５',L'６',L'７',
	L'８',L'９',L'：',L'；',L'＜',L'＝',L'＞',L'？',
	L'＠',L'Ａ',L'Ｂ',L'Ｃ',L'Ｄ',L'Ｅ',L'Ｆ',L'Ｇ',
	L'Ｈ',L'Ｉ',L'Ｊ',L'Ｋ',L'Ｌ',L'Ｍ',L'Ｎ',L'Ｏ',
	L'Ｐ',L'Ｑ',L'Ｒ',L'Ｓ',L'Ｔ',L'Ｕ',L'Ｖ',L'Ｗ',
	L'Ｘ',L'Ｙ',L'Ｚ',L'［',L'￥',L'］',L'＾',L'＿',
	L'‘',L'ａ',L'ｂ',L'ｃ',L'ｄ',L'ｅ',L'ｆ',L'ｇ',
	L'ｈ',L'ｉ',L'ｊ',L'ｋ',L'ｌ',L'ｍ',L'ｎ',L'ｏ',
	L'ｐ',L'ｑ',L'ｒ',L'ｓ',L'ｔ',L'ｕ',L'ｖ',L'ｗ',
	L'ｘ',L'ｙ',L'ｚ',L'｛',L'｜',L'｝',L'￣'
};
static const wchar_t HiraTable[]={
	L'ぁ',L'あ',L'ぃ',L'い',L'ぅ',L'う',L'ぇ',
	L'え',L'ぉ',L'お',L'か',L'が',L'き',L'ぎ',L'く',
	L'ぐ',L'け',L'げ',L'こ',L'ご',L'さ',L'ざ',L'し',
	L'じ',L'す',L'ず',L'せ',L'ぜ',L'そ',L'ぞ',L'た',
	L'だ',L'ち',L'ぢ',L'っ',L'つ',L'づ',L'て',L'で',
	L'と',L'ど',L'な',L'に',L'ぬ',L'ね',L'の',L'は',
	L'ば',L'ぱ',L'ひ',L'び',L'ぴ',L'ふ',L'ぶ',L'ぷ',
	L'へ',L'べ',L'ぺ',L'ほ',L'ぼ',L'ぽ',L'ま',L'み',
	L'む',L'め',L'も',L'ゃ',L'や',L'ゅ',L'ゆ',L'ょ',
	L'よ',L'ら',L'り',L'る',L'れ',L'ろ',L'ゎ',L'わ',
	L'ゐ',L'ゑ',L'を',L'ん',L'　',L'　',L'　',L'ゝ',
	L'ゞ',L'ー',L'。',L'「',L'」',L'、',L'・'
};
static const wchar_t KanaTable[]={
	L'ァ',L'ア',L'ィ',L'イ',L'ゥ',L'ウ',L'ェ',
	L'エ',L'ォ',L'オ',L'カ',L'ガ',L'キ',L'ギ',L'ク',
	L'グ',L'ケ',L'ゲ',L'コ',L'ゴ',L'サ',L'ザ',L'シ',
	L'ジ',L'ス',L'ズ',L'セ',L'ゼ',L'ソ',L'ゾ',L'タ',
	L'ダ',L'チ',L'ヂ',L'ッ',L'ツ',L'ヅ',L'テ',L'デ',
	L'ト',L'ド',L'ナ',L'ニ',L'ヌ',L'ネ',L'ノ',L'ハ',
	L'バ',L'パ',L'ヒ',L'ビ',L'ピ',L'フ',L'ブ',L'プ',
	L'ヘ',L'ベ',L'ペ',L'ホ',L'ボ',L'ポ',L'マ',L'ミ',
	L'ム',L'メ',L'モ',L'ャ',L'ヤ',L'ュ',L'ユ',L'ョ',
	L'ヨ',L'ラ',L'リ',L'ル',L'レ',L'ロ',L'ヮ',L'ワ',
	L'ヰ',L'ヱ',L'ヲ',L'ン',L'ヴ',L'ヵ',L'ヶ',L'ヽ',
	L'ヾ',L'ー',L'。',L'「',L'」',L'、',L'・'
};

//デフォルトマクロ文(NULは効果がないと規定されている)
static const BYTE DefaultMacro[][20]={
	{ 0x1B,0x24,0x39,0x1B,0x29,0x4A,0x1B,0x2A,0x30,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x24,0x39,0x1B,0x29,0x31,0x1B,0x2A,0x30,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x24,0x39,0x1B,0x29,0x20,0x40,0x1B,0x2A,0x30,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x32,0x1B,0x29,0x34,0x1B,0x2A,0x35,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x32,0x1B,0x29,0x33,0x1B,0x2A,0x35,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x32,0x1B,0x29,0x20,0x40,0x1B,0x2A,0x35,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x20,0x41,0x1B,0x29,0x20,0x42,0x1B,0x2A,0x20,0x43,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x20,0x44,0x1B,0x29,0x20,0x45,0x1B,0x2A,0x20,0x46,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x20,0x47,0x1B,0x29,0x20,0x48,0x1B,0x2A,0x20,0x49,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x20,0x4A,0x1B,0x29,0x20,0x4B,0x1B,0x2A,0x20,0x4C,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x20,0x4D,0x1B,0x29,0x20,0x4E,0x1B,0x2A,0x20,0x4F,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x24,0x39,0x1B,0x29,0x20,0x42,0x1B,0x2A,0x30,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x24,0x39,0x1B,0x29,0x20,0x43,0x1B,0x2A,0x30,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x24,0x39,0x1B,0x29,0x20,0x44,0x1B,0x2A,0x30,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x31,0x1B,0x29,0x30,0x1B,0x2A,0x4A,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x4A,0x1B,0x29,0x32,0x1B,0x2A,0x20,0x41,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D }
};

#endif //Table

wchar_t CDRCMap::GetUCS(WORD cc) const
{
	for( int i=0 ; i<m_used; i++ ){
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
}

CARIB8CharDecode::~CARIB8CharDecode(void)
{
}

//STD-B24で規定される初期化動作をおこなう
BOOL CARIB8CharDecode::InitCaption(void)
{
	m_G0.iMF = MF_JIS_KANJI1;
	m_G0.iMode = MF_MODE_G;
	m_G0.iByte = 2;

	m_G1.iMF = MF_ASCII;
	m_G1.iMode = MF_MODE_G;
	m_G1.iByte = 1;

	m_G2.iMF = MF_HIRA;
	m_G2.iMode = MF_MODE_G;
	m_G2.iByte = 1;

	m_G3.iMF = MF_MACRO;
	m_G3.iMode = MF_MODE_DRCS;
	m_G3.iByte = 1;

	m_GL = &m_G0;
	m_GR = &m_G2;

	m_strDecode = L"";
	m_emStrSize = STR_NORMAL;

	m_bCharColorIndex = 7;
	m_bBackColorIndex = 8;
	m_bRasterColorIndex = 8;
	m_bDefPalette = 0;

	m_bUnderLine = FALSE;
	m_bShadow = FALSE;
	m_bBold = FALSE;
	m_bItalic = FALSE;
	m_bFlushMode = 0;
	m_bHLC = 0;

	switch(m_wSWFMode){
	case 7:
		//960x540横
		m_wClientW = 960;
		m_wClientH = 540;
		m_wCharHInterval = 4;
		m_wCharVInterval = 24;
		break;
	case 8:
		//960x540縦
		m_wClientW = 960;
		m_wClientH = 540;
		m_wCharHInterval = 12;
		m_wCharVInterval = 24;
		break;
	case 9:
		//720x480横
		m_wClientW = 720;
		m_wClientH = 480;
		m_wCharHInterval = 4;
		m_wCharVInterval = 16;
		break;
	case 10:
		//720x480縦
		m_wClientW = 720;
		m_wClientH = 480;
		m_wCharHInterval = 8;
		m_wCharVInterval = 24;
		break;
	default:
		return FALSE;
	}
	m_wClientX = 0;
	m_wClientY = 0;
	m_wPosStartX = m_wPosX = m_wClientX;
	m_wPosY = m_wClientY + GetLineDirSize() - 1;
	m_wCharW = 36;
	m_wCharH = 36;
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
BOOL CARIB8CharDecode::Caption( const BYTE* pbSrc, DWORD dwSrcSize, vector<CAPTION_DATA>* pCaptionList, CDRCMap* pDRCMap, WORD wInitSWFMode )
{
	if( pbSrc == NULL || dwSrcSize == 0 || pCaptionList == NULL || pDRCMap == NULL ){
		return FALSE;
	}
	m_bSpacing = FALSE;
	m_pCaptionList = pCaptionList;
	m_pDRCMap = pDRCMap;
	m_wSWFMode = m_wInitSWFMode = wInitSWFMode;
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

		//1バイト目チェック
		if( pbSrc[dwReadSize] <= 0x20 ){
			//C0制御コード
			bRet = C0( pbSrc+dwReadSize, dwSrcSize-dwReadSize, &dwReadBuff );
		}else if( pbSrc[dwReadSize] < 0x7F ){
			//GL符号領域
			bRet = GL_GR( pbSrc+dwReadSize, dwSrcSize-dwReadSize, &dwReadBuff, m_GL );
		}else if( pbSrc[dwReadSize] <= 0xA0 ){
			//C1制御コード
			bRet = C1( pbSrc+dwReadSize, dwSrcSize-dwReadSize, &dwReadBuff );
		}else if( pbSrc[dwReadSize] < 0xFF ){
			//GR符号領域
			bRet = GL_GR( pbSrc+dwReadSize, dwSrcSize-dwReadSize, &dwReadBuff, m_GR );
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

	DWORD dwReadSize = 0;
	DWORD dwReadBuff = 0;
	m_bSpacing = FALSE;

	switch(pbSrc[0]){
	case 0x20:
		//SP 背景色空白
		m_strDecode += L'　';
		ActivePositionForward(1);
		m_bSpacing = TRUE;
		dwReadSize = 1;
		break;
	case 0x0D:
		//APR 改行
		CheckModify();
		m_wPosX = m_wClientX;
		m_wPosY += GetLineDirSize();
		if( m_wPosY >= m_wClientY + m_wClientH ){
			m_wPosY = m_wClientY + GetLineDirSize() - 1;
		}
		m_wPosStartX = m_wPosX;
		dwReadSize = 1;
		break;
	case 0x0E:
		//LS1 GLにG1セット
		m_GL = &m_G1;
		dwReadSize = 1;
		break;
	case 0x0F:
		//LS0 GLにG0セット
		m_GL = &m_G0;
		dwReadSize = 1;
		break;
	case 0x19:
		//SS2 シングルシフト
		//G2で呼ぶ(マクロ展開を考慮してGLは入れ替えない)
		if( GL_GR( pbSrc+1, dwSrcSize-1, &dwReadBuff, &m_G2 ) == FALSE ){
			return FALSE;
		}
		dwReadSize = 1+dwReadBuff;
		break;
	case 0x1D:
		//SS3 シングルシフト
		//G3で呼ぶ(マクロ展開を考慮してGLは入れ替えない)
		if( GL_GR( pbSrc+1, dwSrcSize-1, &dwReadBuff, &m_G3 ) == FALSE ){
			return FALSE;
		}
		dwReadSize = 1+dwReadBuff;
		break;
	case 0x1B:
		//ESC エスケープシーケンス
		if( ESC( pbSrc+1, dwSrcSize-1, &dwReadBuff ) == FALSE ){
			return FALSE;
		}
		dwReadSize = 1+dwReadBuff;
		break;
	case 0x08:
		//APB 動作位置後退
		{
			CheckModify();
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
			dwReadSize = 1;
		}
		break;
	case 0x09:
		//APF 動作位置前進
		CheckModify();
		ActivePositionForward(1);
		//文字出力がないのでm_wPosStartXを更新しないと進行に矛盾がおこる
		m_wPosStartX = m_wPosX;
		dwReadSize = 1;
		break;
	case 0x0A:
		//APD 動作行前進
		CheckModify();
		m_wPosY += GetLineDirSize();
		if( m_wPosY >= m_wClientY + m_wClientH ){
			m_wPosY = m_wClientY + GetLineDirSize() - 1;
		}
		m_wPosStartX = m_wPosX;
		dwReadSize = 1;
		break;
	case 0x0B:
		//APU 動作行後退
		{
			CheckModify();
			WORD wDirY = GetLineDirSize();
			if( m_wPosY < m_wClientY + wDirY*2-1 ){
				m_wPosY = m_wClientY + (m_wClientH/wDirY+1) * wDirY - 1;
			}
			m_wPosY -= wDirY;
			m_wPosStartX = m_wPosX;
			dwReadSize = 1;
		}
		break;
	case 0x16:
		//PAPF 指定動作位置前進
		if( dwSrcSize < 2 ){
			return FALSE;
		}
		CheckModify();
		ActivePositionForward(pbSrc[1] - 0x40);
		m_wPosStartX = m_wPosX;
		dwReadSize = 2;
		break;
	case 0x1C:
		//APS 動作位置指定
		if( dwSrcSize < 3 ){
			return FALSE;
		}
		CheckModify();
		//動作位置基準点は左下なので1行前進しておく(参考:mark10als)
		m_wPosY = m_wClientY + GetLineDirSize() * (pbSrc[1] - 0x40 + 1) - 1;
		m_wPosX = m_wClientX + GetCharDirSize() * (pbSrc[2] - 0x40);
		m_wPosStartX = m_wPosX;
		dwReadSize = 3;
		break;
	case 0x0C:
		//CS
		{
			dwReadSize = 1;
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
		dwReadSize = 1;
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
	DWORD dwReadSize = 0;
	DWORD dwReadBuff = 0;
	m_bSpacing = FALSE;

	if( pbSrc[0] != 0x7F ){
		//基本的に文字修飾に変更を伴うので本文をリストに移しておく
		CheckModify();
	}
	switch(pbSrc[0]){
	case 0x7F:
		//DEL 前景色空白
		m_strDecode += L'■';
		ActivePositionForward(1);
		m_bSpacing = TRUE;
		dwReadSize = 1;
		break;
	case 0x89:
		//MSZ 半角指定
		m_emStrSize = STR_MEDIUM;
		dwReadSize = 1;
		break;
	case 0x8A:
		//NSZ 全角指定
		m_emStrSize = STR_NORMAL;
		dwReadSize = 1;
		break;
	case 0x80:
		//BKF 文字黒
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x00;
		dwReadSize = 1;
		break;
	case 0x81:
		//RDF 文字赤
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x01;
		dwReadSize = 1;
		break;
	case 0x82:
		//GRF 文字緑
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x02;
		dwReadSize = 1;
		break;
	case 0x83:
		//YLF 文字黄
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x03;
		dwReadSize = 1;
		break;
	case 0x84:
		//BLF 文字青
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x04;
		dwReadSize = 1;
		break;
	case 0x85:
		//MGF 文字マゼンタ
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x05;
		dwReadSize = 1;
		break;
	case 0x86:
		//CNF 文字シアン
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x06;
		dwReadSize = 1;
		break;
	case 0x87:
		//WHF 文字白
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x07;
		dwReadSize = 1;
		break;
	case 0x88:
		//SSZ 小型サイズ
		m_emStrSize = STR_SMALL;
		dwReadSize = 1;
		break;
	case 0x8B:
		//SZX 指定サイズ
		if( dwSrcSize < 2 ) return FALSE;
		if( pbSrc[1] == 0x60 ){
			m_emStrSize = STR_MICRO;
		}else if( pbSrc[1] == 0x41 ){
			m_emStrSize = STR_HIGH_W;
		}else if( pbSrc[1] == 0x44 ){
			m_emStrSize = STR_WIDTH_W;
		}else if( pbSrc[1] == 0x45 ){
			m_emStrSize = STR_W;
		}else if( pbSrc[1] == 0x6B ){
			m_emStrSize = STR_SPECIAL_1;
		}else if( pbSrc[1] == 0x64 ){
			m_emStrSize = STR_SPECIAL_2;
		}
		dwReadSize = 2;
		break;
	case 0x90:
		//COL 色指定
		if( dwSrcSize < 2 ) return FALSE;
		if( pbSrc[1] == 0x20 ){
			if( dwSrcSize < 3 ) return FALSE;
			dwReadSize = 3;
			//規定によりパレットは0から127まで使われる
			m_bDefPalette = pbSrc[2]&0x07;
		}else{
			switch(pbSrc[1]&0xF0){
				case 0x40:
					m_bCharColorIndex = (m_bDefPalette<<4) | (pbSrc[1]&0x0F);
					break;
				case 0x50:
					m_bBackColorIndex = (m_bDefPalette<<4) | (pbSrc[1]&0x0F);
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
			dwReadSize = 2;
		}
		break;
	case 0x91:
		//FLC フラッシング制御
		if( dwSrcSize < 2 ) return FALSE;
		if( pbSrc[1] == 0x40 ){
			m_bFlushMode = 1;
		}else if( pbSrc[1] == 0x47 ){
			m_bFlushMode = 2;
		}else if( pbSrc[1] == 0x4F ){
			m_bFlushMode = 0;
		}
		dwReadSize = 2;
		break;
	case 0x93:
		//POL パターン極性
		//未サポート
		if( dwSrcSize < 2 ) return FALSE;
		dwReadSize = 2;
		break;
	case 0x94:
		//WMM 書き込みモード変更
		//未サポート
		if( dwSrcSize < 2 ) return FALSE;
		dwReadSize = 2;
		break;
	case 0x95:
		//MACRO マクロ定義
		//未サポート(MACRO 0x4Fまで送る)
		dwReadSize = 2;
		do{
			if( ++dwReadSize > dwSrcSize ){
				return FALSE;
			}
		}while( pbSrc[dwReadSize-2] != 0x95 || pbSrc[dwReadSize-1] != 0x4F );
		break;
	case 0x97:
		//HLC 囲み制御
		if( dwSrcSize < 2 ) return FALSE;
		m_bHLC = pbSrc[1]&0x0F;
		dwReadSize = 2;
		break;
	case 0x98:
		//RPC 文字繰り返し
		if( dwSrcSize < 2 || m_bRPC ) return FALSE;
		m_bRPC = TRUE;
		m_wRPC = pbSrc[1]-0x40 == 0 ? 0xFF : pbSrc[1]-0x40;
		dwReadSize = 2;
		break;
	case 0x99:
		//SPL アンダーライン モザイクの終了
		m_bBold = FALSE;
		dwReadSize = 1;
		break;
	case 0x9A:
		//STL アンダーライン モザイクの開始
		m_bBold = TRUE;
		dwReadSize = 1;
		break;
	case 0x9D:
		//TIME 時間制御
		if( dwSrcSize < 3 ) return FALSE;
		if( pbSrc[1] == 0x20 ){
			m_dwWaitTime += (pbSrc[2]-0x40) * 100;
			dwReadSize = 3;
		}else{
			//未サポート
			dwReadSize = 1;
			do{
				if( ++dwReadSize > dwSrcSize ){
					return FALSE;
				}
			}while( pbSrc[dwReadSize-1] < 0x40 || 0x43 < pbSrc[dwReadSize-1] );
		}
		break;
	case 0x9B:
		//CSI コントロールシーケンス
		if( CSI( pbSrc, dwSrcSize, &dwReadBuff ) == FALSE ){
			return FALSE;
		}
		dwReadSize = dwReadBuff;
		break;
	default:
		//未サポートの制御コード
		dwReadSize = 1;
		break;
	}

	*pdwReadSize = dwReadSize;

	return TRUE;
}

BOOL CARIB8CharDecode::GL_GR( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize, const MF_MODE* mode )
{
	if( dwSrcSize == 0 || (pbSrc[0]&0x7F) <= 0x20 || 0x7F <= (pbSrc[0]&0x7F) ){
		return FALSE;
	}

	if( mode->iMode == MF_MODE_G ){
		//文字コード
		m_bSpacing = TRUE;
		switch( mode->iMF ){
			case MF_JISX_KANA:
				{
				//JISX X0201の0x7FまではASCIIと同じ -> STD-B24には記述がみつからなかったので一旦普通の扱いにする
				//Gセットのカタカナ系集合
				m_strDecode += KanaTable[(pbSrc[0]&0x7F)-0x21];
				ActivePositionForward(1);
				}
				break;
			case MF_ASCII:
			case MF_PROP_ASCII:
				{
				//全角なのでテーブルからコード取得
				m_strDecode += AsciiTable[(pbSrc[0]&0x7F)-0x21];
				ActivePositionForward(1);
				}
				break;
			case MF_HIRA:
			case MF_PROP_HIRA:
				{
				//Gセットのひらがな系集合
				m_strDecode += HiraTable[(pbSrc[0]&0x7F)-0x21];
				ActivePositionForward(1);
				}
				break;
			case MF_KANA:
			case MF_PROP_KANA:
				{
				//Gセットのカタカナ系集合
				m_strDecode += KanaTable[(pbSrc[0]&0x7F)-0x21];
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
				unsigned char ucFirst = pbSrc[0]&0x7F;
				unsigned char ucSecond = pbSrc[1]&0x7F;
				if( ToSJIS(&ucFirst, &ucSecond) == FALSE ){
					AddGaijiToString(pbSrc[0]&0x7F, pbSrc[1]&0x7F);
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
			if( (int)dwSrcSize < mode->iByte ){
				return FALSE;
			}
			//1バイトDRCSは0x01xxから0x0Fxxに符号シフト
			WORD cc = (WORD)( mode->iMF==MF_DRCS_0 ? ((pbSrc[0]&0x7F)<<8) | (pbSrc[1]&0x7F) :
			                                         ((mode->iMF-MF_DRCS_0)<<8) | (pbSrc[0]&0x7F) );
			m_strDecode += m_pDRCMap->MapUCS(cc);
			ActivePositionForward(1);
			m_bSpacing = TRUE;
		}else if( mode->iMF == MF_MACRO ){
			DWORD dwTemp=0;
			//マクロ
			if( 0x60 <= (pbSrc[0]&0x7F) && (pbSrc[0]&0x7F) <= 0x6F ){
				if( Analyze(DefaultMacro[pbSrc[0]&0x0F], sizeof(DefaultMacro[0]), &dwTemp) == FALSE ){
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

BOOL CARIB8CharDecode::ToSJIS( unsigned char *pucFirst, unsigned char *pucSecond )
{
	unsigned char &ucFirst = *pucFirst;
	unsigned char &ucSecond = *pucSecond;

	if( ucFirst >= 0x75 && ucSecond >= 0x21 ){
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

	// ARIB 追加漢字
	if( (usRow=0x7521) <= usSrc && usSrc < usRow + G_CELL_SIZE ){
		m_strDecode += m_GaijiTable[5*G_CELL_SIZE + usSrc-usRow];
	}else if( (usRow=0x7621) <= usSrc && usSrc < usRow + G_CELL_SIZE ){
		m_strDecode += m_GaijiTable[6*G_CELL_SIZE + usSrc-usRow];
	// ARIB 追加記号
	}else if( (usRow=0x7A21) <= usSrc && usSrc < usRow + G_CELL_SIZE ){
		m_strDecode += m_GaijiTable[0*G_CELL_SIZE + usSrc-usRow];
	}else if( (usRow=0x7B21) <= usSrc && usSrc < usRow + G_CELL_SIZE ){
		m_strDecode += m_GaijiTable[1*G_CELL_SIZE + usSrc-usRow];
	}else if( (usRow=0x7C21) <= usSrc && usSrc < usRow + G_CELL_SIZE ){
		m_strDecode += m_GaijiTable[2*G_CELL_SIZE + usSrc-usRow];
	}else if( (usRow=0x7D21) <= usSrc && usSrc < usRow + G_CELL_SIZE ){
		m_strDecode += m_GaijiTable[3*G_CELL_SIZE + usSrc-usRow];
	}else if( (usRow=0x7E21) <= usSrc && usSrc < usRow + G_CELL_SIZE ){
		m_strDecode += m_GaijiTable[4*G_CELL_SIZE + usSrc-usRow];
	}else{
		m_strDecode += L'・';
	}

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

//ESC()と違いpbSrcの0要素目にCSI(0x9B)自身を含むので注意
BOOL CARIB8CharDecode::CSI( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize )
{
	DWORD dwReadSize = 1;

	//終端文字をさがす(シーケンス終了まで中間文字0x20は含まれない)
	do{
		if( (++dwReadSize)+1 > dwSrcSize ){
			return FALSE;
		}
	}while( pbSrc[dwReadSize-1] != 0x20 );
	
	switch(pbSrc[dwReadSize]){
		case 0x53:
			//SWF
			{
				BOOL bCharMode = FALSE;
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						if( bCharMode == FALSE ){
							m_wSWFMode = wParam;
						}else{
							//未サポート
						}
					}else if(pbSrc[i] == 0x3B){
						bCharMode = TRUE;
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
				if( InitCaption() == FALSE ){
					return FALSE;
				}
			}
			break;
		case 0x6E:
			//RCS
			{
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						m_bRasterColorIndex = (BYTE)(wParam&0x7F);
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
			}
			break;
		case 0x61:
			//ACPS
			{
				BOOL bSeparate = FALSE;
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						if( bSeparate == FALSE ){
							m_wPosStartX = m_wPosX = wParam;
						}else{
							m_wPosY = wParam;
						}
					}else if(pbSrc[i] == 0x3B){
						bSeparate = TRUE;
						m_wPosStartX = m_wPosX = wParam;
						wParam = 0;
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
			}
			break;
		case 0x56:
			//SDF
			{
				BOOL bSeparate = FALSE;
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						if( bSeparate == FALSE ){
							m_wClientW = wParam;
						}else{
							m_wClientH = wParam;
						}
					}else if(pbSrc[i] == 0x3B){
						bSeparate = TRUE;
						m_wClientW = wParam;
						wParam = 0;
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
			}
			break;
		case 0x5F:
			//SDP
			{
				BOOL bSeparate = FALSE;
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						if( bSeparate == FALSE ){
							m_wClientX = wParam;
						}else{
							m_wClientY = wParam;
						}
					}else if(pbSrc[i] == 0x3B){
						bSeparate = TRUE;
						m_wClientX = wParam;
						wParam = 0;
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
			}
			break;
		case 0x57:
			//SSM
			{
				BOOL bSeparate = FALSE;
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						if( bSeparate == FALSE ){
							m_wCharW = wParam;
						}else{
							m_wCharH = wParam;
						}
					}else if(pbSrc[i] == 0x3B){
						bSeparate = TRUE;
						m_wCharW = wParam;
						wParam = 0;
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
			}
			break;
		case 0x58:
			//SHS
			{
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						m_wCharHInterval = wParam;
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
			}
			break;
		case 0x59:
			//SVS
			{
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						m_wCharVInterval = wParam;
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
			}
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
			{
				BOOL bSeparate = FALSE;
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						if( bSeparate == FALSE ){
							if( wParam == 0x02 ){
								m_bShadow = TRUE;
							}
						}
					}else if(pbSrc[i] == 0x3B){
						bSeparate = TRUE;
						if( wParam == 0x02 ){
							m_bShadow = TRUE;
						}
						wParam = 0;
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
			}
			break;
		case 0x64:
			//MDF
			{
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						if( wParam == 0 ){
							m_bBold = FALSE;
							m_bItalic = FALSE;
						}else if( wParam == 1 ){
							m_bBold = TRUE;
						}else if( wParam == 2 ){
							m_bItalic = TRUE;
						}else if( wParam == 3 ){
							m_bBold = TRUE;
							m_bItalic = TRUE;
						}
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
			}
			break;
		case 0x66:
			//XCS
			//未サポート
			break;
		case 0x68:
			//PRA
			//未サポート
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
void CARIB8CharDecode::CheckModify(void)
{
	if( m_strDecode.length() > 0 ){
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
//	OutputDebugStringA(m_strDecode.c_str());

	pItem->stCharColor = DefClut[m_bCharColorIndex];
	pItem->stBackColor = DefClut[m_bBackColorIndex];
	pItem->stRasterColor = DefClut[m_bRasterColorIndex];

	pItem->bUnderLine = m_bUnderLine;
	pItem->bShadow = m_bShadow;
	pItem->bBold = m_bBold;
	pItem->bItalic = m_bItalic;
	pItem->bFlushMode = m_bFlushMode;
	pItem->bHLC = m_bHLC<<4;

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
	if( m_emStrSize==STR_SMALL || m_emStrSize==STR_MEDIUM ){
		wSize /= 2;
	}else if( m_emStrSize==STR_W || m_emStrSize==STR_WIDTH_W ){
		wSize *= 2;
	}
	return wSize==0 ? 1 : wSize;
}

WORD CARIB8CharDecode::GetLineDirSize(void) const
{
	WORD wSize = m_wCharH + m_wCharVInterval;
	if( m_emStrSize==STR_SMALL ){
		wSize /= 2;
	}else if( m_emStrSize==STR_W || m_emStrSize==STR_HIGH_W ){
		wSize *= 2;
	}
	return wSize==0 ? 1 : wSize;
}

#define GET_PIXEL(b,x) ( ((b)[(x)/4]>>((3-(x)%4)*2)) & 0x3 )

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
			BYTE bWidth = pbSrc[dwRead+2];
			BYTE bHeight = pbSrc[dwRead+3];
			if( bMode != 1 || bDepth != 2 || bWidth > DRCS_SIZE_MAX || bHeight > DRCS_SIZE_MAX ){
				//未サポート(運用規定外)
				return FALSE;
			}
			dwRead += 4;
			if( dwSrcSize < dwRead + (bHeight*bWidth+3)/4 ){
				return FALSE;
			}
			//ここで新規追加
			pDRCList->resize(pDRCList->size() + 1);
			DRCS_PATTERN &drcs = pDRCList->back();
			drcs.wDRCCode = wDRCCode;

			DWORD dwSizeImage = 0;
			//ビットマップの仕様により左下から走査
			for( int y=bHeight-1; y>=0; y-- ){
				for( int x=0; x<bWidth; x++ ){
					if( x%2==0 ){
						drcs.bBitmap[dwSizeImage++] = GET_PIXEL(pbSrc+dwRead, y*bWidth+x) << 4;
					}else{
						drcs.bBitmap[dwSizeImage-1] |= GET_PIXEL(pbSrc+dwRead, y*bWidth+x);
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

			dwRead += (bHeight*bWidth+3)/4;
		}
	}
	return TRUE;
}

BOOL CARIB8CharDecode::GetGaijiTable(WCHAR* pTable, DWORD* pdwTableSize) const
{
	if( pTable==NULL || pdwTableSize==NULL ){
		return FALSE;
	}
	DWORD dwCopyBytes = min(*pdwTableSize * sizeof(WCHAR), sizeof(m_GaijiTable));
	memcpy(pTable, m_GaijiTable, dwCopyBytes);
	*pdwTableSize = dwCopyBytes / sizeof(WCHAR);
	return TRUE;
}

BOOL CARIB8CharDecode::SetGaijiTable(const WCHAR* pTable, DWORD* pdwTableSize)
{
	if( pTable==NULL || pdwTableSize==NULL ){
		return FALSE;
	}
	DWORD dwCopyBytes = min(*pdwTableSize * sizeof(WCHAR), sizeof(m_GaijiTable));
	memcpy(m_GaijiTable, pTable, dwCopyBytes);
	*pdwTableSize = dwCopyBytes / sizeof(WCHAR);
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
