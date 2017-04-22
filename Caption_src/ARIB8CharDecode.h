#pragma once

#include <string>
#include <vector>
using std::wstring;
using std::vector;

//文字符号集合
//Gセット
#define MF_JIS_KANJI1 0x39 //JIS互換漢字1面
#define MF_JIS_KANJI2 0x3A //JIS互換漢字2面
#define MF_KIGOU 0x3B //追加記号
#define MF_ASCII 0x4A //英数
#define MF_HIRA  0x30 //平仮名
#define MF_KANA  0x31 //片仮名
#define MF_KANJI 0x42 //漢字
#define MF_MOSAIC_A 0x32 //モザイクA
#define MF_MOSAIC_B 0x33 //モザイクB
#define MF_MOSAIC_C 0x34 //モザイクC
#define MF_MOSAIC_D 0x35 //モザイクD
#define MF_PROP_ASCII 0x36 //プロポーショナル英数
#define MF_PROP_HIRA  0x37 //プロポーショナル平仮名
#define MF_PROP_KANA  0x38 //プロポーショナル片仮名
#define MF_JISX_KANA 0x49 //JIX X0201片仮名
//DRCS
#define MF_DRCS_0 0x40 //DRCS-0
#define MF_DRCS_1 0x41 //DRCS-1
#define MF_DRCS_2 0x42 //DRCS-2
#define MF_DRCS_3 0x43 //DRCS-3
#define MF_DRCS_4 0x44 //DRCS-4
#define MF_DRCS_5 0x45 //DRCS-5
#define MF_DRCS_6 0x46 //DRCS-6
#define MF_DRCS_7 0x47 //DRCS-7
#define MF_DRCS_8 0x48 //DRCS-8
#define MF_DRCS_9 0x49 //DRCS-9
#define MF_DRCS_10 0x4A //DRCS-10
#define MF_DRCS_11 0x4B //DRCS-11
#define MF_DRCS_12 0x4C //DRCS-12
#define MF_DRCS_13 0x4D //DRCS-13
#define MF_DRCS_14 0x4E //DRCS-14
#define MF_DRCS_15 0x4F //DRCS-15
#define MF_MACRO 0x70 //マクロ

//符号集合の分類
#define MF_MODE_G 1 //Gセット
#define MF_MODE_DRCS 2 //DRCS

#define G_CELL_SIZE 94

//文字サイズ
typedef enum{
	STR_SMALL = 0, //SSZ
	STR_MEDIUM, //MSZ
	STR_NORMAL, //NSZ
	STR_MICRO, //SZX 0x60
	STR_HIGH_W, //SZX 0x41
	STR_WIDTH_W, //SZX 0x44
	STR_W, //SZX 0x45
	STR_SPECIAL_1, //SZX 0x6B
	STR_SPECIAL_2, //SZX 0x64
} STRING_SIZE;

struct CAPTION_CHAR_DATA{
	wstring strDecode;
	STRING_SIZE emCharSizeMode;

	CLUT_DAT_DLL stCharColor;
	CLUT_DAT_DLL stBackColor;
	CLUT_DAT_DLL stRasterColor;

	BOOL bUnderLine;
	BOOL bShadow;
	BOOL bBold;
	BOOL bItalic;
	BYTE bFlushMode;
	BYTE bHLC;
	BYTE bPRA;

	WORD wCharW;
	WORD wCharH;
	WORD wCharHInterval;
	WORD wCharVInterval;
};

struct CAPTION_DATA{
	//bClear==TRUEのときdwWaitTime以外のフィールドは未定義
	BOOL bClear;
	WORD wSWFMode;
	WORD wClientX;
	WORD wClientY;
	WORD wClientW;
	WORD wClientH;
	WORD wPosX;
	WORD wPosY;
	//表示区画を同じくする本文のリスト
	vector<CAPTION_CHAR_DATA> CharList;
	DWORD dwWaitTime;
};

//DRCS図形の縦横最大サイズ(運用規定より)
#define DRCS_SIZE_MAX 36

struct DRCS_PATTERN{
	WORD wDRCCode;
	WORD wGradation;
	BITMAPINFOHEADER bmiHeader;
	BYTE bBitmap[(DRCS_SIZE_MAX*4+31)/32*4 * DRCS_SIZE_MAX];
	DRCS_PATTERN() {}
};

//参考:up0511mod
//U+EC00～U+ECFFの範囲でDRCSをUCSにマッピングする
class CDRCMap
{
	int m_used, m_front;
	WORD m_wCodeMap[0x100];
public:
	CDRCMap(void) : m_used(0), m_front(0) {}
	void Clear(void) { m_used = m_front = 0; }
	wchar_t GetUCS(WORD cc) const;
	wchar_t MapUCS(WORD cc);
};

class CARIB8CharDecode
{
public:
	CARIB8CharDecode(void);
	~CARIB8CharDecode(void);
	//字幕を想定したワイド文字列への変換
	BOOL Caption( const BYTE* pbSrc, DWORD dwSrcSize, vector<CAPTION_DATA>* pCaptionList, CDRCMap* pDRCMap, WORD wInitSWFMode );
	//DRCSヘッダの分析(参考:mark10als)
	static BOOL DRCSHeaderparse( const BYTE* pbSrc, DWORD dwSrcSize, vector<DRCS_PATTERN>* pDRCList, BOOL bDRCS_0 );
	BOOL GetGaijiTable(WCHAR* pTable, DWORD* pdwTableSize) const;
	BOOL SetGaijiTable(const WCHAR* pTable, DWORD* pdwTableSize);
	BOOL ResetGaijiTable(DWORD* pdwTableSize);

protected:
	struct MF_MODE{
		int iMF; //文字符号集合
		int iMode; //符号集合の分類
		int iByte; //読み込みバイト数
	};

	MF_MODE m_G0;
	MF_MODE m_G1;
	MF_MODE m_G2;
	MF_MODE m_G3;
	MF_MODE* m_GL;
	MF_MODE* m_GR;

	//デコードした文字列
	wstring m_strDecode;
	//文字サイズ
	STRING_SIZE m_emStrSize;

	//CLUTのインデックス
	BYTE m_bCharColorIndex;
	BYTE m_bBackColorIndex;
	BYTE m_bRasterColorIndex;
	BYTE m_bDefPalette;

	BOOL m_bUnderLine;
	BOOL m_bShadow;
	BOOL m_bBold;
	BOOL m_bItalic;
	BYTE m_bFlushMode;
	BYTE m_bHLC;
	BYTE m_bPRA;
	BOOL m_bRPC;
	//文字繰り返しの残りカウント(ただし0x40以上のときは表示領域端まで)
	WORD m_wRPC;
	//C0、C1、GL_GR呼び出し後、解析した文字がスペーシング文字であったかどうか
	BOOL m_bSpacing;

	//表示書式(運用規定で960x540または720x480)
	WORD m_wSWFMode;
	//初期化動作時の表示書式
	WORD m_wInitSWFMode;
	//表示領域の位置(字幕プレーン左上角からの座標)
	WORD m_wClientX;
	WORD m_wClientY;
	//表示領域の横/縦ドット数
	WORD m_wClientW;
	WORD m_wClientH;
	//現在の文字の動作位置(字幕プレーン左上角からの座標)
	WORD m_wPosX;
	WORD m_wPosY;
	//現在の文字列1行の動作位置(字幕プレーン左上角からの座標)
	WORD m_wPosStartX;
	//初期動作位置の設定が完了したかどうか
	BOOL m_bPosInit;
	//文字サイズ
	WORD m_wCharW;
	WORD m_wCharH;
	//字間隔/行間隔
	WORD m_wCharHInterval;
	WORD m_wCharVInterval;
	//表示タイミング(ミリ秒)
	DWORD m_dwWaitTime;

	vector<CAPTION_DATA>* m_pCaptionList;
	CDRCMap* m_pDRCMap;
	WCHAR m_GaijiTable[G_CELL_SIZE * 7][2];
protected:
	BOOL InitCaption(void);
	BOOL Analyze( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize );

	const BOOL IsCaptionPropertyChanged(void) const;
	void CreateCaptionData(CAPTION_DATA* pItem) const;
	void CreateCaptionCharData(CAPTION_CHAR_DATA* pItem) const;
	void CheckModify(BOOL bForce=FALSE);

	//制御符号
	BOOL C0( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize );
	BOOL C1( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize );
	BOOL GL_GR( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize, const MF_MODE* mode );
	//エスケープシーケンス
	BOOL ESC( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize );
	//JIS->SJIS変換
	static BOOL ToSJIS( unsigned char *pucFirst, unsigned char *pucSecond );
	void AddGaijiToString( const BYTE bFirst, const BYTE bSecond );

	BOOL CSI( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize );

	void AddSJISToString( unsigned char ucFirst, unsigned char ucSecond );
	void ActivePositionForward( int nCount );
	WORD GetCharDirSize(void) const;
	WORD GetLineDirSize(void) const;
};
