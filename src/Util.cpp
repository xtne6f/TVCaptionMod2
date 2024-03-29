﻿#include <Windows.h>
#include <CommCtrl.h>
#include <ShlObj.h>
#include "Util.h"

// 必要なバッファを確保してGetPrivateProfileSection()を呼ぶ
std::vector<TCHAR> GetPrivateProfileSectionBuffer(LPCTSTR lpAppName, LPCTSTR lpFileName)
{
    std::vector<TCHAR> buf(4096);
    for (;;) {
        DWORD len = GetPrivateProfileSection(lpAppName, buf.data(), static_cast<DWORD>(buf.size()), lpFileName);
        if (len < buf.size() - 2) {
            buf.resize(len + 1);
            break;
        }
        if (buf.size() >= READ_FILE_MAX_SIZE / 2) {
            buf.assign(1, TEXT('\0'));
            break;
        }
        buf.resize(buf.size() * 2);
    }
    return buf;
}

// GetPrivateProfileSection()で取得したバッファから、キーに対応する文字列を取得する
void GetBufferedProfileString(LPCTSTR lpBuff, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize)
{
    size_t nKeyLen = _tcslen(lpKeyName);
    while (*lpBuff) {
        size_t nLen = _tcslen(lpBuff);
        if (!_tcsnicmp(lpBuff, lpKeyName, nKeyLen) && lpBuff[nKeyLen] == TEXT('=')) {
            if ((lpBuff[nKeyLen + 1] == TEXT('\'') || lpBuff[nKeyLen + 1] == TEXT('"')) &&
                nLen >= nKeyLen + 3 && lpBuff[nKeyLen + 1] == lpBuff[nLen - 1])
            {
                _tcsncpy_s(lpReturnedString, nSize, lpBuff + nKeyLen + 2, min(nLen - nKeyLen - 3, static_cast<size_t>(nSize - 1)));
            }
            else {
                _tcsncpy_s(lpReturnedString, nSize, lpBuff + nKeyLen + 1, _TRUNCATE);
            }
            return;
        }
        lpBuff += nLen + 1;
    }
    _tcsncpy_s(lpReturnedString, nSize, lpDefault, _TRUNCATE);
}

// GetPrivateProfileSection()で取得したバッファから、キーに対応する数値を取得する
int GetBufferedProfileInt(LPCTSTR lpBuff, LPCTSTR lpKeyName, int nDefault)
{
    TCHAR sz[16];
    GetBufferedProfileString(lpBuff, lpKeyName, TEXT(""), sz, _countof(sz));
    LPTSTR endp;
    int nRet = _tcstol(sz, &endp, 10);
    return endp == sz ? nDefault : nRet;
}

BOOL WritePrivateProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, int value, LPCTSTR lpFileName)
{
    TCHAR sz[16];
    _stprintf_s(sz, TEXT("%d"), value);
    return ::WritePrivateProfileString(lpAppName, lpKeyName, sz, lpFileName);
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

bool IsWindows7OrLater()
{
    OSVERSIONINFOEX vi;
    vi.dwOSVersionInfoSize = sizeof(vi);
    vi.dwMajorVersion = 6;
    vi.dwMinorVersion = 1;
    return !!::VerifyVersionInfo(&vi, VER_MAJORVERSION | VER_MINORVERSION,
                                 ::VerSetConditionMask(::VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL),
                                                       VER_MINORVERSION, VER_GREATER_EQUAL));
}

// BOM付きUTF-16テキストファイルを文字列として全て読む
std::vector<WCHAR> ReadTextFileToEnd(LPCTSTR fileName, DWORD dwShareMode)
{
    std::vector<WCHAR> ret;
    HANDLE hFile = ::CreateFile(fileName, GENERIC_READ, dwShareMode, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return ret;

    WCHAR bom;
    DWORD readBytes;
    DWORD fileBytes = ::GetFileSize(hFile, nullptr);
    if (fileBytes < sizeof(WCHAR) || READ_FILE_MAX_SIZE <= fileBytes ||
        !::ReadFile(hFile, &bom, sizeof(WCHAR), &readBytes, nullptr) ||
        readBytes != sizeof(WCHAR) || bom != L'\xFEFF')
    {
        ::CloseHandle(hFile);
        return ret;
    }

    ret.resize((fileBytes - 1) / sizeof(WCHAR) + 1, L'\0');
    fileBytes -= sizeof(WCHAR);
    if (fileBytes > 0 && !::ReadFile(hFile, ret.data(), fileBytes, &readBytes, nullptr)) {
        ret.clear();
        ::CloseHandle(hFile);
        return ret;
    }

    ::CloseHandle(hFile);
    return ret;
}

bool IsNonSpacingCharacter(TCHAR c)
{
    // 「´｀¨＾￣＿◯」の結合文字
    return c == 0x0301 || c == 0x0300 || c == 0x0308 || c == 0x0302 ||
           c == 0x0305 || c == 0x0332 || c == 0x20DD;
}

int StrlenWoLoSurrogateOrNonSpacing(LPCTSTR str)
{
    int len = 0;
    for (; *str; ++str) {
        if ((*str & 0xFC00) != 0xDC00 && !IsNonSpacingCharacter(*str)) ++len;
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
    static_cast<void>(FontType);
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
                pmt->stream_type[pmt->pid_count] = (unsigned char)stream_type;
                pmt->pid[pmt->pid_count] = (table[pos+1]&0x1f)<<8 | table[pos+2];
                // ストリーム識別記述子を探す(運用規定と異なり必ずしも先頭に配置されない)
                // 映像系ストリームの情報は再生中の映像PIDとのマッチングに使うだけなので、component_tagの情報は必須でない
                pmt->component_tag[pmt->pid_count] = 0xff;
                for (info_pos = 0; info_pos + 2 < es_info_length; ) {
                    if (table[pos+5+info_pos] == 0x52) {
                        pmt->component_tag[pmt->pid_count] = table[pos+5+info_pos+2];
                        break;
                    }
                    info_pos += 2 + table[pos+5+info_pos+1];
                }
                ++pmt->pid_count;
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

bool CompareLogFont(const LOGFONT &lf1, const LOGFONT &lf2)
{
    return lf1.lfHeight == lf2.lfHeight &&
           lf1.lfWidth == lf2.lfWidth &&
           lf1.lfEscapement == lf2.lfEscapement &&
           lf1.lfOrientation == lf2.lfOrientation &&
           lf1.lfWeight == lf2.lfWeight &&
           lf1.lfItalic == lf2.lfItalic &&
           lf1.lfUnderline == lf2.lfUnderline &&
           lf1.lfStrikeOut == lf2.lfStrikeOut &&
           lf1.lfCharSet == lf2.lfCharSet &&
           lf1.lfOutPrecision == lf2.lfOutPrecision &&
           lf1.lfClipPrecision == lf2.lfClipPrecision &&
           lf1.lfQuality == lf2.lfQuality &&
           lf1.lfPitchAndFamily == lf2.lfPitchAndFamily &&
           !_tcscmp(lf1.lfFaceName, lf2.lfFaceName);
}

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    static_cast<void>(lParam);
    if (uMsg == BFFM_INITIALIZED && reinterpret_cast<LPCTSTR>(lpData)[0]) {
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
    }
    return 0;
}

bool BrowseFolderDialog(HWND hwndOwner, TCHAR (&szDirectory)[MAX_PATH], LPCTSTR pszTitle)
{
    TCHAR szDisplayName[MAX_PATH];
    BROWSEINFO bi;
    bi.hwndOwner = hwndOwner;
    bi.pidlRoot = nullptr;
    bi.pszDisplayName = szDisplayName;
    bi.lpszTitle = pszTitle;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpfn = BrowseCallbackProc;
    bi.lParam = reinterpret_cast<LPARAM>(szDirectory);
    PIDLIST_ABSOLUTE pidl = SHBrowseForFolder(&bi);
    if (pidl) {
        if (SHGetPathFromIDList(pidl, szDirectory)) {
            CoTaskMemFree(pidl);
            return true;
        }
        CoTaskMemFree(pidl);
    }
    return false;
}

bool SaveImageAsBmp(LPCTSTR fileName, const BITMAPINFOHEADER &bih, const void *pBits)
{
    if (bih.biBitCount == 24) {
        HANDLE hFile = ::CreateFile(fileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile != INVALID_HANDLE_VALUE) {
            BITMAPFILEHEADER bmfHeader = {};
            bmfHeader.bfType = 0x4D42;
            bmfHeader.bfOffBits = sizeof(bmfHeader) + sizeof(bih);
            bmfHeader.bfSize = bmfHeader.bfOffBits + (bih.biWidth * 3 + 3) / 4 * 4 * bih.biHeight;
            DWORD dwWritten;
            ::WriteFile(hFile, &bmfHeader, sizeof(bmfHeader), &dwWritten, nullptr);
            ::WriteFile(hFile, &bih, sizeof(bih), &dwWritten, nullptr);
            ::WriteFile(hFile, pBits, bmfHeader.bfSize - bmfHeader.bfOffBits, &dwWritten, nullptr);
            ::CloseHandle(hFile);
            return true;
        }
    }
    return false;
}

struct ImageSaveInfo
{
    LPCTSTR pszFileName;
    LPCTSTR pszFormat;
    LPCTSTR pszOption;
    const BITMAPINFO *pbmi;
    const void *pBits;
    LPCTSTR pszComment;
};

bool SaveImageAsPngOrJpeg(HMODULE hTVTestImage, LPCTSTR fileName, bool pngOrJpeg, int compressionLevelOrQuality, const BITMAPINFOHEADER &bih, const void *pBits)
{
    BOOL (WINAPI *pfnSaveImage)(const ImageSaveInfo *) =
        reinterpret_cast<BOOL (WINAPI *)(const ImageSaveInfo *)>(::GetProcAddress(hTVTestImage, "SaveImage"));
    if (pfnSaveImage) {
        ImageSaveInfo info;
        info.pszFileName = fileName;
        info.pszFormat = pngOrJpeg ? TEXT("PNG") : TEXT("JPEG");
        TCHAR option[16];
        _stprintf_s(option, TEXT("%d"), min(max(compressionLevelOrQuality, 0), pngOrJpeg ? 9 : 100));
        info.pszOption = option;
        BITMAPINFO bmi = {};
        bmi.bmiHeader = bih;
        info.pbmi = &bmi;
        info.pBits = pBits;
        info.pszComment = nullptr;
        return !!pfnSaveImage(&info);
    }
    return false;
}

HBITMAP LoadAribPngAsDIBSection(HMODULE hTVTestImage, const BYTE *pPngData, size_t dataSize, void **ppBits, RECT *pCropRect)
{
    HBITMAP hbm = nullptr;
    HGLOBAL (WINAPI *pfnLoadAribPngFromMemory)(const void *, SIZE_T) =
        reinterpret_cast<HGLOBAL (WINAPI *)(const void *, SIZE_T)>(::GetProcAddress(hTVTestImage, "LoadAribPngFromMemory"));
    if (pfnLoadAribPngFromMemory) {
        HGLOBAL hDIB = pfnLoadAribPngFromMemory(pPngData, dataSize);
        if (hDIB) {
            // 中身は常に32ビットDIB
            BITMAPINFO *pbmi = static_cast<BITMAPINFO *>(::GlobalLock(hDIB));
            if (pbmi) {
                BITMAPINFOHEADER bmih = pbmi->bmiHeader;
                if (pCropRect) {
                    bmih.biWidth = min(max(bmih.biWidth - pCropRect->left, 0), pCropRect->right - pCropRect->left);
                    bmih.biHeight = min(max(bmih.biHeight - pCropRect->top, 0), pCropRect->bottom - pCropRect->top);
                }
                if (bmih.biBitCount == 32 && bmih.biWidth > 0 && bmih.biHeight > 0) {
                    hbm = ::CreateDIBSection(nullptr, reinterpret_cast<BITMAPINFO *>(&bmih), DIB_RGB_COLORS, ppBits, nullptr, 0);
                    if (hbm) {
                        DWORD *pdwBits = static_cast<DWORD *>(*ppBits);
                        DWORD *pdwSrcBits = reinterpret_cast<DWORD *>(pbmi->bmiColors);
                        if (bmih.biWidth == pbmi->bmiHeader.biWidth && bmih.biHeight == pbmi->bmiHeader.biHeight) {
                            ::memcpy(pdwBits, pdwSrcBits, bmih.biWidth * bmih.biHeight * 4);
                        }
                        else {
                            pdwSrcBits += pbmi->bmiHeader.biWidth * (pbmi->bmiHeader.biHeight - bmih.biHeight - pCropRect->top) + pCropRect->left;
                            for (int y = 0; y < bmih.biHeight; ++y) {
                                ::memcpy(pdwBits, pdwSrcBits, bmih.biWidth * 4);
                                pdwBits += bmih.biWidth;
                                pdwSrcBits += pbmi->bmiHeader.biWidth;
                            }
                        }
                    }
                }
                ::GlobalUnlock(hDIB);
            }
            ::GlobalFree(hDIB);
        }
    }
    return hbm;
}

HBITMAP CopyDIBSectionWithTransparency(HBITMAP hbmSrc, const CLUT_DAT_DLL *pTransparentColorList, size_t colorLen, void **ppBits)
{
    // 32ビットDIBであること
    BITMAP bm;
    if (::GetObject(hbmSrc, sizeof(BITMAP), &bm) && bm.bmBitsPixel == 32 && bm.bmWidthBytes % 4 == 0) {
        BITMAPINFOHEADER bmih = {};
        bmih.biSize = sizeof(BITMAPINFOHEADER);
        bmih.biWidth = bm.bmWidth;
        bmih.biHeight = bm.bmHeight;
        bmih.biPlanes = 1;
        bmih.biBitCount = 32;
        bmih.biCompression = BI_RGB;
        HBITMAP hbm = ::CreateDIBSection(nullptr, reinterpret_cast<BITMAPINFO *>(&bmih), DIB_RGB_COLORS, ppBits, nullptr, 0);
        if (hbm) {
            DWORD *pdwBits = static_cast<DWORD *>(*ppBits);
            DWORD *pdwSrcBits = static_cast<DWORD *>(bm.bmBits);
            for (int y = 0; y < bmih.biHeight; ++y) {
                for (int x = 0; x < bmih.biWidth; ++x) {
                    DWORD c = pdwSrcBits[x];
                    for (size_t i = 0; i < colorLen; ++i) {
                        CLUT_DAT_DLL t = pTransparentColorList[i];
                        if (t.ucB == (c & 0xFF) && t.ucG == (c >> 8 & 0xFF) && t.ucR == (c >> 16 & 0xFF) && t.ucAlpha == (c >> 24)) {
                            c = 0;
                            break;
                        }
                    }
                    pdwBits[x] = c;
                }
                pdwBits += bmih.biWidth;
                pdwSrcBits += bm.bmWidthBytes / 4;
            }
            return hbm;
        }
    }
    return nullptr;
}

bool StretchDrawBitmap(HDC hdc, int x, int y, int width, int height, HBITMAP hbm, int stretchMode, int halfSizeStretchMode)
{
    BITMAP bm;
    if (::GetObject(hbm, sizeof(BITMAP), &bm)) {
        HDC hdcSrc = ::CreateCompatibleDC(hdc);
        if (hdcSrc) {
            HBITMAP hbmOld = static_cast<HBITMAP>(::SelectObject(hdcSrc, hbm));
            int oldStretchMode = ::SetStretchBltMode(hdc,
                bm.bmWidth == width && bm.bmHeight == height ? STRETCH_DELETESCANS :
                (bm.bmWidth == width || bm.bmWidth == width * 2) &&
                (bm.bmHeight == height || bm.bmHeight == height * 2) && halfSizeStretchMode ? halfSizeStretchMode : stretchMode);
            bool fRet = !!::StretchBlt(hdc, x, y, width, height, hdcSrc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
            ::SetStretchBltMode(hdc, oldStretchMode);
            ::SelectObject(hdcSrc, hbmOld);
            ::DeleteDC(hdcSrc);
            return fRet;
        }
    }
    return false;
}

#if 1 // From: TVTest_0.7.23_Src/DrawUtil.cpp

namespace DrawUtil {

// 単色で塗りつぶす
bool Fill(HDC hdc,const RECT *pRect,COLORREF Color)
{
	HBRUSH hbr=::CreateSolidBrush(Color);

	if (!hbr)
		return false;
	::FillRect(hdc,pRect,hbr);
	::DeleteObject(hbr);
	return true;
}

}	// namespace DrawUtil

#endif
