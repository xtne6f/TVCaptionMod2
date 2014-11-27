#ifndef INCLUDE_CAPTION_H
#define INCLUDE_CAPTION_H

#include "../Caption/CaptionDef.h"
#include "../Caption/Caption.h"

// 名前衝突的に危ういので接頭辞CP_を付けさせてもらう

#define CP_ERR_INIT             10
#define CP_ERR_NOT_INIT         11

#define CP_ERR_NEED_NEXT_PACKET 13      // 次のTSパケット入れないと解析できない
#define CP_ERR_CAN_NOT_ANALYZ   14      // 本当にTSパケット？解析不可能
#define CP_ERR_NOT_FIRST        15      // 最初のTSパケット未入力
#define CP_ERR_INVALID_PACKET   16      // 本当にTSパケット？パケット飛んで壊れてるかも

#define CP_CHANGE_VERSION       20
#define CP_NO_ERR_TAG_INFO      21
#define CP_NO_ERR_CAPTION_1     22
#define CP_NO_ERR_CAPTION_8     29

#define G_CELL_SIZE 94

typedef enum {
    CP_STR_SMALL = 0,   // SSZ
    CP_STR_MEDIUM,      // MSZ
    CP_STR_NORMAL,      // NSZ
    CP_STR_MICRO,       // SZX 0x60
    CP_STR_HIGH_W,      // SZX 0x41
    CP_STR_WIDTH_W,     // SZX 0x44
    CP_STR_W,           // SZX 0x45
    CP_STR_SPECIAL_1,   // SZX 0x6B
    CP_STR_SPECIAL_2,   // SZX 0x64
} CP_STRING_SIZE;

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
