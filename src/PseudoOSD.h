#ifndef PSEUDO_OSD_H
#define PSEUDO_OSD_H


class CPseudoOSD
{
	HWND m_hwnd;
	COLORREF m_crBackColor;
	COLORREF m_crTextColor;
	struct STYLE_ELEM {
		std::basic_string<TCHAR> Text;
		HBITMAP hbm;
		int Width;
		LOGFONT lf;
		union {
			RECT AdjustRect;
			RECT PaintRect;
		};
		STYLE_ELEM(LPCTSTR pszText,int Width_,const LOGFONT &lf_,const RECT &AdjustRect_) : Text(pszText),hbm(NULL),Width(Width_),lf(lf_),AdjustRect(AdjustRect_) {}
		STYLE_ELEM(HBITMAP hbm_,int Width_,const RECT &PaintRect_) : hbm(hbm_),Width(Width_),PaintRect(PaintRect_) {}
	};
	std::vector<STYLE_ELEM> m_StyleList;
	struct {
		int Left,Top,Height;
	} m_Position;
	UINT_PTR m_TimerID;
	int m_FlashingInterval;
	int m_Opacity;
	int m_BackOpacity;
	int m_StrokeWidth;
	int m_StrokeSmoothLevel;
	bool m_fStrokeByDilate;
	bool m_fHLLeft,m_fHLTop,m_fHLRight,m_fHLBottom;
	bool m_fVertAntiAliasing;
	bool m_fHideText;
	bool m_fWindowPrepared;
	bool m_fLayeredWindow;
	HWND m_hwndParent;
	HWND m_hwndOwner;
	POINT m_ParentPosition;

	static int GetEntireWidth(const std::vector<STYLE_ELEM> &List);
	void DrawTextList(HDC hdc,int MultX,int MultY) const;
	void DrawImageList(HDC hdc,int MultX,int MultY) const;
	void Draw(HDC hdc,const RECT &PaintRect) const;
	static bool AllocateWorkBitmap(int Width,int Height,int HeightMono,int *pAllocWidth);
	void UpdateLayeredWindow(HDC hdcCompose=NULL,void *pBitsCompose=NULL,int WidthCompose=0,int HeightCompose=0,bool fKeepAlpha=false);

	static const LPCTSTR m_pszWindowClass;
	static HINSTANCE m_hinst;
	static int m_RefCount;
	static HBITMAP m_hbmWork;
	static HBITMAP m_hbmWorkMono;
	static void *m_pBits;
	static void *m_pBitsMono;
	static CPseudoOSD *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	static bool Initialize(HINSTANCE hinst);
	static void FreeWorkBitmap();

	CPseudoOSD();
	~CPseudoOSD();
	bool Create(HWND hwndParent,bool fLayeredWindow=false);
	bool Destroy();
	bool PrepareWindow();
	bool Show();
	bool Hide();
	bool IsVisible() const;
	void ClearText();
	bool AddText(LPCTSTR pszText,int Width,const LOGFONT &lf,const RECT &AdjustRect);
	bool AddImage(HBITMAP hbm,int Width,const RECT &PaintRect);
	bool SetPosition(int Left,int Top,int Height);
	void GetPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const;
	void SetTextColor(COLORREF crText,COLORREF crBack);
	bool SetOpacity(int Opacity,int BackOpacity=50);
	void SetStroke(int Width,int SmoothLevel,bool fStrokeByDilate);
	void SetHighlightingBlock(bool fLeft,bool fTop,bool fRight,bool fBottom);
	void SetVerticalAntiAliasing(bool fVertAntiAliasing);
	void SetFlashingInterval(int Interval);
	int GetFlashingInterval() { return m_FlashingInterval; }
	void OnParentMove();
	HBITMAP CreateBitmap();
	void Compose(HDC hdc,int Left,int Top);
private:
	CPseudoOSD(const CPseudoOSD &other);
	CPseudoOSD &operator=(const CPseudoOSD &other);
};


#endif
