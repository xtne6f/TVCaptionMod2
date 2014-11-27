#ifndef INCLUDE_UTIL_H
#define INCLUDE_UTIL_H

#ifdef _DEBUG
#define DEBUG_OUT(x) ::OutputDebugString(x)
#define DDEBUG_OUT
#else
#define DEBUG_OUT(x)
#endif

int GetPrivateProfileSignedInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, int nDefault, LPCTSTR lpFileName);
BOOL WritePrivateProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, int value, LPCTSTR lpFileName);
DWORD GetLongModuleFileName(HMODULE hModule, LPTSTR lpFileName, DWORD nSize);

#define APP_NAME TEXT("TVCaption2")

#define PCR_PER_MSEC 45

#define H_262_VIDEO         0x02
#define PES_PRIVATE_DATA    0x06
#define AVC_VIDEO           0x1B

typedef struct {
	int           sync;
	int           transport_error_indicator;
	int           payload_unit_start_indicator;
	int           transport_priority;
	int           pid;
	int           transport_scrambling_control;
	int           adaptation_field_control;
	int           continuity_counter;
} TS_HEADER;

typedef struct {
	int           adaptation_field_length;
	int           discontinuity_counter;
	int           random_access_indicator;
	int           elementary_stream_priority_indicator;
	int           pcr_flag;
	int           opcr_flag;
	int           splicing_point_flag;
	int           transport_private_data_flag;
	int           adaptation_field_extension_flag;
	unsigned int  pcr_45khz;
} ADAPTATION_FIELD; // (partial)

typedef struct {
    int             pointer_field;
    int             table_id;
    int             section_length;
    int             version_number;
    int             current_next_indicator;
    int             continuity_counter;
    int             data_count;
    unsigned char   data[1025];
} PSI;

typedef struct {
    int             program_number;
    int             version_number;
    int             pcr_pid;
    int             pid_count;
    unsigned char   stream_type[256];
    unsigned short  pid[256]; // PESの一部に限定
    PSI             psi;
} PMT;

#define PAT_PID_MAX 64

typedef struct {
    int             transport_stream_id;
    int             version_number;
    int             pid_count;
    //unsigned short  program_number[PAT_PID_MAX];
    unsigned short  pid[PAT_PID_MAX]; // NITを除く
    PMT             *pmt[PAT_PID_MAX];
    PSI             psi;
} PAT;

typedef struct {
    int           packet_start_code_prefix;
    int           stream_id;
    int           pes_packet_length;
    int           pts_dts_flags;
    unsigned int  pts_45khz;
    unsigned int  dts_45khz;
} PES_HEADER; // (partial)

void reset_pat(PAT *pat);
void extract_pat(PAT *pat, const unsigned char *payload, int payload_size, int unit_start, int counter);
void extract_pmt(PMT *pmt, const unsigned char *payload, int payload_size, int unit_start, int counter);
void extract_pes_header(PES_HEADER *dst, const unsigned char *payload, int payload_size/*, int stream_type*/);
void extract_adaptation_field(ADAPTATION_FIELD *dst, const unsigned char *data);

inline void extract_ts_header(TS_HEADER *dst, const unsigned char *packet)
{
    dst->sync                         = packet[0];
    dst->transport_error_indicator    = packet[1] & 0x80;
    dst->payload_unit_start_indicator = packet[1] & 0x40;
    dst->transport_priority           = packet[1] & 0x20;
    dst->pid = ((packet[1] & 0x1f) << 8) | packet[2];
    dst->transport_scrambling_control = (packet[3] >> 6) & 0x03;
    dst->adaptation_field_control     = (packet[3] >> 4) & 0x03;
    dst->continuity_counter           = packet[3] & 0x0f;
}

class CCriticalLock
{
public:
    CCriticalLock() { ::InitializeCriticalSection(&m_section); }
    ~CCriticalLock() { ::DeleteCriticalSection(&m_section); }
    void Lock() { ::EnterCriticalSection(&m_section); }
    void Unlock() { ::LeaveCriticalSection(&m_section); }
    //CRITICAL_SECTION &GetCriticalSection() { return m_section; }
private:
    CRITICAL_SECTION m_section;
};

class CBlockLock
{
public:
    CBlockLock(CCriticalLock *pLock) : m_pLock(pLock) { m_pLock->Lock(); }
    ~CBlockLock() { m_pLock->Unlock(); }
private:
    CCriticalLock *m_pLock;
};

bool CompareLogFont(const LOGFONT *pFont1,const LOGFONT *pFont2);

namespace DrawUtil {

bool Fill(HDC hdc,const RECT *pRect,COLORREF Color);
bool DrawBitmap(HDC hdc,int DstX,int DstY,int DstWidth,int DstHeight,
				HBITMAP hbm,const RECT *pSrcRect=NULL,BYTE Opacity=255);

enum FontType {
	FONT_DEFAULT,
	FONT_MESSAGE,
	FONT_MENU,
	FONT_CAPTION,
	FONT_SMALLCAPTION,
	FONT_STATUS
};

bool GetSystemFont(FontType Type,LOGFONT *pLogFont);

class CFont {
	HFONT m_hfont;
public:
	CFont();
	CFont(const CFont &Font);
	CFont(const LOGFONT &Font);
	CFont(FontType Type);
	~CFont();
	CFont &operator=(const CFont &Font);
	bool operator==(const CFont &Font) const;
	bool operator!=(const CFont &Font) const;
	bool Create(const LOGFONT *pLogFont);
	bool Create(FontType Type);
	bool IsCreated() const { return m_hfont!=NULL; }
	void Destroy();
	bool GetLogFont(LOGFONT *pLogFont) const;
	HFONT GetHandle() const { return m_hfont; }
	int GetHeight(bool fCell=true) const;
	int GetHeight(HDC hdc,bool fCell=true) const;
};

inline HFONT SelectObject(HDC hdc,const CFont &Font) {
	return static_cast<HFONT>(::SelectObject(hdc,Font.GetHandle()));
}

}	// namespace DrawUtil

#endif // INCLUDE_UTIL_H
