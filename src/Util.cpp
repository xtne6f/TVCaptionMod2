#include <Windows.h>
#include <Shlwapi.h>
#include <CommCtrl.h>
#include "Util.h"

// GetPrivateProfileInt()の負値対応版
// 実際にはGetPrivateProfileInt()も負値を返すが、仕様ではない
int GetPrivateProfileSignedInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, int nDefault, LPCTSTR lpFileName)
{
    TCHAR szVal[32];
    GetPrivateProfileString(lpAppName, lpKeyName, TEXT(""), szVal, _countof(szVal), lpFileName);
    int nRet;
    return ::StrToIntEx(szVal, STIF_DEFAULT, &nRet) ? nRet : nDefault;
}

BOOL WritePrivateProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, int value, LPCTSTR lpFileName)
{
    TCHAR szValue[32];
    ::wsprintf(szValue, TEXT("%d"), value);
    return ::WritePrivateProfileString(lpAppName, lpKeyName, szValue, lpFileName);
}

#define PROGRAM_STREAM_MAP          0xBC
#define PADDING_STREAM              0xBE
#define PRIVATE_STREAM_2            0xBF
#define ECM                         0xF0
#define EMM                         0xF1
#define PROGRAM_STREAM_DIRECTORY    0xFF
#define DSMCC_STREAM                0xF2
#define ITU_T_REC_TYPE_E_STREAM     0xF8

// 参考: ITU-T H.222.0 Sec.2.4.3.6
void extract_pes_header(PES_HEADER *dst, const unsigned char *payload, int payload_size/*, int stream_type*/)
{
    const unsigned char *p;

    dst->packet_start_code_prefix = 0;
    if (payload_size < 19) return;

    p = payload;

    dst->packet_start_code_prefix = (p[0]<<16) | (p[1]<<8) | p[2];
    if (dst->packet_start_code_prefix != 1) {
        dst->packet_start_code_prefix = 0;
        return;
    }

    dst->stream_id         = p[3];
    dst->pes_packet_length = (p[4]<<8) | p[5];
    dst->pts_dts_flags     = 0;
    if (dst->stream_id != PROGRAM_STREAM_MAP &&
        dst->stream_id != PADDING_STREAM &&
        dst->stream_id != PRIVATE_STREAM_2 &&
        dst->stream_id != ECM &&
        dst->stream_id != EMM &&
        dst->stream_id != PROGRAM_STREAM_DIRECTORY &&
        dst->stream_id != DSMCC_STREAM &&
        dst->stream_id != ITU_T_REC_TYPE_E_STREAM)
    {
        dst->pts_dts_flags = (p[7]>>6) & 0x03;
        if (dst->pts_dts_flags >= 2) {
            dst->pts_45khz = ((unsigned int)((p[9]>>1)&7)<<29)|(p[10]<<21)|(((p[11]>>1)&0x7f)<<14)|(p[12]<<6)|((p[13]>>2)&0x3f);
            if (dst->pts_dts_flags == 3) {
                dst->dts_45khz = ((unsigned int)((p[14]>>1)&7)<<29)|(p[15]<<21)|(((p[16]>>1)&0x7f)<<14)|(p[17]<<6)|((p[18]>>2)&0x3f);
            }
        }
    }
    // スタフィング(Sec.2.4.3.5)によりPESのうちここまでは必ず読める
}

#if 1 // From: tsselect-0.1.8/tsselect.c (一部改変)
void extract_adaptation_field(ADAPTATION_FIELD *dst, const unsigned char *data)
{
	const unsigned char *p;
	const unsigned char *tail;

	p = data;
	
	memset(dst, 0, sizeof(ADAPTATION_FIELD));
	if( p[0] == 0 ){
		// a single stuffing byte
		dst->adaptation_field_length = 0;
		return;
	}
	if( p[0] > 183 ){
		dst->adaptation_field_length = -1;
		return;
	}

	dst->adaptation_field_length = p[0];
	p += 1;
	tail = p + dst->adaptation_field_length;
	if( (p+1) > tail ){
		memset(dst, 0, sizeof(ADAPTATION_FIELD));
		dst->adaptation_field_length = -1;
		return;
	}

	dst->discontinuity_counter = (p[0] >> 7) & 1;
	dst->random_access_indicator = (p[0] >> 6) & 1;
	dst->elementary_stream_priority_indicator = (p[0] >> 5) & 1;
	dst->pcr_flag = (p[0] >> 4) & 1;
	dst->opcr_flag = (p[0] >> 3) & 1;
	dst->splicing_point_flag = (p[0] >> 2) & 1;
	dst->transport_private_data_flag = (p[0] >> 1) & 1;
	dst->adaptation_field_extension_flag = p[0] & 1;
	
	p += 1;
	
	if(dst->pcr_flag != 0){
		if( (p+6) > tail ){
			memset(dst, 0, sizeof(ADAPTATION_FIELD));
			dst->adaptation_field_length = -1;
			return;
		}
		dst->pcr_45khz = ((unsigned int)p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];
		p += 6;
	}
}
#endif

#if 1 // From: TVTest_0.7.23_Src/Util.cpp
bool CompareLogFont(const LOGFONT *pFont1,const LOGFONT *pFont2)
{
	return memcmp(pFont1,pFont2,28/*offsetof(LOGFONT,lfFaceName)*/)==0
		&& lstrcmp(pFont1->lfFaceName,pFont2->lfFaceName)==0;
}
#endif

#if 1 // From: TVTest_0.7.23_Src/DrawUtil.cpp

namespace DrawUtil {

// 単色で塗りつぶす
bool Fill(HDC hdc,const RECT *pRect,COLORREF Color)
{
	HBRUSH hbr=::CreateSolidBrush(Color);

	if (hbr==NULL)
		return false;
	::FillRect(hdc,pRect,hbr);
	::DeleteObject(hbr);
	return true;
}

// ビットマップを描画する
bool DrawBitmap(HDC hdc,int DstX,int DstY,int DstWidth,int DstHeight,
				HBITMAP hbm,const RECT *pSrcRect,BYTE Opacity)
{
	if (hdc==NULL || hbm==NULL)
		return false;

	int SrcX,SrcY,SrcWidth,SrcHeight;
	if (pSrcRect!=NULL) {
		SrcX=pSrcRect->left;
		SrcY=pSrcRect->top;
		SrcWidth=pSrcRect->right-pSrcRect->left;
		SrcHeight=pSrcRect->bottom-pSrcRect->top;
	} else {
		BITMAP bm;
		if (::GetObject(hbm,sizeof(BITMAP),&bm)!=sizeof(BITMAP))
			return false;
		SrcX=SrcY=0;
		SrcWidth=bm.bmWidth;
		SrcHeight=bm.bmHeight;
	}

	HDC hdcMemory=::CreateCompatibleDC(hdc);
	if (hdcMemory==NULL)
		return false;
	HBITMAP hbmOld=static_cast<HBITMAP>(::SelectObject(hdcMemory,hbm));

	if (Opacity==255) {
		if (SrcWidth==DstWidth && SrcHeight==DstHeight) {
			::BitBlt(hdc,DstX,DstY,DstWidth,DstHeight,
					 hdcMemory,SrcX,SrcY,SRCCOPY);
		} else {
			int OldStretchMode=::SetStretchBltMode(hdc,STRETCH_HALFTONE);
			::StretchBlt(hdc,DstX,DstY,DstWidth,DstHeight,
						 hdcMemory,SrcX,SrcY,SrcWidth,SrcHeight,SRCCOPY);
			::SetStretchBltMode(hdc,OldStretchMode);
		}
	} else {
		BLENDFUNCTION bf={AC_SRC_OVER,0,Opacity,0};
		::GdiAlphaBlend(hdc,DstX,DstY,DstWidth,DstHeight,
						hdcMemory,SrcX,SrcY,SrcWidth,SrcHeight,bf);
	}

	::SelectObject(hdcMemory,hbmOld);
	::DeleteDC(hdcMemory);
	return true;
}

// システムフォントを取得する
bool GetSystemFont(FontType Type,LOGFONT *pLogFont)
{
	if (pLogFont==NULL)
		return false;
	if (Type==FONT_DEFAULT) {
		return ::GetObject(::GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),pLogFont)==sizeof(LOGFONT);
	} else {
		NONCLIENTMETRICS ncm;
		LOGFONT *plf;
		ncm.cbSize=CCSIZEOF_STRUCT(NONCLIENTMETRICS,lfMessageFont);
		::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,ncm.cbSize,&ncm,0);
		switch (Type) {
		case FONT_MESSAGE:		plf=&ncm.lfMessageFont;		break;
		case FONT_MENU:			plf=&ncm.lfMenuFont;		break;
		case FONT_CAPTION:		plf=&ncm.lfCaptionFont;		break;
		case FONT_SMALLCAPTION:	plf=&ncm.lfSmCaptionFont;	break;
		case FONT_STATUS:		plf=&ncm.lfStatusFont;		break;
		default:
			return false;
		}
		*pLogFont=*plf;
	}
	return true;
}

CFont::CFont()
	: m_hfont(NULL)
{
}

CFont::CFont(const CFont &Font)
	: m_hfont(NULL)
{
	*this=Font;
}

CFont::CFont(const LOGFONT &Font)
	: m_hfont(NULL)
{
	Create(&Font);
}

CFont::CFont(FontType Type)
	: m_hfont(NULL)
{
	Create(Type);
}

CFont::~CFont()
{
	Destroy();
}

CFont &CFont::operator=(const CFont &Font)
{
	if (Font.m_hfont) {
		LOGFONT lf;
		Font.GetLogFont(&lf);
		Create(&lf);
	} else {
		if (m_hfont)
			::DeleteObject(m_hfont);
		m_hfont=NULL;
	}
	return *this;
}

bool CFont::operator==(const CFont &Font) const
{
	if (m_hfont==NULL)
		return Font.m_hfont==NULL;
	if (Font.m_hfont==NULL)
		return m_hfont==NULL;
	LOGFONT lf1,lf2;
	GetLogFont(&lf1);
	Font.GetLogFont(&lf2);
	return CompareLogFont(&lf1,&lf2);
}

bool CFont::operator!=(const CFont &Font) const
{
	return !(*this==Font);
}

bool CFont::Create(const LOGFONT *pLogFont)
{
	if (pLogFont==NULL)
		return false;
	HFONT hfont=::CreateFontIndirect(pLogFont);
	if (hfont==NULL)
		return false;
	if (m_hfont)
		::DeleteObject(m_hfont);
	m_hfont=hfont;
	return true;
}

bool CFont::Create(FontType Type)
{
	LOGFONT lf;

	if (!GetSystemFont(Type,&lf))
		return false;
	return Create(&lf);
}

void CFont::Destroy()
{
	if (m_hfont) {
		::DeleteObject(m_hfont);
		m_hfont=NULL;
	}
}

bool CFont::GetLogFont(LOGFONT *pLogFont) const
{
	if (m_hfont==NULL || pLogFont==NULL)
		return false;
	return ::GetObject(m_hfont,sizeof(LOGFONT),pLogFont)==sizeof(LOGFONT);
}

int CFont::GetHeight(bool fCell) const
{
	if (m_hfont==NULL)
		return 0;

	HDC hdc=::CreateCompatibleDC(NULL);
	int Height;
	if (hdc==NULL) {
		LOGFONT lf;
		if (!GetLogFont(&lf))
			return 0;
		Height=abs(lf.lfHeight);
	} else {
		Height=GetHeight(hdc,fCell);
		::DeleteDC(hdc);
	}
	return Height;
}

int CFont::GetHeight(HDC hdc,bool fCell) const
{
	if (m_hfont==NULL || hdc==NULL)
		return 0;
	HGDIOBJ hOldFont=::SelectObject(hdc,m_hfont);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc,&tm);
	::SelectObject(hdc,hOldFont);
	if (!fCell)
		tm.tmHeight-=tm.tmInternalLeading;
	return tm.tmHeight;
}

}	// namespace DrawUtil

#endif
