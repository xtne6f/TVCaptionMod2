#ifndef PSEUDO_OSD_H
#define PSEUDO_OSD_H


class CPseudoOSD
{
	HWND m_hwnd;
	COLORREF m_crBackColor;
	COLORREF m_crTextColor;
	LOGFONT m_LogFont;
	std::vector<std::basic_string<TCHAR>> m_TextList;
	std::vector<int> m_lfWidthList;
	HBITMAP m_hbm;
	class CWindowPosition {
	public:
		int Left,Top,Height,ImageWidth;
		std::vector<int> WidthList;
		CWindowPosition() : Left(0),Top(0),Height(0),ImageWidth(0) {}
		int GetEntireWidth() const {
			int w=ImageWidth;
			for (size_t i=0; i<WidthList.size(); i++)
				w+=WidthList[i];
			return w;
		}
	} m_Position;
	UINT_PTR m_TimerID;
	int m_AnimationCount;
	int m_Opacity;
	int m_BackOpacity;
	int m_StrokeWidth;
	int m_StrokeSmoothLevel;
	bool m_fStrokeByDilate;
	bool m_fHLLeft,m_fHLTop,m_fHLRight,m_fHLBottom;
	RECT m_ImagePaintRect;
	bool m_fLayeredWindow;
	HWND m_hwndParent;
	HWND m_hwndOwner;
	POINT m_ParentPosition;
	bool m_fWindowPrepared;

	void Draw(HDC hdc,const RECT &PaintRect) const;
	static bool AllocateWorkBitmap(int Width,int Height,int *pAllocWidth,int *pAllocHeight);
	void UpdateLayeredWindow();

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
	bool PrepareWindow(DWORD Time=0,bool fAnimation=false);
	bool Show(DWORD Time=0,bool fAnimation=false);
	bool Hide();
	bool IsVisible() const;
	void ClearText();
	bool AddText(LPCTSTR pszText,int Width,int lfWidth=0);
	bool SetPosition(int Left,int Top,int Height);
	void GetPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const;
	void SetTextColor(COLORREF crText,COLORREF crBack);
	bool SetImage(HBITMAP hbm,int Width,const RECT *pPaintRect=NULL);
	bool SetOpacity(int Opacity,int BackOpacity=50);
	void SetStroke(int Width,int SmoothLevel,bool fStrokeByDilate);
	void SetHighlightingBlock(bool fLeft,bool fTop,bool fRight,bool fBottom);
	void SetFont(const LOGFONT *pLogFont);
	void OnParentMove();
	void DrawTextList(HDC hdc,int Mult) const;
private:
	CPseudoOSD(const CPseudoOSD &other);
	CPseudoOSD &operator=(const CPseudoOSD &other);
};


#endif
