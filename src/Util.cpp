#include <Windows.h>
#include <Shlwapi.h>
#include <CommCtrl.h>
#include <ShlObj.h>
#include "Util.h"

// GetPrivateProfileSection()で取得したバッファから、キーに対応する文字列を取得する
void GetBufferedProfileString(LPCTSTR lpBuff, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize)
{
    int nKeyLen = ::lstrlen(lpKeyName);
    if (nKeyLen <= 126) {
        TCHAR szKey[128];
        ::lstrcpy(szKey, lpKeyName);
        ::lstrcpy(szKey + (nKeyLen++), TEXT("="));
        while (*lpBuff) {
            int nLen = ::lstrlen(lpBuff);
            if (!::StrCmpNI(lpBuff, szKey, nKeyLen)) {
                if ((lpBuff[nKeyLen] == TEXT('\'') || lpBuff[nKeyLen] == TEXT('"')) &&
                    nLen >= nKeyLen + 2 && lpBuff[nKeyLen] == lpBuff[nLen - 1])
                {
                    ::lstrcpyn(lpReturnedString, lpBuff + nKeyLen + 1, min(nLen-nKeyLen-1, static_cast<int>(nSize)));
                }
                else {
                    ::lstrcpyn(lpReturnedString, lpBuff + nKeyLen, nSize);
                }
                return;
            }
            lpBuff += nLen + 1;
        }
    }
    ::lstrcpyn(lpReturnedString, lpDefault, nSize);
}

// GetPrivateProfileSection()で取得したバッファから、キーに対応する数値を取得する
int GetBufferedProfileInt(LPCTSTR lpBuff, LPCTSTR lpKeyName, int nDefault)
{
    TCHAR szVal[32];
    GetBufferedProfileString(lpBuff, lpKeyName, TEXT(""), szVal, _countof(szVal));
    int nRet;
    return ::StrToIntEx(szVal, STIF_DEFAULT, &nRet) ? nRet : nDefault;
}

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
std::vector<WCHAR> ReadTextFileToEnd(LPCTSTR fileName, DWORD dwShareMode)
{
    std::vector<WCHAR> ret;
    HANDLE hFile = ::CreateFile(fileName, GENERIC_READ, dwShareMode, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return ret;

    WCHAR bom;
    DWORD readBytes;
    DWORD fileBytes = ::GetFileSize(hFile, NULL);
    if (fileBytes < sizeof(WCHAR) || READ_FILE_MAX_SIZE <= fileBytes ||
        !::ReadFile(hFile, &bom, sizeof(WCHAR), &readBytes, NULL) ||
        readBytes != sizeof(WCHAR) || bom != L'\xFEFF')
    {
        ::CloseHandle(hFile);
        return ret;
    }

    ret.resize((fileBytes - 1) / sizeof(WCHAR) + 1, L'\0');
    fileBytes -= sizeof(WCHAR);
    if (fileBytes > 0 && !::ReadFile(hFile, &ret.front(), fileBytes, &readBytes, NULL)) {
        ret.clear();
        ::CloseHandle(hFile);
        return ret;
    }

    ::CloseHandle(hFile);
    return ret;
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
    if (lpelfe->elfLogFont.lfFaceName[0] != TEXT('@')) {
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

// 参考: ITU-T H.222.0 Sec.2.4.4.3 および ARIB TR-B14 第一分冊第二編8.2
void extract_pat(PAT *pat, const unsigned char *payload, int payload_size, int unit_start, int counter)
{
    int program_number;
    int pos;
    size_t pmt_pos, i;
    int pid;
    const unsigned char *table;

    extract_psi(&pat->psi, payload, payload_size, unit_start, counter);

    if (pat->psi.version_number &&
        pat->psi.current_next_indicator &&
        pat->psi.table_id == 0 &&
        pat->psi.section_length >= 5)
    {
        // PAT更新
        table = pat->psi.data + 1 + pat->psi.pointer_field;
        pat->transport_stream_id = (table[3]<<8) | table[4];
        pat->version_number = pat->psi.version_number;

        // 受信済みPMTを調べ、必要ならば新規に生成する
        pmt_pos = 0;
        pos = 3 + 5;
        while (pos + 3 < 3 + pat->psi.section_length - 4/*CRC32*/) {
            program_number = (table[pos]<<8) | (table[pos+1]);
            if (program_number != 0) {
                pid = ((table[pos+2]&0x1f)<<8) | table[pos+3];
                for (i = pmt_pos; i < pat->pmt.size(); ++i) {
                    if (pat->pmt[i].pmt_pid == pid) {
                        if (i != pmt_pos) {
                            PMT sw = pat->pmt[i];
                            pat->pmt[i] = pat->pmt[pmt_pos];
                            pat->pmt[pmt_pos] = sw;
                        }
                        ++pmt_pos;
                        break;
                    }
                }
                if (i == pat->pmt.size()) {
                    pat->pmt.insert(pat->pmt.begin() + pmt_pos, PMT());
                    pat->pmt[pmt_pos++].pmt_pid = pid;
                }
            }
            pos += 4;
        }
        // PATから消えたPMTを破棄する
        pat->pmt.resize(pmt_pos);
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
                 stream_type == AVC_VIDEO ||
                 stream_type == H_265_VIDEO) &&
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

}	// namespace DrawUtil

#endif
