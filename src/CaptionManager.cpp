#include <Windows.h>
#include "Caption.h"
#include "Util.h"
#include "CaptionManager.h"

#define MSB(x) ((x) & 0x80000000)

CCaptionManager::CCaptionManager()
    : m_pCaptionDll(NULL)
    , m_dwIndex(0)
    , m_fProfileC(false)
    , m_fShowLang2(false)
    , m_fSuperimpose(false)
    , m_fEnPts(false)
    , m_pcr(0)
    , m_pts(0)
    , m_pCapList(NULL)
    , m_pDrcsList(NULL)
    , m_capCount(0)
    , m_drcsCount(0)
    , m_fEnLastTagInfoPcr(false)
    , m_lastTagInfoPcr(0)
    , m_queueFront(0)
    , m_queueRear(0)
{
    m_lang1.ucLangTag = 0xFF;
    m_lang2.ucLangTag = 0xFF;
}

void CCaptionManager::SetCaptionDll(const CCaptionDll *pCaptionDll, DWORD dwIndexToUse, bool fProfileC)
{
    m_pCaptionDll = pCaptionDll;
    m_dwIndex = dwIndexToUse;
    m_fProfileC = fProfileC;
    Clear();
}

void CCaptionManager::Clear()
{
    m_fSuperimpose = false;
    m_fEnPts = false;
    m_capCount = 0;
    m_drcsCount = 0;
    m_lang1.ucLangTag = 0xFF;
    m_lang2.ucLangTag = 0xFF;
    m_fEnLastTagInfoPcr = false;
    m_queueFront = m_queueRear;
    if (m_pCaptionDll) {
        m_pCaptionDll->Clear(m_dwIndex);
        if (m_fProfileC) {
            // 既定の字幕管理データを取得済みにする
            LANG_TAG_INFO_DLL *pLangList;
            if (m_pCaptionDll->GetTagInfo(m_dwIndex, &pLangList, NULL) == TRUE) {
                m_lang1 = pLangList[0];
            }
        }
    }
}

void CCaptionManager::AddPacket(LPCBYTE pPacket)
{
    ::memcpy(m_queue[m_queueRear], pPacket, 188);
    m_queueRear = (m_queueRear+1) % PACKET_QUEUE_SIZE;
}

// キューにある字幕ストリームから次の字幕文を取得する
// キューが空になるか字幕文を得る(m_capCount!=0)と返る
// PopCaption()やGetDrcsList()で返されたバッファは無効になる
void CCaptionManager::Analyze(DWORD currentPcr)
{
    if (!m_pCaptionDll) return;
    m_capCount = 0;
    m_drcsCount = 0;

    // "字幕管理データを3分以上未受信の場合は選局時の初期化動作を行う"
    if (m_fEnLastTagInfoPcr &&
        (MSB(currentPcr - m_lastTagInfoPcr) || currentPcr - m_lastTagInfoPcr >= 180000 * PCR_PER_MSEC))
    {
        DEBUG_OUT(TEXT(__FUNCTION__) TEXT("(): Clear\n"));
        Clear();
    }

    while (m_queueFront != m_queueRear) {
        BYTE *pPacket = m_queue[m_queueFront];
        m_queueFront = (m_queueFront+1) % PACKET_QUEUE_SIZE;

        TS_HEADER header;
        extract_ts_header(&header, pPacket);

        // 字幕PTSを取得
        if (header.payload_unit_start_indicator &&
            (header.adaptation_field_control&1)/*1,3*/)
        {
            BYTE *pPayload = pPacket + 4;
            if (header.adaptation_field_control == 3) {
                // アダプテーションに続けてペイロードがある
                ADAPTATION_FIELD adapt;
                extract_adaptation_field(&adapt, pPayload);
                pPayload = adapt.adaptation_field_length >= 0 ? pPayload + adapt.adaptation_field_length + 1 : NULL;
            }
            if (pPayload) {
                int payloadSize = 188 - static_cast<int>(pPayload - pPacket);
                PES_HEADER pesHeader;
                extract_pes_header(&pesHeader, pPayload, payloadSize);
                if (pesHeader.packet_start_code_prefix) {
                    if (pesHeader.stream_id == 0xBF) {
                        m_fSuperimpose = true;
                    }
                    else if (pesHeader.pts_dts_flags >= 2) {
                        m_pts = (DWORD)pesHeader.pts_45khz;
                        m_fEnPts = true;
                    }
                }
            }
        }

        DWORD ret = m_pCaptionDll->AddTSPacket(m_dwIndex, pPacket);
        unsigned char ucLangTag = m_fShowLang2 && m_lang2.ucLangTag!=0xFF ? 1 : 0;
        if ((m_fSuperimpose || m_fEnPts) && ret == (DWORD)(CP_NO_ERR_CAPTION_1+ucLangTag)) {
            // 字幕文データ
            if (m_pCaptionDll->GetCaptionData(m_dwIndex, ucLangTag, &m_pCapList, &m_capCount) != TRUE) {
                m_capCount = 0;
            }
            else {
                // DRCS図形データ(あれば)
                if (m_pCaptionDll->GetDRCSPattern(m_dwIndex, ucLangTag, &m_pDrcsList, &m_drcsCount) != TRUE) {
                    m_drcsCount = 0;
                }
                m_pcr = currentPcr;
                break;
            }
        }
        else if (ret == CP_CHANGE_VERSION) {
            // 字幕管理データ
            m_lang1.ucLangTag = 0xFF;
            m_lang2.ucLangTag = 0xFF;
            LANG_TAG_INFO_DLL *pLangList;
            DWORD langCount;
            if (m_pCaptionDll->GetTagInfo(m_dwIndex, &pLangList, &langCount) == TRUE) {
                if (langCount >= 1) m_lang1 = pLangList[0];
                if (langCount >= 2) m_lang2 = pLangList[1];
            }
            m_lastTagInfoPcr = currentPcr;
            m_fEnLastTagInfoPcr = true;
        }
        else if (ret == CP_NO_ERR_TAG_INFO) {
            m_lastTagInfoPcr = currentPcr;
            m_fEnLastTagInfoPcr = true;
        }
        else if (ret != TRUE && ret != CP_ERR_NEED_NEXT_PACKET &&
                 (ret < CP_NO_ERR_CAPTION_1 || CP_NO_ERR_CAPTION_8 < ret))
        {
            DEBUG_OUT(TEXT(__FUNCTION__) TEXT("(): Error packet!\n"));
        }
    }
}

// 表示タイミングに達した字幕本文を1つだけとり出す
const CAPTION_DATA_DLL *CCaptionManager::PopCaption(DWORD currentPcr, bool fIgnorePts)
{
    if (m_capCount != 0) {
        CAPTION_DATA_DLL *pCaption = m_pCapList;
        // 文字スーパーは非同期PESなので字幕文取得時のPCRが表示タイミングの基準になる
        if ((m_fSuperimpose || fIgnorePts) && MSB(m_pcr + pCaption->dwWaitTime * PCR_PER_MSEC - currentPcr) ||
            !m_fSuperimpose && !fIgnorePts && MSB(m_pts + pCaption->dwWaitTime * PCR_PER_MSEC - currentPcr))
        {
            ++m_pCapList;
            --m_capCount;
            return pCaption;
        }
    }
    return NULL;
}
