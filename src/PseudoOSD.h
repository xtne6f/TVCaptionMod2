#ifndef PSEUDO_OSD_H
#define PSEUDO_OSD_H


class CPseudoOSD
{
	HWND m_hwnd;
	COLORREF m_crBackColor;
	struct STYLE_ELEM {
		tstring Text;
		HBITMAP hbm;
		int Width;
		int OriginalWidth;
		LOGFONT lf;
		COLORREF cr;
		union {
			RECT AdjustRect;
			RECT PaintRect;
		};
		STYLE_ELEM(LPCTSTR pszText,int Width_,int OriginalWidth_,const LOGFONT &lf_,COLORREF cr_,const RECT &AdjustRect_)
			: Text(pszText),hbm(nullptr),Width(Width_),OriginalWidth(OriginalWidth_),lf(lf_),cr(cr_),AdjustRect(AdjustRect_) {}
		STYLE_ELEM(HBITMAP hbm_,int Width_,COLORREF cr_,const RECT &PaintRect_)
			: hbm(hbm_),Width(Width_),OriginalWidth(Width_),cr(cr_),PaintRect(PaintRect_) {}
	};
	std::vector<STYLE_ELEM> m_StyleList;
	struct {
		int Left,OriginalLeft,Top,Height;
	} m_Position;
	int m_OffsetX;
	int m_OffsetY;
	double m_ScaleX;
	double m_ScaleY;
	WORD m_wSWFMode;
	UINT_PTR m_TimerID;
	int m_FlashingInterval;
	int m_Opacity;
	int m_BackOpacity;
	int m_StrokeWidth;
	int m_StrokeSmoothLevel;
	int m_StrokeByDilateThreshold;
	bool m_fHLLeft,m_fHLTop,m_fHLRight,m_fHLBottom;
	int m_VertAntiAliasingThreshold;
	bool m_fHideText;
	bool m_fWindowPrepared;
	bool m_fLayeredWindow;
	HWND m_hwndParent;
	HWND m_hwndOwner;
	POINT m_ParentPosition;
	bool m_fHide;

	void DrawTextList(HDC hdc,int MultX,int MultY,bool fSetColor) const;
	void DrawImageList(HDC hdc,int MultX,int MultY) const;
	void Draw(HDC hdc,const RECT &PaintRect) const;
	static bool AllocateWorkBitmap(int Width,int Height,int HeightMono,int *pAllocWidth);
	void UpdateLayeredWindow(HDC hdcCompose=nullptr,void *pBitsCompose=nullptr,int WidthCompose=0,int HeightCompose=0,bool fKeepAlpha=false);

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
	bool Create(HWND hwndParent,bool fLayeredWindow,bool fRenewWindow=false);
	bool Destroy();
	bool PrepareWindow();
	bool Show();
	bool Hide();
	bool IsVisible() const;
	void ClearText();
	void AddText(LPCTSTR pszText,int Width,int OriginalWidth,const LOGFONT &lf,COLORREF cr,const RECT &AdjustRect);
	void AddImage(HBITMAP hbm,int Width,COLORREF cr,const RECT &PaintRect);
	void SetPosition(int Left,int OriginalLeft,int Top,int Height);
	void GetOriginalPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const;
	void GetWindowPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const;
	void SetWindowOffset(int X,int Y);
	void SetWindowScale(double X,double Y);
	void SetSWFMode(WORD wMode) { m_wSWFMode=wMode; }
	WORD GetSWFMode() const { return m_wSWFMode; }
	void SetBackgroundColor(COLORREF crBack);
	bool SetOpacity(int Opacity,int BackOpacity=50);
	void SetStroke(int Width,int SmoothLevel,int ByDilateThreshold);
	void SetHighlightingBlock(bool fLeft,bool fTop,bool fRight,bool fBottom);
	void SetVerticalAntiAliasing(int Threshold);
	void SetFlashingInterval(int Interval);
	int GetFlashingInterval() { return m_FlashingInterval; }
	void OnParentMove();
	void OnParentSize();
	HBITMAP CreateBitmap();
	void Compose(HDC hdc,int Left,int Top);
private:
	CPseudoOSD(const CPseudoOSD &other);
	CPseudoOSD &operator=(const CPseudoOSD &other);
};


#endif
