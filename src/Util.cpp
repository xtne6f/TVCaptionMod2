#include <Windows.h>
#include <Shlwapi.h>
#include <CommCtrl.h>
#include <ShlObj.h>
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

DWORD GetLongModuleFileName(HMODULE hModule, LPTSTR lpFileName, DWORD nSize)
{
    TCHAR longOrShortName[MAX_PATH];
    DWORD nRet = ::GetModuleFileName(hModule, longOrShortName, MAX_PATH);
    if (nRet && nRet < MAX_PATH) {
        nRet = ::GetLongPathName(longOrShortName, lpFileName, nSize);
        if (nRet < nSize) return nRet;
    }
    return 0;
}

// BOM付きUTF-16テキストファイルを文字列として全て読む
// 成功するとnewされた配列のポインタが返るので、必ずdeleteすること
WCHAR *NewReadTextFileToEnd(LPCTSTR fileName, DWORD dwShareMode)
{
    HANDLE hFile = ::CreateFile(fileName, GENERIC_READ, dwShareMode, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return NULL;

    WCHAR bom;
    DWORD readBytes;
    DWORD fileBytes = ::GetFileSize(hFile, NULL);
    if (fileBytes < sizeof(WCHAR) || READ_FILE_MAX_SIZE <= fileBytes ||
        !::ReadFile(hFile, &bom, sizeof(WCHAR), &readBytes, NULL) ||
        readBytes != sizeof(WCHAR) || bom != L'\xFEFF')
    {
        ::CloseHandle(hFile);
        return NULL;
    }

    WCHAR *pRet = new WCHAR[fileBytes / sizeof(WCHAR) - 1 + 1];
    if (!::ReadFile(hFile, pRet, fileBytes - sizeof(WCHAR), &readBytes, NULL)) {
        delete [] pRet;
        ::CloseHandle(hFile);
        return NULL;
    }
    pRet[readBytes / sizeof(WCHAR)] = 0;

    ::CloseHandle(hFile);
    return pRet;
}

int StrlenWoLoSurrogate(LPCTSTR str)
{
    int len = 0;
    for (; *str; ++str) {
        if ((*str & 0xFC00) != 0xDC00) ++len;
    }
    return len;
}

bool HexStringToByteArray(LPCTSTR str, BYTE *pDest, int destLen)
{
    int x, xx = 0;
    for (destLen *= 2; destLen > 0; --destLen, ++str) {
        if (TEXT('0') <= *str && *str <= TEXT('9'))
            x = *str - TEXT('0');
        else if (TEXT('A') <= *str && *str <= TEXT('F'))
            x = *str - TEXT('A') + 10;
        else if (TEXT('a') <= *str && *str <= TEXT('f'))
            x = *str - TEXT('a') + 10;
        else
            break;
        if (destLen & 1)
            *pDest++ = static_cast<BYTE>(xx | x);
        else
            xx = x << 4;
    }
    return destLen == 0;
}

void AddToComboBoxList(HWND hDlg, int id, const LPCTSTR *pList)
{
    for (; *pList; ++pList) {
        ::SendDlgItemMessage(hDlg, id, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(*pList));
    }
}

static int CALLBACK EnumAddFaceNameProc(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *, int FontType, LPARAM lParam)
{
    if (FontType == TRUETYPE_FONTTYPE && lpelfe->elfLogFont.lfFaceName[0] != TEXT('@')) {
        ::SendMessage(reinterpret_cast<HWND>(lParam), CB_ADDSTRING, 0,
                      reinterpret_cast<LPARAM>(lpelfe->elfLogFont.lfFaceName));
    }
    return TRUE;
}

void AddFaceNameToComboBoxList(HWND hDlg, int id)
{
    HDC hdc = ::GetDC(hDlg);
    LOGFONT lf;
    lf.lfCharSet = SHIFTJIS_CHARSET;
    lf.lfPitchAndFamily = 0;
    lf.lfFaceName[0] = 0;
    ::EnumFontFamiliesEx(hdc, &lf, reinterpret_cast<FONTENUMPROC>(EnumAddFaceNameProc),
                         reinterpret_cast<LPARAM>(::GetDlgItem(hDlg, id)), 0);
    ::ReleaseDC(hDlg, hdc);
}

static void extract_psi(PSI *psi, const unsigned char *payload, int payload_size, int unit_start, int counter)
{
    int pointer;
    int section_length;
    const unsigned char *table;

    if (unit_start) {
        psi->continuity_counter = 0x20|counter;
        psi->data_count = psi->version_number = 0;
    }
    else {
        psi->continuity_counter = (psi->continuity_counter+1)&0x2f;
        if (psi->continuity_counter != (0x20|counter)) {
            psi->continuity_counter = psi->data_count = psi->version_number = 0;
            return;
        }
    }
    if (psi->data_count + payload_size <= sizeof(psi->data)) {
        memcpy(psi->data + psi->data_count, payload, payload_size);
        psi->data_count += payload_size;
    }
    // TODO: CRC32

    // psi->version_number != 0 のとき各フィールドは有効
    if (psi->data_count >= 1) {
        pointer = psi->data[0];
        if (psi->data_count >= pointer + 4) {
            section_length = ((psi->data[pointer+2]&0x03)<<8) | psi->data[pointer+3];
            if (section_length >= 3 && psi->data_count >= pointer + 4 + section_length) {
                table = psi->data + 1 + pointer;
                psi->pointer_field          = pointer;
                psi->table_id               = table[0];
                psi->section_length         = section_length;
                psi->version_number         = 0x20 | ((table[5]>>1)&0x1f);
                psi->current_next_indicator = table[5] & 0x01;
            }
        }
    }
}

void reset_pat(PAT *pat)
{
    while (pat->pid_count > 0) {
        delete pat->pmt[--pat->pid_count];
    }
    ::memset(pat, 0, sizeof(PAT));
}

// 参考: ITU-T H.222.0 Sec.2.4.4.3 および ARIB TR-B14 第一分冊第二編8.2
void extract_pat(PAT *pat, const unsigned char *payload, int payload_size, int unit_start, int counter)
{
    int program_number;
    int found;
    int pos, i, j, n;
    int pmt_exists[PAT_PID_MAX];
    unsigned short pid;
    const unsigned char *table;

    extract_psi(&pat->psi, payload, payload_size, unit_start, counter);

    if (pat->psi.version_number &&
        pat->psi.version_number != pat->version_number &&
        pat->psi.current_next_indicator &&
        pat->psi.table_id == 0 &&
        pat->psi.section_length >= 5)
    {
        // PAT更新
        table = pat->psi.data + 1 + pat->psi.pointer_field;
        pat->transport_stream_id = (table[3]<<8) | table[4];
        pat->version_number = pat->psi.version_number;

        // 受信済みPMTを調べ、必要ならば新規に生成する
        memset(pmt_exists, 0, sizeof(pmt_exists));
        pos = 3 + 5;
        while (pos + 3 < 3 + pat->psi.section_length - 4/*CRC32*/) {
            program_number = (table[pos]<<8) | (table[pos+1]);
            if (program_number != 0) {
                pid = ((table[pos+2]&0x1f)<<8) | table[pos+3];
                for (found=0, i=0; i < pat->pid_count; ++i) {
                    if (pat->pid[i] == pid) {
                        pmt_exists[i] = found = 1;
                        break;
                    }
                }
                if (!found && pat->pid_count < PAT_PID_MAX) {
                    //pat->program_number[pat->pid_count] = program_number;
                    pat->pid[pat->pid_count] = pid;
                    pat->pmt[pat->pid_count] = new PMT;
                    memset(pat->pmt[pat->pid_count], 0, sizeof(PMT));
                    pmt_exists[pat->pid_count++] = 1;
                }
            }
            pos += 4;
        }
        // PATから消えたPMTを破棄する
        n = pat->pid_count;
        for (i=0, j=0; i < n; ++i) {
            if (!pmt_exists[i]) {
                delete pat->pmt[i];
                --pat->pid_count;
            }
            else {
                pat->pid[j] = pat->pid[i];
                pat->pmt[j++] = pat->pmt[i];
            }
        }
    }
}

// 参考: ITU-T H.222.0 Sec.2.4.4.8 および ARIB TR-B14 第二分冊第四編第3部
void extract_pmt(PMT *pmt, const unsigned char *payload, int payload_size, int unit_start, int counter)
{
    int program_info_length;
    int es_info_length;
    int stream_type;
    int info_pos;
    int pos;
    const unsigned char *table;

    extract_psi(&pmt->psi, payload, payload_size, unit_start, counter);

    if (pmt->psi.version_number &&
        pmt->psi.version_number != pmt->version_number &&
        pmt->psi.current_next_indicator &&
        pmt->psi.table_id == 2 &&
        pmt->psi.section_length >= 9)
    {
        // PMT更新
        table = pmt->psi.data + 1 + pmt->psi.pointer_field;
        pmt->program_number = (table[3]<<8) | table[4];
        pmt->version_number = pmt->psi.version_number;
        pmt->pcr_pid        = ((table[8]&0x1f)<<8) | table[9];
        program_info_length = ((table[10]&0x03)<<8) | table[11];

        pmt->pid_count = 0;
        pos = 3 + 9 + program_info_length;
        while (pos + 4 < 3 + pmt->psi.section_length - 4/*CRC32*/) {
            stream_type = table[pos];
            es_info_length = (table[pos+3]&0x03)<<8 | table[pos+4];
            if ((stream_type == H_262_VIDEO ||
                 stream_type == PES_PRIVATE_DATA ||
                 stream_type == AVC_VIDEO) &&
                pos + 5 + es_info_length <= 3 + pmt->psi.section_length - 4/*CRC32*/)
            {
                // ストリーム識別記述子を探す(運用規定と異なり必ずしも先頭に配置されない)
                for (info_pos = 0; info_pos + 2 < es_info_length; ) {
                    if (table[pos+5+info_pos] == 0x52) {
                        pmt->stream_type[pmt->pid_count] = (unsigned char)stream_type;
                        pmt->pid[pmt->pid_count] = (table[pos+1]&0x1f)<<8 | table[pos+2];
                        pmt->component_tag[pmt->pid_count++] = table[pos+5+info_pos+2];
                        break;
                    }
                    info_pos += 2 + table[pos+5+info_pos+1];
                }
            }
            pos += 5 + es_info_length;
        }
    }
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

int CALLBACK BrowseFolderCallback(HWND hwnd,UINT uMsg,LPARAM lpData,LPARAM lParam)
{
	switch (uMsg) {
	case BFFM_INITIALIZED:
		if (((LPTSTR)lParam)[0]!=TEXT('\0')) {
			TCHAR szDirectory[MAX_PATH];

			lstrcpy(szDirectory,(LPTSTR)lParam);
			PathRemoveBackslash(szDirectory);
			SendMessage(hwnd,BFFM_SETSELECTION,TRUE,(LPARAM)szDirectory);
		}
		break;
	}
	return 0;
}

bool BrowseFolderDialog(HWND hwndOwner,LPTSTR pszDirectory,LPCTSTR pszTitle)
{
	BROWSEINFO bi;
	PIDLIST_ABSOLUTE pidl;
	BOOL fRet;

	bi.hwndOwner=hwndOwner;
	bi.pidlRoot=NULL;
	bi.pszDisplayName=pszDirectory;
	bi.lpszTitle=pszTitle;
	bi.ulFlags=BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.lpfn=BrowseFolderCallback;
	bi.lParam=(LPARAM)pszDirectory;
	pidl=SHBrowseForFolder(&bi);
	if (pidl==NULL)
		return false;
	fRet=SHGetPathFromIDList(pidl,pszDirectory);
	CoTaskMemFree(pidl);
	return fRet==TRUE;
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
