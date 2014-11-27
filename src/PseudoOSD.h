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
	bool m_fHLLeft,m_fHLTop,m_fHLRight,m_fHLBottom;
	RECT m_ImagePaintRect;
	bool m_fLayeredWindow;
	HWND m_hwndParent;
	HWND m_hwndOwner;
	POINT m_ParentPosition;

	void Draw(HDC hdc,const RECT &PaintRect) const;
	void UpdateLayeredWindow();

	static const LPCTSTR m_pszWindowClass;
	static HINSTANCE m_hinst;
	static CPseudoOSD *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	static bool Initialize(HINSTANCE hinst);
	static bool IsPseudoOSD(HWND hwnd);

	CPseudoOSD();
	~CPseudoOSD();
	bool Create(HWND hwndParent,bool fLayeredWindow=false);
	bool Destroy();
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
	void SetStroke(int Width,int SmoothLevel);
	void SetHighlightingBlock(bool fLeft,bool fTop,bool fRight,bool fBottom);
	void SetFont(const LOGFONT *pLogFont);
	void OnParentMove();
private:
	CPseudoOSD(const CPseudoOSD &other);
	CPseudoOSD &operator=(const CPseudoOSD &other);
};


#endif
