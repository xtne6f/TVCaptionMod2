#define NOMINMAX
#include <windows.h>
#include "ViewerClockEstimator.h"
#include <algorithm>
#include <iterator>

CViewerClockEstimator::CViewerClockEstimator()
    : m_fEnabled(true)
    , m_streamPcr(-1)
    , m_updateDiffPcr(-1)
    , m_pcrDiff(DEFAULT_PCR_DIFF)
{
    for (int i = 0; i < 2; ++i) {
        m_pesHeaderRemain[i] = -1;
        m_unitState[i] = 0;
        m_unitSize[i] = 0;
        m_crc[i] = 0;
    }
    for (DWORD i = 0; i < 256; ++i) {
        m_crcTable[i] = i << 24;
        for (int j = 0; j < 8; ++j) {
            m_crcTable[i] = (m_crcTable[i] << 1) ^ ((m_crcTable[i] & 0x80000000) ? 0x04c11db7 : 0);
        }
    }
}

void CViewerClockEstimator::SetEnabled(bool fEnabled)
{
    lock_recursive_mutex lock(m_streamLock);

    if (m_fEnabled != fEnabled) {
        Reset();
        m_fEnabled = fEnabled;
    }
}

bool CViewerClockEstimator::GetEnabled() const
{
    lock_recursive_mutex lock(m_streamLock);

    return m_fEnabled;
}

void CViewerClockEstimator::SetStreamPcr(LONGLONG pcr)
{
    lock_recursive_mutex lock(m_streamLock);

    m_streamPcr = pcr;

    if (((0x200000000 + m_streamPcr - m_updateDiffPcr) & 0x1ffffffff) > VIEWER_PCR_QUEUE_DIFF) {
        m_updateDiffPcr = m_streamPcr;

        for (int i = 0; i < 2; ++i) {
            while (!m_crcPcrDeq[i].empty()) {
                DWORD pcrDiff = static_cast<DWORD>(m_streamPcr) - m_crcPcrDeq[i].front().second;
                if (pcrDiff < (i ? VIEWER_PCR_QUEUE_DIFF : STREAM_PCR_QUEUE_DIFF)) {
                    break;
                }
                m_crcPcrDeq[i].pop_front();
            }
        }

        // 2地点で得たストリームの各ユニットについて、CRC32値の一致するものの時差を計算する
        m_pcrDiffs.clear();
        int entireCount = 0;
        if (!m_crcPcrDeq[0].empty() && !m_crcPcrDeq[1].empty()) {
            auto itStream = m_crcPcrDeq[0].rbegin() + 1;
            auto itStreamEnd = m_crcPcrDeq[0].rend();
            auto itViewerEnd = m_crcPcrDeq[1].rend();
            for (auto itViewer = m_crcPcrDeq[1].rbegin() + 1; itViewer != itViewerEnd; ++itViewer) {
                DWORD crc = itViewer->first;
                for (; itStream != itStreamEnd && crc != itStream->first; ++itStream);
                if (itStream == itStreamEnd) {
                    // 下流で見つかったものが上流にないことはあまりないので比較的まれ
                    // 計算量を制限するため1/100だけスキップする
                    itViewer += std::min<size_t>(m_crcPcrDeq[1].size() / 100, std::distance(itViewer, itViewerEnd) - 1);
                    itStream = m_crcPcrDeq[0].rbegin() + 1;
                }
                else {
                    // 一致した
                    DWORD pcrDiff = itViewer->second - itStream->second;
                    if (pcrDiff < STREAM_PCR_QUEUE_DIFF) {
                        auto it = std::lower_bound(m_pcrDiffs.begin(), m_pcrDiffs.end(), std::make_pair(pcrDiff, 0));
                        if (it == m_pcrDiffs.end() || it->first != pcrDiff) {
                            it = m_pcrDiffs.insert(it, std::make_pair(pcrDiff, 0));
                        }
                        it->second++;
                        ++entireCount;
                    }
                    ++itStream;
                }
            }
        }

        // 中央値を使う
        int medianCount = 0;
        for (auto it = m_pcrDiffs.begin(); it != m_pcrDiffs.end(); ++it) {
            medianCount += it->second;
            if (medianCount * 2 >= entireCount) {
                m_pcrDiff = it->first < ADJUST_PCR_DIFF ? 0 : it->first - ADJUST_PCR_DIFF;
#ifndef NDEBUG
                TCHAR debug[64];
                _stprintf_s(debug, TEXT("Estimated viewer PCR diff: %d\n"), m_pcrDiff);
                DEBUG_OUT(debug);
#endif
                break;
            }
        }
    }
}

LONGLONG CViewerClockEstimator::GetStreamPcr() const
{
    lock_recursive_mutex lock(m_streamLock);

    return m_streamPcr;
}

void CViewerClockEstimator::Reset()
{
    lock_recursive_mutex lock(m_streamLock);

    m_crcPcrDeq[0].clear();
    m_crcPcrDeq[1].clear();
    m_pcrDiff = DEFAULT_PCR_DIFF;
}

void CViewerClockEstimator::SetVideoPes(bool fViewer, bool fUnitStart, const BYTE *data, size_t len)
{
    if (fUnitStart) {
        m_pesHeaderRemain[fViewer] = -1;
        if (data[0] == 0 && data[1] == 0 && data[2] == 1 && len >= 9) {
            // TODO: ここでPTSを取って補正に使うとよりよいかもしれない
            m_pesHeaderRemain[fViewer] = 9 + data[8];
        }
    }
    if (m_pesHeaderRemain[fViewer] > 0) {
        // PESヘッダを無視する
        int n = std::min(m_pesHeaderRemain[fViewer], static_cast<int>(len));
        m_pesHeaderRemain[fViewer] -= n;
        data += n;
        len -= n;
    }
    if (m_pesHeaderRemain[fViewer] == 0) {
        SetVideoStream(fViewer, data, len);
    }
}

void CViewerClockEstimator::SetVideoStream(bool fViewer, const BYTE *data, size_t len)
{
    {
        lock_recursive_mutex lock(m_streamLock);

        if (!m_fEnabled) return;
    }

    int state = m_unitState[fViewer];
    size_t unitSize = m_unitSize[fViewer];
    DWORD crc = m_crc[fViewer];

    // スタートコードで区切られた各ユニットのCRC32値と取得時刻のペアを記録する
    for (size_t i = 0; i < len; ++i) {
        if (state <= 1 && data[i] == 0) {
            ++state;
            ++unitSize;
            crc = CalcCrc32(0, crc);
        }
        else if (state == 2 && data[i] == 1) {
            lock_recursive_mutex lock(m_streamLock);

            if (!m_fEnabled) return;
            state = 0;
            if (!m_crcPcrDeq[fViewer].empty()) {
                m_crcPcrDeq[fViewer].back().first = crc;
                if (unitSize < 40) {
                    // 小さすぎるものは除外
                    m_crcPcrDeq[fViewer].pop_back();
                }
            }
            unitSize = 1;
            crc = CalcCrc32(1);
            while (!m_crcPcrDeq[fViewer].empty()) {
                DWORD pcrDiff = static_cast<DWORD>(m_streamPcr) - m_crcPcrDeq[fViewer].front().second;
                if (pcrDiff < (fViewer ? VIEWER_PCR_QUEUE_DIFF : STREAM_PCR_QUEUE_DIFF)) {
                    break;
                }
                m_crcPcrDeq[fViewer].pop_front();
            }
            m_crcPcrDeq[fViewer].push_back(std::make_pair(crc, static_cast<DWORD>(m_streamPcr)));
        }
        else if (state != 2 || data[i] != 0) {
            state = 0;
            ++unitSize;
            crc = CalcCrc32(data[i], crc);
        }
    }

    m_unitState[fViewer] = state;
    m_unitSize[fViewer] = unitSize;
    m_crc[fViewer] = crc;
}

LONGLONG CViewerClockEstimator::GetViewerPcr() const
{
    lock_recursive_mutex lock(m_streamLock);

    return m_streamPcr < 0 ? -1 : (0x200000000 + m_streamPcr - m_pcrDiff) & 0x1ffffffff;
}
