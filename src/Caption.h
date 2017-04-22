#ifndef INCLUDE_CAPTION_H
#define INCLUDE_CAPTION_H

#include "../Caption_src/CaptionDef.h"
#include "../Caption_src/Caption.h"

inline bool operator==(const CLUT_DAT_DLL &a, const CLUT_DAT_DLL &b) {
    return a.ucR == b.ucR && a.ucG == b.ucG && a.ucB == b.ucB && a.ucAlpha == b.ucAlpha;
}

BOOL CalcMD5FromDRCSPattern(BYTE *pbHash, const DRCS_PATTERN_DLL *pPattern);

// 2ストリーム(字幕+文字スーパーなど)をまとめて扱うラッパークラス
// 昔の名残りでこんな名前だが実装はすでにDLLではない

class CCaptionDll {
public:
    CCaptionDll() : m_fInitialized(false) {}
    bool IsInitialized() const { return m_fInitialized; }
    bool Initialize() {
        if (m_fInitialized) return true;
        m_fInitialized = true;
        if (SwitchStream(1) && InitializeCPW() == TRUE &&
            SwitchStream(0) && InitializeCPW() == TRUE)
        {
            return true;
        }
        m_fInitialized = false;
        return false;
    }
    void UnInitialize() {
        if (m_fInitialized) {
            if (SwitchStream(1)) UnInitializeCP();
            if (SwitchStream(0)) UnInitializeCP();
            m_fInitialized = false;
        }
    }
    DWORD AddTSPacket(DWORD dwIndex, BYTE *pbPacket) const {
        return SwitchStream(dwIndex) ? AddTSPacketCP(pbPacket) : FALSE;
    }
    DWORD Clear(DWORD dwIndex) const {
        return SwitchStream(dwIndex) ? ClearCP() : FALSE;
    }
    DWORD GetTagInfo(DWORD dwIndex, LANG_TAG_INFO_DLL** ppList, DWORD* pdwListCount) const {
        return SwitchStream(dwIndex) ? GetTagInfoCP(ppList, pdwListCount) : FALSE;
    }
    DWORD GetCaptionData(DWORD dwIndex, unsigned char ucLangTag, CAPTION_DATA_DLL** ppList, DWORD* pdwListCount) const {
        return SwitchStream(dwIndex) ? GetCaptionDataCPW(ucLangTag, ppList, pdwListCount) : FALSE;
    }
    DWORD GetDRCSPattern(DWORD dwIndex, unsigned char ucLangTag, DRCS_PATTERN_DLL** ppList, DWORD* pdwListCount) const {
        return SwitchStream(dwIndex) ? GetDRCSPatternCP(ucLangTag, ppList, pdwListCount) : FALSE;
    }
    bool SetGaiji(DWORD dwCommand, const WCHAR* ppTable, DWORD* pdwTableSize) const {
        DWORD dwTableSize = *pdwTableSize;
        return SwitchStream(1) && SetGaijiCP(dwCommand, ppTable, &dwTableSize) == TRUE &&
               SwitchStream(0) && SetGaijiCP(dwCommand, ppTable, pdwTableSize) == TRUE;
    }
    bool GetGaiji(DWORD dwCommand, WCHAR* ppTable, DWORD* pdwTableSize) const {
        return SwitchStream(0) && GetGaijiCP(dwCommand, ppTable, pdwTableSize) == TRUE;
    }
private:
    bool SwitchStream(DWORD dwIndex) const {
        return m_fInitialized && dwIndex < 2 && SwitchStreamCP(dwIndex) == TRUE;
    }
    bool m_fInitialized;
};

#endif // INCLUDE_CAPTION_H
