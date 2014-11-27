// TVTestのVMR9/EVRレンダラに対するOSD合成管理クラス ver.1 (2013-01-24)
// 改変流用自由(ただしプラグイン間の互換をなるべく維持してください)
#ifndef INCLUDE_OSD_COMPOSITOR_H
#define INCLUDE_OSD_COMPOSITOR_H

#include <DShow.h>
#include <d3d9.h>

#ifndef OSD_COMPOSITOR_VERSION
#define OSD_COMPOSITOR_VERSION 1
#endif

class COsdCompositor
{
public:
    COsdCompositor();
    ~COsdCompositor();
    static HWND FindHandle();
    bool SetContainerWindow(HWND hwndContainer);
    int AddTexture(HBITMAP hbm, int Left, int Top, bool fShow, int Group);
    bool DeleteTexture(int ID, int Group);
    bool ShowTexture(bool fShow, int ID, int Group);
    bool GetSurfaceRect(RECT *pRect);
    bool UpdateSurface();
#if OSD_COMPOSITOR_VERSION >= 1
    typedef BOOL (CALLBACK *UpdateCallbackFunc)(void *pBits, const RECT *pSurfaceRect, int Pitch, void *pClientData);
    bool SetUpdateCallback(UpdateCallbackFunc Callback, void *pClientData = NULL, bool fTop = false);
    int GetVersion();
#endif
    bool Initialize(bool fSetHook);
    void Uninitialize();
    void OnFilterGraphInitialized(IGraphBuilder *pGraphBuilder);
    void OnFilterGraphFinalize(IGraphBuilder *pGraphBuilder);
private:
    struct TEXTURE_PARAM {
        int nSize;
        int ID;
        int Left;
        int Top;
        HBITMAP hbm;
    };
    struct TEXTURE {
        int ID;
        int Left;
        int Top;
        void *pBits;
        int Width;
        int Height;
    };
#if OSD_COMPOSITOR_VERSION >= 1
    struct SET_UPDATE_CALLBACK_PARAM {
        int nSize;
        int Flags;
        UpdateCallbackFunc Callback;
        void *pClientData;
    };
#endif
    enum RENDERER_TYPE { RT_VMR9, RT_EVR, };
    typedef HRESULT (WINAPI CoCreateInstanceFunc)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID FAR*);
    typedef IDirect3D9 *(WINAPI Direct3DCreate9Func)(UINT);

    LRESULT SendMessageToHandle(UINT Msg, WPARAM wParam, LPARAM lParam) const;
    static bool Hook(CoCreateInstanceFunc *pfnNew, CoCreateInstanceFunc *pfnOld);
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void ComposeAlpha(DWORD* __restrict pBitsDest, int PitchDest, int WidthDest, int HeightDest,
                             const DWORD* __restrict pBits, int Pitch, int Width, int Height, int Left, int Top);
    static bool GetVideoPosition(RECT *pRect, IBaseFilter *pRenderer, RENDERER_TYPE RendererType);
    static bool SetAlphaBitmap(IBaseFilter *pRenderer, RENDERER_TYPE RendererType, IDirect3DSurface9 *pD3DS9, const NORMALIZEDRECT *pNrc);
    bool CreateDevice();
    void ReleaseDevice();
    bool SetupSurface(int VideoWidth, int VideoHeight, RECT *pSurfaceRect);
    static HRESULT WINAPI CoCreateInstanceHook(
        REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID FAR* ppv);

    HWND m_hwnd;
    HWND m_hwndContainer;
    HMODULE m_hOle32;
    CoCreateInstanceFunc *m_pfnCoCreateInstance;
    IBaseFilter *m_pRenderer;
    RENDERER_TYPE m_RendererType;
    HMODULE m_hD3D9;
    IDirect3D9 *m_pD3D9;
    IDirect3DDevice9 *m_pD3DD9;
    IDirect3DSurface9 *m_pD3DS9;
    TEXTURE m_TxList[256];
    int m_TxListLen;
    int m_TxCount;
    struct { int ID, Group; } m_GroupList[256];
    int m_GroupListLen;
#if OSD_COMPOSITOR_VERSION >= 1
    struct { UpdateCallbackFunc Callback; void *pClientData; bool fTop; } m_CallbackList[64];
    int m_CallbackListLen;
    UpdateCallbackFunc m_LocCallback;
#endif
};

#endif // INCLUDE_OSD_COMPOSITOR_H
