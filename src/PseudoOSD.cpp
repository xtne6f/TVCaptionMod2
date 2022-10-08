#include <Windows.h>
#include <WindowsX.h>
#include "Util.h"
#include "PseudoOSD.h"

#if 0 // アセンブリ検索用
#define MAGIC_NUMBER(x) { g_dwMagic=(x); }
static DWORD g_dwMagic;
#else
#define MAGIC_NUMBER
#endif

#ifndef lengthof
#define lengthof _countof
#endif

// タイマーの識別子
#define TIMER_ID_FLASHING	1


const LPCTSTR CPseudoOSD::m_pszWindowClass=TEXT("TVCaption2 Pseudo OSD");
HINSTANCE CPseudoOSD::m_hinst=nullptr;
int CPseudoOSD::m_RefCount=0;

HBITMAP CPseudoOSD::m_hbmWork=nullptr;
HBITMAP CPseudoOSD::m_hbmWorkMono=nullptr;
void *CPseudoOSD::m_pBits=nullptr;
void *CPseudoOSD::m_pBitsMono=nullptr;


bool CPseudoOSD::Initialize(HINSTANCE hinst)
{
	if (!m_hinst) {
		WNDCLASS wc={};

		wc.style=CS_HREDRAW;
		wc.lpfnWndProc=WndProc;
		wc.hInstance=hinst;
		wc.lpszClassName=m_pszWindowClass;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CPseudoOSD::CPseudoOSD()
	: m_hwnd(nullptr)
	, m_crBackColor(RGB(16,0,16))
	, m_OffsetX(0)
	, m_OffsetY(0)
	, m_ScaleX(1)
	, m_ScaleY(1)
	, m_wSWFMode(0)
	, m_TimerID(0)
	, m_FlashingInterval(0)
	, m_Opacity(80)
	, m_BackOpacity(50)
	, m_StrokeWidth(0)
	, m_StrokeSmoothLevel(0)
	, m_StrokeByDilateThreshold(0)
	, m_fHLLeft(false)
	, m_fHLTop(false)
	, m_fHLRight(false)
	, m_fHLBottom(false)
	, m_VertAntiAliasingThreshold(0)
	, m_fHideText(false)
	, m_fWindowPrepared(false)
	, m_fLayeredWindow(false)
	, m_hwndParent(nullptr)
	, m_hwndOwner(nullptr)
{
	m_Position.Left=0;
	m_Position.Top=0;
	m_Position.Height=0;
	m_ParentPosition.x=0;
	m_ParentPosition.y=0;
	m_RefCount++;
}


CPseudoOSD::~CPseudoOSD()
{
	Destroy();
	if (--m_RefCount==0) {
		FreeWorkBitmap();
	}
}


bool CPseudoOSD::Create(HWND hwndParent,bool fLayeredWindow,bool fRenewWindow)
{
	if (m_hwnd) {
		if (!fRenewWindow && m_fLayeredWindow==fLayeredWindow &&
		    ((fLayeredWindow && m_hwndParent==hwndParent && ::GetWindow(m_hwnd,GW_OWNER)==m_hwndOwner) ||
		     (!fLayeredWindow && ::GetParent(m_hwnd)==hwndParent)))
			return true;
		::DestroyWindow(m_hwnd);
	}
	m_fLayeredWindow=fLayeredWindow;
	m_hwndParent=hwndParent;
	int x,y,w,h;
	GetWindowPosition(&x,&y,&w,&h);
	if (fLayeredWindow) {
		POINT pt;
		RECT rc;

		pt.x=x;
		pt.y=y;
		::ClientToScreen(hwndParent,&pt);
		if (::CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
							 m_pszWindowClass,nullptr,WS_POPUP,
							 pt.x,pt.y,w,h,hwndParent,nullptr,m_hinst,this)==nullptr)
			return false;

		::GetWindowRect(hwndParent,&rc);
		m_ParentPosition.x=rc.left;
		m_ParentPosition.y=rc.top;
		// WS_POPUPに親はいない。hwndParentからトップレベルまで遡ったウィンドウがオーナーになる
		m_hwndOwner = ::GetWindow(m_hwnd,GW_OWNER);
		return true;
	}
	return ::CreateWindowEx(0,m_pszWindowClass,nullptr,WS_CHILD,
							x,y,w,h,hwndParent,nullptr,m_hinst,this)!=nullptr;
}


bool CPseudoOSD::Destroy()
{
	if (m_hwnd)
		::DestroyWindow(m_hwnd);
	ClearText();
	return true;
}


// ウィンドウの描画作業を完了する
bool CPseudoOSD::PrepareWindow()
{
	if (!m_hwnd)
		return false;

	m_fHideText=m_FlashingInterval<0;
	m_fWindowPrepared=true;

	int x,y,w,h;
	GetWindowPosition(&x,&y,&w,&h);
	if (m_fLayeredWindow) {
		POINT pt;
		pt.x=x;
		pt.y=y;
		::ClientToScreen(m_hwndParent,&pt);
		::SetWindowPos(m_hwnd,nullptr,pt.x,pt.y,w,h,SWP_NOZORDER|SWP_NOACTIVATE);
		UpdateLayeredWindow();
	} else {
		::SetWindowPos(m_hwnd,HWND_TOP,x,y,w,h,0);
	}
	return true;
}


bool CPseudoOSD::Show()
{
	if (!m_fWindowPrepared)
		PrepareWindow();
	m_fWindowPrepared=false;

	if (!m_hwnd)
		return false;

	if (m_FlashingInterval!=0) {
		m_TimerID|=::SetTimer(m_hwnd,TIMER_ID_FLASHING,
		                      m_FlashingInterval<0?-m_FlashingInterval:m_FlashingInterval,nullptr);
	}
	if (m_fLayeredWindow) {
		// 実際には親子関係じゃないので自力で可視になるか判断する必要がある
		if (::IsWindowVisible(m_hwndParent)) {
			::ShowWindow(m_hwnd,SW_SHOWNOACTIVATE);
		}
		::UpdateWindow(m_hwnd);

		RECT rc;
		::GetWindowRect(m_hwndParent,&rc);
		m_ParentPosition.x=rc.left;
		m_ParentPosition.y=rc.top;
		return true;
	}
	if (::IsWindowVisible(m_hwnd)) {
		::RedrawWindow(m_hwnd,nullptr,nullptr,RDW_INVALIDATE | RDW_UPDATENOW);
	} else {
		::ShowWindow(m_hwnd,SW_SHOW);
		::BringWindowToTop(m_hwnd);
		::UpdateWindow(m_hwnd);
	}
	return true;
}


bool CPseudoOSD::Hide()
{
	if (!m_hwnd)
		return false;
	::ShowWindow(m_hwnd,SW_HIDE);
	ClearText();
	if (m_TimerID&TIMER_ID_FLASHING) {
		::KillTimer(m_hwnd,TIMER_ID_FLASHING);
		m_TimerID&=~TIMER_ID_FLASHING;
	}
	return true;
}


bool CPseudoOSD::IsVisible() const
{
	if (!m_hwnd)
		return false;
	return ::IsWindowVisible(m_hwnd)!=FALSE;
}


void CPseudoOSD::ClearText()
{
	if (!m_StyleList.empty()) {
		std::vector<STYLE_ELEM>::const_iterator it = m_StyleList.begin();
		for (; it!=m_StyleList.end(); ++it) {
			if (it->hbm) ::DeleteObject(it->hbm);
		}
		m_StyleList.clear();
	}
}


// テキストを追加する
// lf.lfWidth<0のときは半角テキスト間隔で描画する
void CPseudoOSD::AddText(LPCTSTR pszText,int Width,const LOGFONT &lf,COLORREF cr,const RECT &AdjustRect)
{
	m_StyleList.push_back(STYLE_ELEM(pszText,Width,lf,cr,AdjustRect));
}


// 画像を追加する
// 受けとったビットマップはクラス側で破棄する
void CPseudoOSD::AddImage(HBITMAP hbm,int Width,COLORREF cr,const RECT &PaintRect)
{
	m_StyleList.push_back(STYLE_ELEM(hbm,Width,cr,PaintRect));
}


void CPseudoOSD::SetPosition(int Left,int Top,int Height)
{
	m_Position.Left=Left;
	m_Position.Top=Top;
	m_Position.Height=max(Height,0);
}


void CPseudoOSD::GetWindowPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const
{
	if (pLeft)
		*pLeft=(int)(m_Position.Left*m_ScaleX)+m_OffsetX;
	if (pTop)
		*pTop=(int)(m_Position.Top*m_ScaleY)+m_OffsetY;
	if (pWidth) {
		int x=0;
		for (std::vector<STYLE_ELEM>::const_iterator it=m_StyleList.begin(); it!=m_StyleList.end(); x+=(it++)->Width);
		*pWidth=(int)((m_Position.Left+x)*m_ScaleX)-(int)(m_Position.Left*m_ScaleX);
	}
	if (pHeight)
		*pHeight=(int)((m_Position.Top+m_Position.Height)*m_ScaleY)-(int)(m_Position.Top*m_ScaleY);
}


void CPseudoOSD::SetWindowOffset(int X,int Y)
{
	m_OffsetX=X;
	m_OffsetY=Y;
}


void CPseudoOSD::SetWindowScale(double X,double Y)
{
	m_ScaleX=max(X,0.0);
	m_ScaleY=max(Y,0.0);
}


void CPseudoOSD::SetBackgroundColor(COLORREF crBack)
{
	m_crBackColor=crBack;
}


bool CPseudoOSD::SetOpacity(int Opacity,int BackOpacity)
{
	m_Opacity=Opacity;
	m_BackOpacity=BackOpacity;
	return true;
}


void CPseudoOSD::SetStroke(int Width,int SmoothLevel,int ByDilateThreshold)
{
	m_StrokeWidth=Width;
	m_StrokeSmoothLevel=SmoothLevel;
	m_StrokeByDilateThreshold=ByDilateThreshold;
}


void CPseudoOSD::SetHighlightingBlock(bool fLeft,bool fTop,bool fRight,bool fBottom)
{
	m_fHLLeft=fLeft;
	m_fHLTop=fTop;
	m_fHLRight=fRight;
	m_fHLBottom=fBottom;
}


void CPseudoOSD::SetVerticalAntiAliasing(int Threshold)
{
	m_VertAntiAliasingThreshold=Threshold;
}


void CPseudoOSD::SetFlashingInterval(int Interval)
{
	m_FlashingInterval=Interval;
}


void CPseudoOSD::OnParentMove()
{
	if (m_hwnd && m_fLayeredWindow) {
		RECT rcParent,rc;

		::GetWindowRect(m_hwndParent,&rcParent);
		::GetWindowRect(m_hwnd,&rc);
		::OffsetRect(&rc,
					 rcParent.left-m_ParentPosition.x,
					 rcParent.top-m_ParentPosition.y);
		::SetWindowPos(m_hwnd,nullptr,rc.left,rc.top,
					   rc.right-rc.left,rc.bottom-rc.top,
					   SWP_NOZORDER | SWP_NOACTIVATE);
		m_ParentPosition.x=rcParent.left;
		m_ParentPosition.y=rcParent.top;
	}
}


static BOOL TextOutMonospace(HDC hdc,int x,int y,LPCTSTR lpString,UINT cbCount,int Width,int MultX,int MultY)
{
	INT dx[1024];
	cbCount=min(cbCount,lengthof(dx));
	UINT cbCountWos=0;
	for (UINT i=0; i<cbCount; i++) {
		if ((lpString[i] & 0xFC00) != 0xDC00) cbCountWos++;
	}
	for (UINT i=0; i<cbCount; i++) {
		if ((lpString[i] & 0xFC00) != 0xDC00) {
			dx[i]=Width/cbCountWos*MultX;
			Width-=Width/cbCountWos;
			cbCountWos--;
		} else {
			dx[i]=0;
		}
	}
	return ::ExtTextOut(hdc,x*MultX,y*MultY,0,nullptr,lpString,cbCount,dx);
}

static void DrawLine(HDC hdc,int bx,int by,int ex,int ey,int cw,COLORREF cr)
{
	LOGBRUSH lb;
	lb.lbStyle=BS_SOLID;
	lb.lbColor=cr;
	lb.lbHatch=0;

	HPEN hPen=::ExtCreatePen(PS_SOLID|PS_GEOMETRIC|PS_ENDCAP_SQUARE,cw,&lb,0,nullptr);
	if (hPen) {
		HGDIOBJ hPenOld=::SelectObject(hdc,hPen);
		POINT lastPos;
		::MoveToEx(hdc,bx,by,&lastPos);
		::LineTo(hdc,ex,ey);
		::MoveToEx(hdc,lastPos.x,lastPos.y,nullptr);
		::SelectObject(hdc,hPenOld);
		::DeleteObject(hPen);
	}
}

void CPseudoOSD::DrawTextList(HDC hdc,int MultX,int MultY,bool fSetColor) const
{
	UINT oldTa=::SetTextAlign(hdc,TA_LEFT|TA_BOTTOM|TA_NOUPDATECP);
	int x=0;
	std::vector<STYLE_ELEM>::const_iterator it = m_StyleList.begin();
	LOGFONT lfLast={};
	HFONT hfont=nullptr;
	for (; it!=m_StyleList.end(); ++it) {
		int lenWos=StrlenWoLoSurrogate(it->Text.c_str());
		if (lenWos > 0) {
			LOGFONT lf=it->lf;
			int lfScaledWidth=(int)(lf.lfWidth*m_ScaleX);
			int lfScaledHeight=(int)(lf.lfHeight*m_ScaleY);
			int adjSize=it->AdjustRect.right;
			int adjRatio=it->AdjustRect.bottom;
			lf.lfWidth=lfScaledWidth*(lfScaledWidth<0?-1:1)*(adjSize*adjRatio/100)/100*MultX;
			lf.lfHeight=lfScaledHeight*adjSize/100*MultY;
			//フォントが変化するときだけ作る
			if (!CompareLogFont(lf,lfLast)) {
				if (hfont) ::DeleteObject(hfont);
				hfont=::CreateFontIndirect(&lf);
				lfLast=lf;
			}
			if (hfont) {
				COLORREF crOld = RGB(0,0,0);
				if (fSetColor) ::SetTextColor(hdc,it->cr);
				HGDIOBJ hfontOld=::SelectObject(hdc,hfont);
				int w=(int)((x+it->Width)*m_ScaleX)-(int)(x*m_ScaleX);
				int h=(int)((m_Position.Height)*m_ScaleY);
				int intvX=w/lenWos-lfScaledWidth*(lfScaledWidth<0?-1:2);
				int intvY=h-lfScaledHeight*(lfScaledHeight<0?-1:1)*adjSize/100;
				int adjLeft=lfScaledHeight*(lfScaledHeight<0?-1:1)*it->AdjustRect.left/72;
				int adjTop=lfScaledHeight*(lfScaledHeight<0?-1:1)*it->AdjustRect.top/72;
				TextOutMonospace(hdc,(int)(x*m_ScaleX)+intvX/2+adjLeft,h-1-intvY/2+adjTop,
				                 it->Text.c_str(),(int)it->Text.length(),w,MultX,MultY);
				::SelectObject(hdc,hfontOld);
				if (fSetColor) ::SetTextColor(hdc,crOld);
			}
		}
		x+=it->Width;
	}
	if (hfont) ::DeleteObject(hfont);
	::SetTextAlign(hdc,oldTa);
}

void CPseudoOSD::DrawImageList(HDC hdc,int MultX,int MultY) const
{
	int x=0;
	std::vector<STYLE_ELEM>::const_iterator it = m_StyleList.begin();
	for (; it!=m_StyleList.end(); ++it) {
		if (it->hbm) {
			DrawUtil::DrawBitmap(hdc,(int)((x+it->PaintRect.left)*m_ScaleX)*MultX,(int)(it->PaintRect.top*m_ScaleY)*MultY,
			                     (int)(it->PaintRect.right*m_ScaleX)*MultX,(int)(it->PaintRect.bottom*m_ScaleY)*MultY,it->hbm);
		}
		x+=it->Width;
	}
}

void CPseudoOSD::Draw(HDC hdc,const RECT &PaintRect) const
{
	static_cast<void>(PaintRect);
	RECT rc;

	::GetClientRect(m_hwnd,&rc);
	DrawUtil::Fill(hdc,&rc,m_crBackColor);
	if (m_fHideText) return;

	if (!m_StyleList.empty()) {
		if (m_fHLLeft) DrawLine(hdc,1,rc.bottom-1,1,1,2,m_StyleList.front().cr);
		if (m_fHLTop) DrawLine(hdc,1,1,rc.right-1,1,2,m_StyleList.front().cr);
		if (m_fHLRight) DrawLine(hdc,rc.right-1,1,rc.right-1,rc.bottom-1,2,m_StyleList.front().cr);
		if (m_fHLBottom) DrawLine(hdc,rc.right-1,rc.bottom-1,1,rc.bottom-1,2,m_StyleList.front().cr);
	}

	int OldBkMode=::SetBkMode(hdc,TRANSPARENT);
	DrawTextList(hdc,1,1,true);
	::SetBkMode(hdc,OldBkMode);

	DrawImageList(hdc,1,1);
}

// OSDのビットマップイメージを生成する
HBITMAP CPseudoOSD::CreateBitmap()
{
	int Width,Height;
	GetWindowPosition(nullptr,nullptr,&Width,&Height);
	if (Width<1 || Height<1 || !m_fLayeredWindow) return nullptr;

	// ここだけbottom-upビットマップなので注意
	void *pBits;
	BITMAPINFO bmi={};
	bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth=Width;
	bmi.bmiHeader.biHeight=Height;
	bmi.bmiHeader.biPlanes=1;
	bmi.bmiHeader.biBitCount=32;
	bmi.bmiHeader.biCompression=BI_RGB;
	HBITMAP hbm=::CreateDIBSection(nullptr,&bmi,DIB_RGB_COLORS,&pBits,nullptr,0);
	if (hbm) {
		HDC hdcTmp=::CreateCompatibleDC(nullptr);
		HBITMAP hbmOld=static_cast<HBITMAP>(::SelectObject(hdcTmp,hbm));
		UpdateLayeredWindow(hdcTmp,pBits,Width,Height,true);
		::SelectObject(hdcTmp,hbmOld);
		::DeleteDC(hdcTmp);
	}
	return hbm;
}

// 表示中のOSDのイメージをhdcに合成する
void CPseudoOSD::Compose(HDC hdc,int Left,int Top)
{
	if (!hdc || !IsVisible()) return;
	RECT rc;
	::GetClientRect(m_hwnd,&rc);
	if (rc.right<1 || rc.bottom<1) return;

	// OSDのイメージを描画する一時ビットマップ
	void *pBits;
	BITMAPINFO bmi={};
	bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth=rc.right;
	bmi.bmiHeader.biHeight=-rc.bottom;
	bmi.bmiHeader.biPlanes=1;
	bmi.bmiHeader.biBitCount=32;
	bmi.bmiHeader.biCompression=BI_RGB;
	HBITMAP hbmTmp=::CreateDIBSection(nullptr,&bmi,DIB_RGB_COLORS,&pBits,nullptr,0);
	if (!hbmTmp) return;

	HDC hdcTmp=::CreateCompatibleDC(hdc);
	HBITMAP hbmOld=static_cast<HBITMAP>(::SelectObject(hdcTmp,hbmTmp));
	if (m_fLayeredWindow) {
		// アルファ合成のために背景をコピーしておく
		::BitBlt(hdcTmp,0,0,rc.right,rc.bottom,hdc,Left,Top,SRCCOPY);
		UpdateLayeredWindow(hdcTmp,pBits,rc.right,rc.bottom);
	} else {
		Draw(hdcTmp,rc);
	}
	::BitBlt(hdc,Left,Top,rc.right,rc.bottom,hdcTmp,0,0,SRCCOPY);
	::SelectObject(hdcTmp,hbmOld);
	::DeleteDC(hdcTmp);
	::DeleteObject(hbmTmp);
}

// インプレースでビットマップの縦方向を1/4に縮小処理する
static void ShrinkBitmap(void *pBits,int Width,RECT Rect)
{
	MAGIC_NUMBER(0x47920258);
	for (int y=0;y<Rect.bottom-Rect.top;y++) {
		DWORD *p=static_cast<DWORD*>(pBits)+(Rect.top+y)*Width+Rect.left;
		DWORD *q0=static_cast<DWORD*>(pBits)+(Rect.top+y*4+0)*Width+Rect.left;
		DWORD *q1=static_cast<DWORD*>(pBits)+(Rect.top+y*4+1)*Width+Rect.left;
		DWORD *q2=static_cast<DWORD*>(pBits)+(Rect.top+y*4+2)*Width+Rect.left;
		DWORD *q3=static_cast<DWORD*>(pBits)+(Rect.top+y*4+3)*Width+Rect.left;
		for (int x=Rect.left;x<Rect.right;x++) {
			DWORD v0=(*q0&*q1)+(((*q0^*q1)&0xfefefefe)>>1);
			DWORD v1=(*q2&*q3)+(((*q2^*q3)&0xfefefefe)>>1);
			*p++=(v0&v1)+(((v0^v1)&0xfefefefe)>>1);
			q0++;
			q1++;
			q2++;
			q3++;
		}
	}
}

// インプレースでビットマップの縦方向を1/2に縮小処理する
static void ShrinkBitmapHalf(void *pBits,int Width,RECT Rect)
{
	MAGIC_NUMBER(0x85756764);
	for (int y=0;y<Rect.bottom-Rect.top;y++) {
		DWORD *p=static_cast<DWORD*>(pBits)+(Rect.top+y)*Width+Rect.left;
		DWORD *q0=static_cast<DWORD*>(pBits)+(Rect.top+y*2+0)*Width+Rect.left;
		DWORD *q1=static_cast<DWORD*>(pBits)+(Rect.top+y*2+1)*Width+Rect.left;
		for (int x=Rect.left;x<Rect.right;x++) {
			*p++=(*q0&*q1)+(((*q0^*q1)&0xfefefefe)>>1);
			q0++;
			q1++;
		}
	}
}

// 縦横4倍の2値画像をもとにアルファチャネルを構成する
static void SetBitmapOpacityQuarter(BYTE* __restrict pBits,int Width,RECT Rect,const BYTE* __restrict pBitsMono,BYTE Opacity)
{
	MAGIC_NUMBER(0x77867563);
	if (Rect.left%2!=0) return;
	static const int BitSum[16]={ 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4 };

	int stride=(Width*4+31)/32*4;
	for (int y=Rect.top;y<Rect.bottom;y++) {
		int i=(y*Width+Rect.left)*4;
		int j0=(y*4+0)*stride+Rect.left/2;
		int j1=(y*4+1)*stride+Rect.left/2;
		int j2=(y*4+2)*stride+Rect.left/2;
		int j3=(y*4+3)*stride+Rect.left/2;
		int x=Rect.left/2;
		for (;x<Rect.right/2;x++) {
			pBits[i+3]=(BYTE)(pBits[i+3]+(BitSum[pBitsMono[j0]>>4]+BitSum[pBitsMono[j1]>>4]+
			                              BitSum[pBitsMono[j2]>>4]+BitSum[pBitsMono[j3]>>4])*(Opacity-pBits[i+3])/16);
			pBits[i+7]=(BYTE)(pBits[i+7]+(BitSum[pBitsMono[j0]&15]+BitSum[pBitsMono[j1]&15]+
			                              BitSum[pBitsMono[j2]&15]+BitSum[pBitsMono[j3]&15])*(Opacity-pBits[i+7])/16);
			i+=8;
			j0++;
			j1++;
			j2++;
			j3++;
		}
		if (x*2<Rect.right) {
			pBits[i+3]=(BYTE)(pBits[i+3]+(BitSum[pBitsMono[j0]>>4]+BitSum[pBitsMono[j1]>>4]+
			                              BitSum[pBitsMono[j2]>>4]+BitSum[pBitsMono[j3]>>4])*(Opacity-pBits[i+3])/16);
		}
	}
}

// 縦横2倍の2値画像(ただし横方向は4倍画像の左半分使用)をもとにアルファチャネルを構成する
static void SetBitmapOpacityHalf(BYTE* __restrict pBits,int Width,RECT Rect,const BYTE* __restrict pBitsMono,BYTE Opacity)
{
	MAGIC_NUMBER(0x37461855);
	if (Rect.left%4!=0) return;

	int stride=(Width*4+31)/32*4;
	for (int y=Rect.top;y<Rect.bottom;y++) {
		int i=(y*Width+Rect.left)*4;
		int j0=(y*2+0)*stride+Rect.left/4;
		int j1=(y*2+1)*stride+Rect.left/4;
		int x=Rect.left/4;
		for (;x<Rect.right/4;x++) {
			BYTE m=(pBitsMono[j0]&0x55)+(pBitsMono[j0]>>1&0x55);
			BYTE n=(pBitsMono[j1]&0x55)+(pBitsMono[j1]>>1&0x55);
			BYTE sum0=(m&0x33)+(n&0x33);
			BYTE sum1=(m>>2&0x33)+(n>>2&0x33);
			pBits[i+3]=(BYTE)(pBits[i+3]+(sum1>>4)*(Opacity-pBits[i+3])/4);
			pBits[i+7]=(BYTE)(pBits[i+7]+(sum0>>4)*(Opacity-pBits[i+7])/4);
			pBits[i+11]=(BYTE)(pBits[i+11]+(sum1&15)*(Opacity-pBits[i+11])/4);
			pBits[i+15]=(BYTE)(pBits[i+15]+(sum0&15)*(Opacity-pBits[i+15])/4);
			i+=16;
			j0++;
			j1++;
		}
		if (x*4<Rect.right) {
			BYTE m=(pBitsMono[j0]&0x55)+(pBitsMono[j0]>>1&0x55);
			BYTE n=(pBitsMono[j1]&0x55)+(pBitsMono[j1]>>1&0x55);
			BYTE sum0=(m&0x33)+(n&0x33);
			BYTE sum1=(m>>2&0x33)+(n>>2&0x33);
			pBits[i+3]=(BYTE)(pBits[i+3]+(sum1>>4)*(Opacity-pBits[i+3])/4);
			if (x*4+1<Rect.right)
				pBits[i+7]=(BYTE)(pBits[i+7]+(sum0>>4)*(Opacity-pBits[i+7])/4);
			if (x*4+2<Rect.right)
				pBits[i+11]=(BYTE)(pBits[i+11]+(sum1&15)*(Opacity-pBits[i+11])/4);
		}
	}
}

static void SetBitmapOpacity(int Mult, BYTE* __restrict pBits,int Width,const RECT& Rect,
                             const BYTE* __restrict pBitsMono,BYTE Opacity)
{
	if (Mult==4) {
		SetBitmapOpacityQuarter(pBits,Width,Rect,pBitsMono,Opacity);
	} else if (Mult==2) {
		SetBitmapOpacityHalf(pBits,Width,Rect,pBitsMono,Opacity);
	}
}

// アルファチャネルを初期化する
static void ClearBitmapAlpha(void *pBits,int Width,RECT Rect,BYTE Opacity)
{
	for (int y=Rect.top;y<Rect.bottom;y++) {
		DWORD *p=static_cast<DWORD*>(pBits)+y*Width+Rect.left;
		for (int x=Rect.left;x<Rect.right;x++) {
			*p=(Opacity<<24)|(*p&0x00FFFFFF);
			p++;
		}
	}
}

// crKeyではない画素のアルファチャネルを置き換える
static void LayBitmapToAlpha(void *pBits,int Width,RECT Rect,BYTE Opacity,COLORREF crKey)
{
	MAGIC_NUMBER(0x66735267);
	DWORD dwKey=(GetRValue(crKey)<<16)|(GetGValue(crKey)<<8)|GetBValue(crKey);
	for (int y=Rect.top;y<Rect.bottom;y++) {
		DWORD *p=static_cast<DWORD*>(pBits)+y*Width+Rect.left;
		for (int x=Rect.left;x<Rect.right;x++) {
			if ((*p&0x00FFFFFF)!=dwKey)
				*p=(Opacity<<24)|(*p&0x00FFFFFF);
			p++;
		}
	}
}

static void PremultiplyBitmap(void *pBits,int Width,RECT Rect)
{
	MAGIC_NUMBER(0x67683625);
	for (int y=Rect.top;y<Rect.bottom;y++) {
		BYTE *p=static_cast<BYTE*>(pBits)+(y*Width+Rect.left)*4;
		for (int x=Rect.left;x<Rect.right;x++) {
			p[0]=(p[0]*p[3]+255)>>8;
			p[1]=(p[1]*p[3]+255)>>8;
			p[2]=(p[2]*p[3]+255)>>8;
			p+=4;
		}
	}
}

// Opacityなアルファチャネルを膨張させる
static void DilateAlpha(void *pBits,int Width,RECT Rect,BYTE Opacity)
{
	MAGIC_NUMBER(0x94275645);
	BYTE LastLine[1+8192];
	if (Rect.right-Rect.left<=1 || 1+(Rect.right-Rect.left)>lengthof(LastLine)) return;

	::ZeroMemory(LastLine,1+(Rect.right-Rect.left));
	for (int y=Rect.top;y<Rect.bottom-1;y++) {
		BYTE *pp=LastLine+1;
		BYTE *q=static_cast<BYTE*>(pBits)+(y*Width+Rect.left)*4;
		BYTE *r=static_cast<BYTE*>(pBits)+((y+1)*Width+Rect.left)*4;
		BYTE LastPixel[3]={0,0,0};
		for (int x=Rect.left;x<Rect.right-1;x++) {
			BYTE i[9]={LastPixel[0],pp[0],pp[1],
			           LastPixel[1],q[3],q[4+3],
			           LastPixel[2],r[3],r[4+3]};
			pp[-1]=LastPixel[1];
			LastPixel[0]=i[1];
			LastPixel[1]=i[4];
			LastPixel[2]=i[7];
			if (i[0]==Opacity||i[1]==Opacity||i[2]==Opacity||i[3]==Opacity||
			    i[5]==Opacity||i[6]==Opacity||i[7]==Opacity||i[8]==Opacity) q[3]=Opacity;
			pp++;
			q+=4;
			r+=4;
		}
		// 右端処理
		BYTE i[9]={LastPixel[0],pp[0],0,
		           LastPixel[1],q[3],0,
		           LastPixel[2],r[3],0};
		if (i[0]==Opacity||i[1]==Opacity||i[2]==Opacity||i[3]==Opacity||
		    i[5]==Opacity||i[6]==Opacity||i[7]==Opacity||i[8]==Opacity) q[3]=Opacity;
	}
}

// アルファチャネルを平滑化する
static void SmoothAlpha(void *pBits,int Width,RECT Rect,BYTE EdgeOpacity,int SmoothLevel)
{
	MAGIC_NUMBER(0x47673283);
	static const BYTE Coefs[5][10]={
		{1,5,1,5,25,5,1,5,1,49},
		{1,4,1,4,16,4,1,4,1,36},
		{1,3,1,3,9,3,1,3,1,25},
		{1,2,1,2,4,2,1,2,1,16},
		{1,1,1,1,1,1,1,1,1,9},
	};
	BYTE LastLine[1+8192];
	if (SmoothLevel<0 || lengthof(Coefs)<=SmoothLevel ||
	    Rect.right-Rect.left<=1 || 1+(Rect.right-Rect.left)>lengthof(LastLine)) return;

	const BYTE *c=Coefs[SmoothLevel];
	::FillMemory(LastLine,1+(Rect.right-Rect.left),EdgeOpacity);
	for (int y=Rect.top;y<Rect.bottom-1;y++) {
		BYTE *pp=LastLine+1;
		BYTE *q=static_cast<BYTE*>(pBits)+(y*Width+Rect.left)*4;
		BYTE *r=static_cast<BYTE*>(pBits)+((y+1)*Width+Rect.left)*4;
		BYTE LastPixel[3]={EdgeOpacity,EdgeOpacity,EdgeOpacity};
		for (int x=Rect.left;x<Rect.right-1;x++) {
			BYTE i[9]={LastPixel[0],pp[0],pp[1],
			           LastPixel[1],q[3],q[4+3],
			           LastPixel[2],r[3],r[4+3]};
			pp[-1]=LastPixel[1];
			LastPixel[0]=i[1];
			LastPixel[1]=i[4];
			LastPixel[2]=i[7];
			q[3]=(BYTE)((i[0]*c[0]+i[1]*c[1]+i[2]*c[2]+
			             i[3]*c[3]+i[4]*c[4]+i[5]*c[5]+
			             i[6]*c[6]+i[7]*c[7]+i[8]*c[8])/c[9]);
			pp++;
			q+=4;
			r+=4;
		}
		// 右端処理
		BYTE i[9]={LastPixel[0],pp[0],EdgeOpacity,
		           LastPixel[1],q[3],EdgeOpacity,
		           LastPixel[2],r[3],EdgeOpacity};
		q[3]=(BYTE)((i[0]*c[0]+i[1]*c[1]+i[2]*c[2]+
		             i[3]*c[3]+i[4]*c[4]+i[5]*c[5]+
		             i[6]*c[6]+i[7]*c[7]+i[8]*c[8])/c[9]);
	}
}

// アルファ合成する
static void ComposeAlpha(BYTE* __restrict pBitsDest,int WidthDest,const BYTE* __restrict pBits,int Width,RECT Rect)
{
	MAGIC_NUMBER(0x36481956);
	for (int y=Rect.top,yy=0;y<Rect.bottom;y++,yy++) {
		int i=(y*Width+Rect.left)*4;
		int j=(yy*WidthDest)*4;
		for (int x=Rect.left;x<Rect.right;x++) {
			pBitsDest[j+0]=((pBits[i+0]<<8) + (pBitsDest[j+0]*(255-pBits[i+3])+255))>>8;
			pBitsDest[j+1]=((pBits[i+1]<<8) + (pBitsDest[j+1]*(255-pBits[i+3])+255))>>8;
			pBitsDest[j+2]=((pBits[i+2]<<8) + (pBitsDest[j+2]*(255-pBits[i+3])+255))>>8;
			pBitsDest[j+3]=0;
			i+=4;
			j+=4;
		}
	}
}

void CPseudoOSD::FreeWorkBitmap()
{
	if (m_hbmWork) {
		::DeleteObject(m_hbmWork);
		::DeleteObject(m_hbmWorkMono);
		m_hbmWork=nullptr;
		m_hbmWorkMono=nullptr;
	}
}

// 作業用ビットマップを確保
// 全オブジェクトで共用(状況によってはMB単位の確保と解放を秒単位でくり返すことになるため)
bool CPseudoOSD::AllocateWorkBitmap(int Width,int Height,int HeightMono,int *pAllocWidth)
{
	if (m_hbmWork) {
		BITMAP bm,bmMono;
		if (::GetObject(m_hbmWork,sizeof(BITMAP),&bm) &&
		    ::GetObject(m_hbmWorkMono,sizeof(BITMAP),&bmMono))
		{
			if (bm.bmWidth>=Width && bm.bmHeight>=Height && bmMono.bmHeight>=HeightMono) {
				*pAllocWidth=bm.bmWidth;
				return true;
			}
			Width=max(Width,bm.bmWidth);
			Height=max(Height,bm.bmHeight);
			HeightMono=max(HeightMono,bmMono.bmHeight);
		}
		FreeWorkBitmap();
	}
	BITMAPINFO bmi;
	::ZeroMemory(&bmi,sizeof(bmi));
	bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth=Width;
	bmi.bmiHeader.biHeight=-Height;
	bmi.bmiHeader.biPlanes=1;
	bmi.bmiHeader.biBitCount=32;
	bmi.bmiHeader.biCompression=BI_RGB;
	m_hbmWork=::CreateDIBSection(nullptr,&bmi,DIB_RGB_COLORS,&m_pBits,nullptr,0);
	if (!m_hbmWork) {
		return false;
	}

	// アルファチャネル描画用の横4倍の2値画像
	struct {
		BITMAPINFOHEADER bmiHeader;
		RGBQUAD bmiColors[2];
	} bmiMono={};
	bmiMono.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bmiMono.bmiHeader.biWidth=Width*4;
	bmiMono.bmiHeader.biHeight=-HeightMono;
	bmiMono.bmiHeader.biPlanes=1;
	bmiMono.bmiHeader.biBitCount=1;
	bmiMono.bmiHeader.biCompression=BI_RGB;
	bmiMono.bmiColors[1].rgbBlue=255;
	bmiMono.bmiColors[1].rgbGreen=255;
	bmiMono.bmiColors[1].rgbRed=255;
	m_hbmWorkMono=::CreateDIBSection(nullptr,(BITMAPINFO*)&bmiMono,DIB_RGB_COLORS,&m_pBitsMono,nullptr,0);
	if (!m_hbmWorkMono) {
		::DeleteObject(m_hbmWork);
		m_hbmWork=nullptr;
		return false;
	}
	*pAllocWidth=Width;
#ifdef DDEBUG_OUT
	TCHAR szDebug[128];
	_stprintf_s(szDebug,TEXT(__FUNCTION__) TEXT("(): w=%d,h=%d,hm=%d\n"),Width,Height,HeightMono);
	DEBUG_OUT(szDebug);
#endif
	return true;
}

// hdcCompose!=nullptrのとき、このデバイスコンテキストのpBitsCompose(32bitDIBビット値)に合成する
// さらにfKeepAlpha==trueのとき、アルファ値つきで描画する
void CPseudoOSD::UpdateLayeredWindow(HDC hdcCompose,void *pBitsCompose,int WidthCompose,int HeightCompose,bool fKeepAlpha)
{
	RECT rc;
	if (hdcCompose) {
		::SetRect(&rc,0,0,WidthCompose,HeightCompose);
	} else {
		::GetClientRect(m_hwnd,&rc);
	}
	int Width=rc.right;
	int Height=rc.bottom;
	if (Width<1 || Height<1)
		return;

	int ObjHeight=0;
	if (!m_StyleList.empty()) {
		ObjHeight=m_StyleList.front().hbm?m_StyleList.front().PaintRect.bottom:m_StyleList.front().lf.lfHeight;
		ObjHeight=(int)(ObjHeight*(ObjHeight<0?-1:1)*m_ScaleY);
	}
	int VertMult=ObjHeight>m_VertAntiAliasingThreshold?4:ObjHeight>m_StrokeByDilateThreshold?2:1;
	int MultMono=ObjHeight>m_VertAntiAliasingThreshold?4:2;
	int AllocWidth;
	if (!AllocateWorkBitmap(Width,Height*VertMult,Height*MultMono,&AllocWidth))
		return;

	HDC hdc,hdcSrc;
	if (hdcCompose) {
		hdc=nullptr;
		hdcSrc=::CreateCompatibleDC(hdcCompose);
	} else {
		hdc=::GetDC(m_hwnd);
		hdcSrc=::CreateCompatibleDC(hdc);
	}
	HDC hdcMono=::CreateCompatibleDC(hdcSrc);
	HBITMAP hbmOld=static_cast<HBITMAP>(::SelectObject(hdcSrc,m_hbmWork));
	HBITMAP hbmMonoOld=static_cast<HBITMAP>(::SelectObject(hdcMono,m_hbmWorkMono));

	rc.bottom=Height*VertMult;
	DrawUtil::Fill(hdcSrc,&rc,m_crBackColor);
	::ZeroMemory(m_pBitsMono,(AllocWidth*4+31)/32*4 * Height*MultMono);
	rc.bottom=Height;

	if (!m_StyleList.empty()) {
		int rb=rc.bottom*VertMult;
		int rr=rc.right;
		if (m_fHLLeft) DrawLine(hdcSrc,1,rb-1,1,1,2,m_StyleList.front().cr);
		if (m_fHLTop) DrawLine(hdcSrc,1,VertMult,rr-1,VertMult,2*VertMult,m_StyleList.front().cr);
		if (m_fHLRight) DrawLine(hdcSrc,rr-1,1,rr-1,rb-1,2,m_StyleList.front().cr);
		if (m_fHLBottom) DrawLine(hdcSrc,rr-1,rb-VertMult,1,rb-VertMult,2*VertMult,m_StyleList.front().cr);

		rb=rc.bottom*MultMono;
		rr=rc.right*MultMono;
		if (m_fHLLeft) DrawLine(hdcMono,MultMono,rb-1,MultMono,1,2*MultMono,RGB(255,255,255));
		if (m_fHLTop) DrawLine(hdcMono,1,MultMono,rr-1,MultMono,2*MultMono,RGB(255,255,255));
		if (m_fHLRight) DrawLine(hdcMono,rr-MultMono,1,rr-MultMono,rb-1,2*MultMono,RGB(255,255,255));
		if (m_fHLBottom) DrawLine(hdcMono,rr-1,rb-MultMono,1,rb-MultMono,2*MultMono,RGB(255,255,255));
	}

	if (!m_fHideText) {
		int OldBkMode=::SetBkMode(hdcSrc,TRANSPARENT);
		DrawTextList(hdcSrc,1,VertMult,true);
		::SetBkMode(hdcSrc,OldBkMode);

		if (ObjHeight>m_StrokeByDilateThreshold) {
			// 縁取りを描画
			int cWidth=m_StrokeWidth==0?0:(int)(m_StrokeWidth*(m_StrokeWidth<0?-m_ScaleY:1))*2*MultMono/72+2*MultMono;
			HPEN hpen=(HPEN)::CreatePen(PS_SOLID,cWidth,RGB(255,255,255));
			if (hpen) {
				HPEN hpenOld=SelectPen(hdcMono,hpen);
				HBRUSH hbr=(HBRUSH)::GetStockObject(WHITE_BRUSH);
				if (hbr) {
					HBRUSH hbrOld=SelectBrush(hdcMono,hbr);
					OldBkMode=::SetBkMode(hdcMono,TRANSPARENT);
					::BeginPath(hdcMono);
					DrawTextList(hdcMono,MultMono,MultMono,false);
					::EndPath(hdcMono);
					::StrokeAndFillPath(hdcMono);
					::SetBkMode(hdcMono,OldBkMode);
					SelectBrush(hdcMono,hbrOld);
				}
				SelectPen(hdcMono,hpenOld);
				::DeletePen(hpen);
			}
		}
		DrawImageList(hdcSrc,1,VertMult);
	}
	::GdiFlush();

	// 縦方向アンチエイリアス
	if (VertMult==4) {
		ShrinkBitmap(m_pBits,AllocWidth,rc);
	} else if (VertMult==2) {
		ShrinkBitmapHalf(m_pBits,AllocWidth,rc);
	}
	ClearBitmapAlpha((BYTE*)m_pBits,AllocWidth,rc,m_fHideText?0:(BYTE)(m_BackOpacity*255/100));

	// 膨張処理による縁取り
	bool fNeedToDilate=false;
	if (ObjHeight<=m_StrokeByDilateThreshold) {
		LayBitmapToAlpha(m_pBits,AllocWidth,rc,(BYTE)(m_Opacity*255/100),m_crBackColor);
		fNeedToDilate=true;
	} else {
		// 膨張処理のために画像部分だけアルファチャネルに写す
		int x=0;
		std::vector<STYLE_ELEM>::const_iterator it = m_StyleList.begin();
		for (; it!=m_StyleList.end(); ++it) {
			RECT rcImage=rc;
			rcImage.left=(int)(x*m_ScaleX);
			x+=it->Width;
			rcImage.right=(int)(x*m_ScaleX);
			if (it->hbm) {
				LayBitmapToAlpha(m_pBits,AllocWidth,rcImage,(BYTE)(m_Opacity*255/100),m_crBackColor);
				fNeedToDilate=true;
			}
		}
	}
	// 囲い部分は処理しない
	RECT rcInner=rc;
	if (m_fHLLeft) rcInner.left+=2;
	if (m_fHLTop) rcInner.top+=2;
	if (m_fHLRight) rcInner.right-=2;
	if (m_fHLBottom) rcInner.bottom-=2;
	if (fNeedToDilate) {
		for (int i=0; i<((int)(m_StrokeWidth*(m_StrokeWidth<0?-m_ScaleY:1))+36)/72; i++) {
			DilateAlpha(m_pBits,AllocWidth,rcInner,(BYTE)(m_Opacity*255/100));
		}
	}
	SetBitmapOpacity(MultMono,(BYTE*)m_pBits,AllocWidth,rc,(BYTE*)m_pBitsMono,(BYTE)(m_Opacity*255/100));
	SmoothAlpha(m_pBits,AllocWidth,rcInner,m_fHideText?0:(BYTE)(m_BackOpacity*255/100),m_StrokeSmoothLevel-(ObjHeight>m_StrokeByDilateThreshold?2:1));

	if (hdcCompose && fKeepAlpha) {
		for (int i=0; i<Height; i++) {
			::memcpy((BYTE*)pBitsCompose+(Height-i-1)*Width*4,(BYTE*)m_pBits+i*AllocWidth*4,Width*4);
		}
	} else if (hdcCompose) {
		PremultiplyBitmap(m_pBits,AllocWidth,rc);
		ComposeAlpha((BYTE*)pBitsCompose,Width,(BYTE*)m_pBits,AllocWidth,rc);
	} else {
		PremultiplyBitmap(m_pBits,AllocWidth,rc);
		SIZE sz={Width,Height};
		POINT ptSrc={0,0};
		BLENDFUNCTION blend={AC_SRC_OVER,0,255,AC_SRC_ALPHA};
		::UpdateLayeredWindow(m_hwnd,hdc,nullptr,&sz,hdcSrc,&ptSrc,0,&blend,ULW_ALPHA);
	}
	::SelectObject(hdcMono,hbmMonoOld);
	::DeleteDC(hdcMono);

	::SelectObject(hdcSrc,hbmOld);
	::DeleteDC(hdcSrc);
	if (hdc) ::ReleaseDC(m_hwnd,hdc);
}


CPseudoOSD *CPseudoOSD::GetThis(HWND hwnd)
{
	return reinterpret_cast<CPseudoOSD*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CPseudoOSD::WndProc(HWND hwnd,UINT uMsg,
												WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			CPseudoOSD *pThis=static_cast<CPseudoOSD*>(pcs->lpCreateParams);

			pThis->m_hwnd=hwnd;
			::SetWindowLongPtr(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pThis));
		}
		return 0;

	case WM_SIZE:
		{
			CPseudoOSD *pThis=GetThis(hwnd);

			if (pThis->m_fLayeredWindow && ::IsWindowVisible(hwnd))
				pThis->UpdateLayeredWindow();
		}
		return 0;

	case WM_PAINT:
		{
			CPseudoOSD *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			if (!pThis->m_fLayeredWindow)
				pThis->Draw(ps.hdc,ps.rcPaint);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_TIMER:
		{
			CPseudoOSD *pThis=GetThis(hwnd);

			switch (wParam) {
			case TIMER_ID_FLASHING:
				pThis->m_fHideText=!pThis->m_fHideText;
				if (pThis->m_fLayeredWindow) {
					if (::IsWindowVisible(hwnd))
						pThis->UpdateLayeredWindow();
				} else {
					::InvalidateRect(hwnd,nullptr,FALSE);
				}
				break;
			}
		}
		return 0;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MOUSEMOVE:
		{
			CPseudoOSD *pThis=GetThis(hwnd);
			POINT pt;
			RECT rc;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			::MapWindowPoints(hwnd,pThis->m_hwndParent,&pt,1);
			::GetClientRect(pThis->m_hwndParent,&rc);
			if (::PtInRect(&rc,pt))
				return ::SendMessage(pThis->m_hwndParent,uMsg,wParam,MAKELPARAM(pt.x,pt.y));
		}
		return 0;

	case WM_SETCURSOR:
		{
			CPseudoOSD *pThis=GetThis(hwnd);

			return ::SendMessage(pThis->m_hwndParent,uMsg,wParam,lParam);
		}

	case WM_DESTROY:
		{
			CPseudoOSD *pThis=GetThis(hwnd);
			pThis->m_TimerID=0;
			pThis->m_hwnd=nullptr;
		}
		return 0;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}
