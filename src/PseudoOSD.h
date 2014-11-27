#ifndef PSEUDO_OSD_H
#define PSEUDO_OSD_H


class CPseudoOSD
{
	HWND m_hwnd;
	COLORREF m_crBackColor;
	COLORREF m_crTextColor;
	HBITMAP m_hbm;

	class CWindowStyle {
	public:
		std::basic_string<TCHAR> Text;
		int Width;
		LOGFONT lf;
		CWindowStyle(LPCTSTR pszText,int Width_,const LOGFONT &lf_) : Text(pszText),Width(Width_),lf(lf_) {}
	};
	class CWindowPosition {
	public:
		int Left,Top,Height,ImageWidth;
		std::vector<CWindowStyle> StyleList;
		CWindowPosition() : Left(0),Top(0),Height(0),ImageWidth(0) {}
		int GetEntireWidth() const {
			int w=ImageWidth;
			std::vector<CWindowStyle>::const_iterator it = StyleList.begin();
			for (; it!=StyleList.end(); ++it) w+=it->Width;
			return w;
		}
	} m_Position;
	UINT_PTR m_TimerID;
	bool m_fHideText;
	int m_FlashingInterval;
	int m_Opacity;
	int m_BackOpacity;
	int m_StrokeWidth;
	int m_StrokeSmoothLevel;
	bool m_fStrokeByDilate;
	bool m_fHLLeft,m_fHLTop,m_fHLRight,m_fHLBottom;
	bool m_fVertAntiAliasing;
	RECT m_ImagePaintRect;
	bool m_fLayeredWindow;
	HWND m_hwndParent;
	HWND m_hwndOwner;
	POINT m_ParentPosition;
	bool m_fWindowPrepared;

	void DrawTextList(HDC hdc,int MultX,int MultY) const;
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
	static bool IsPseudoOSD(HWND hwnd);
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
	bool AddText(LPCTSTR pszText,int Width,const LOGFONT &lf);
	bool SetPosition(int Left,int Top,int Height);
	void GetPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const;
	void SetTextColor(COLORREF crText,COLORREF crBack);
	bool SetImage(HBITMAP hbm,int Width,const RECT *pPaintRect=NULL);
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
