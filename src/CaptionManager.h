#ifndef INCLUDE_CAPTION_MANAGER_H
#define INCLUDE_CAPTION_MANAGER_H

class CCaptionManager
{
    static const int PACKET_QUEUE_SIZE = 1024;
public:
    CCaptionManager();
    void SetCaptionDll(const CCaptionDll *pCaptionDll, DWORD dwIndexToUse, bool fProfileC=false);
    void Clear();
    void AddPacket(LPCBYTE pPacket);
    bool Analyze(DWORD currentPcr);
    const CAPTION_DATA_DLL *PopCaption(DWORD currentPcr, bool fIgnorePts=false);
    bool IsSuperimpose() const { return m_fSuperimpose; }
    bool IsEmpty() const { return m_capCount == 0; }
    void GetDrcsList(const DRCS_PATTERN_DLL **ppList, DWORD *pCount) const { *ppList=m_pDrcsList; *pCount=m_drcsCount; }
    bool SwitchLang() { return m_fShowLang2 = !m_fShowLang2; }
    const LANG_TAG_INFO_DLL *GetLangTag() const {
        return m_fShowLang2 && m_lang2.ucLangTag!=0xFF ? &m_lang2 : m_lang1.ucLangTag!=0xFF ? &m_lang1 : NULL;
    }
private:
    const CCaptionDll *m_pCaptionDll;
    DWORD m_dwIndex;
    bool m_fProfileC;
    bool m_fShowLang2;
    bool m_fSuperimpose;
    bool m_fEnPts;
    DWORD m_pcr;
    DWORD m_pts;
    CAPTION_DATA_DLL *m_pCapList;
    DRCS_PATTERN_DLL *m_pDrcsList;
    DWORD m_capCount;
    DWORD m_drcsCount;
    LANG_TAG_INFO_DLL m_lang1, m_lang2;
    BYTE m_queue[PACKET_QUEUE_SIZE][188];
    int m_queueFront;
    int m_queueRear;
};

#endif // INCLUDE_CAPTION_MANAGER_H
