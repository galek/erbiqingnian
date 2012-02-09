
#include <direct.h>

#include <cg/cg.h>

#include "gearsPlatform.h"
#include "gearsApplication.h"

#include "render.h"
#include "renderDesc.h"

_NAMESPACE_BEGIN

extern char gShadersDir[];

_IMPLEMENT_SINGLETON(GearPlatform);

static const char *g_windowClassName = "phwndclass";
static const DWORD g_windowStyle     = WS_OVERLAPPEDWINDOW;
static const DWORD g_fullscreenStyle = WS_POPUP;

static void doOnMouseDown(HWND m_hwnd, RendererWindow &window, LPARAM lParam, RendererWindow::MouseButton button)
{
	RECT rect;
	GetClientRect(m_hwnd, &rect);
	const uint32 height = (uint32)(rect.bottom-rect.top);
	window.onMouseDown((uint32)LOWORD(lParam), height-(uint32)HIWORD(lParam), button);
}

static void doOnMouseUp(HWND m_hwnd, RendererWindow &window, LPARAM lParam, RendererWindow::MouseButton button)
{
	RECT rect;
	GetClientRect(m_hwnd, &rect);
	const uint32 height = (uint32)(rect.bottom-rect.top);
	window.onMouseUp((uint32)LOWORD(lParam), height-(uint32)HIWORD(lParam), button);
}

static RendererWindow::KeyCode getKeyCode(WPARAM wParam)
{
	RendererWindow::KeyCode keyCode = RendererWindow::KEY_UNKNOWN;
	const int keyparam = (int)wParam;

	if(keyparam >= 'A' && keyparam <= 'Z') keyCode = (RendererWindow::KeyCode)((keyparam - 'A')+RendererWindow::KEY_A);
	else if(keyparam >= '0' && keyparam <= '9') keyCode = (RendererWindow::KeyCode)((keyparam - '0')+RendererWindow::KEY_0);
	else if(keyparam >= VK_NUMPAD0 && keyparam <= VK_DIVIDE) keyCode = (RendererWindow::KeyCode)((keyparam - VK_NUMPAD0)+RendererWindow::KEY_NUMPAD0);
	else if(keyparam == VK_SHIFT) keyCode = RendererWindow::KEY_SHIFT;
	else if(keyparam == VK_CONTROL) keyCode = RendererWindow::KEY_CONTROL;
	else if(keyparam == VK_SPACE) keyCode = RendererWindow::KEY_SPACE;
	else if(keyparam == VK_ESCAPE) keyCode = RendererWindow::KEY_ESCAPE;
	else if(keyparam == VK_OEM_COMMA) keyCode = RendererWindow::KEY_COMMA;
	else if(keyparam == VK_OEM_2) keyCode = RendererWindow::KEY_DIVIDE;
	else if(keyparam == VK_OEM_MINUS) keyCode = RendererWindow::KEY_SUBTRACT;
	else if(keyparam == VK_OEM_PLUS) keyCode = RendererWindow::KEY_ADD;

	return keyCode;
}

static INT_PTR CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#if defined(RENDERER_64BIT)
	RendererWindow *window = (RendererWindow *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
#else
	RendererWindow *window = (RendererWindow *)LongToPtr(GetWindowLongPtr(hwnd, GWLP_USERDATA));
#endif

	bool customHandle = false;

	if(!customHandle)
	{
		switch(msg)
		{
		case WM_CREATE:
			::UpdateWindow(hwnd);
			break;

		case WM_CLOSE:
			if(window)
			{
				window->close();
			}
			break;

		case WM_SIZE:
			if(window)
			{
				RECT rect;
				GetClientRect(hwnd, &rect);
				uint32 width  = (uint32)(rect.right-rect.left);
				uint32 height = (uint32)(rect.bottom-rect.top);
				window->onResize(width, height);
			}
			break;

		case WM_MOUSEMOVE:
			if(window)
			{
				RECT rect;
				GetClientRect(hwnd, &rect);
				uint32 height = (uint32)(rect.bottom-rect.top);
				window->onMouseMove((uint32)LOWORD(lParam), height-(uint32)HIWORD(lParam));
			}
			break;

		case WM_LBUTTONDOWN:
			if(window) doOnMouseDown(hwnd, *window, lParam, RendererWindow::MOUSE_LEFT);
			break;

		case WM_LBUTTONUP:
			if(window) doOnMouseUp(hwnd, *window, lParam, RendererWindow::MOUSE_LEFT);
			break;

		case WM_RBUTTONDOWN:
			if(window) doOnMouseDown(hwnd, *window, lParam, RendererWindow::MOUSE_RIGHT);
			break;

		case WM_RBUTTONUP:
			if(window) doOnMouseUp(hwnd, *window, lParam, RendererWindow::MOUSE_RIGHT);
			break;

		case WM_KEYDOWN:
			if(window) window->onKeyDown(getKeyCode(wParam));
			break;

		case WM_KEYUP:
			if(window) window->onKeyUp(getKeyCode(wParam));
			break;

		case WM_PAINT:
			ValidateRect(hwnd, 0);
			break;

		default:
			return ::DefWindowProc(hwnd, msg, wParam, lParam);
		}
	}
	return 0; 
}

static ATOM registerWindowClass(HINSTANCE hInstance)
{
	static ATOM atom = 0;
	if(!atom)
	{
		WNDCLASSEX wcex;
		wcex.cbSize         = sizeof(WNDCLASSEX); 
		wcex.style          = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc    = (WNDPROC)windowProc;
		wcex.cbClsExtra     = 0;
		wcex.cbWndExtra     = sizeof(void*);
		wcex.hInstance      = hInstance;
		wcex.hIcon          = ::LoadIcon(hInstance, (LPCTSTR)0);
		wcex.hCursor        = ::LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
		wcex.lpszMenuName   = 0;
		wcex.lpszClassName  = g_windowClassName;
		wcex.hIconSm        = ::LoadIcon(wcex.hInstance, (LPCTSTR)0);
		atom = ::RegisterClassEx(&wcex);
	}
	return atom;
}

//////////////////////////////////////////////////////////////////////////

GearPlatform::GearPlatform( RendererWindow* _app ) :  m_app(_app)
{
	m_sf_app = static_cast<GearApplication*>(m_app);

	m_isHandlingMessages = false;
	m_destroyWindow = false;
	m_hasFocus = true;
	m_hwnd = 0;
}

GearPlatform::~GearPlatform()
{
	ph_assert2(m_hwnd==0, "RendererWindow was not closed before being destroyed.");
	if(m_library) 
	{
		FreeLibrary(m_library);
		m_library = 0;
	}
}

void GearPlatform::correctCurrentDir( void )
{
	char exepath[1024] = {0};
	GetModuleFileNameA(0, exepath, sizeof(exepath));

	if(exepath[0])
	{
		popPathSpec(exepath);
		_chdir(exepath);
	}
}

void GearPlatform::popPathSpec( char *path )
{
	char *ls = 0;
	while(*path)
	{
		if(*path == '\\' || *path == '/') 
		{
			ls = path;
		}
		path++;
	}
	if(ls) 
	{
		*ls = 0;
	}
}

bool GearPlatform::openWindow( uint32& width, uint32& height, 
							  const char* title,bool fullscreen )
{
	bool ok = false;
	ph_assert2(m_hwnd==0, "Attempting to open a window that is already opened");
	if(m_hwnd==0)
	{
		registerWindowClass((HINSTANCE)::GetModuleHandle(0));
		RECT winRect;
		winRect.left   = 0;
		winRect.top    = 0;
		winRect.right  = width;
		winRect.bottom = height;
		DWORD dwstyle  = (fullscreen ? g_fullscreenStyle : g_windowStyle);
		UINT  offset   = fullscreen ? 0 : 50;
		::AdjustWindowRect(&winRect, dwstyle, 0);
		m_hwnd = ::CreateWindowA(g_windowClassName, title, dwstyle,
			offset, offset,
			winRect.right-winRect.left, winRect.bottom-winRect.top,
			0, 0, 0, 0);
		ph_assert2(m_hwnd, "CreateWindow failed");
		if(m_hwnd)
		{
			ok = true;
			ShowWindow(m_hwnd, SW_SHOWNORMAL);
			SetFocus(m_hwnd);              
			SetWindowLongPtr(m_hwnd, GWLP_USERDATA, PtrToLong(m_app));
			m_app->onOpen();
		}
	}

	{
		RAWINPUTDEVICE rawInputDevice;
		rawInputDevice.usUsagePage	= 1;
		rawInputDevice.usUsage		= 6;
		rawInputDevice.dwFlags		= 0;
		rawInputDevice.hwndTarget	= NULL;

		BOOL status = RegisterRawInputDevices(&rawInputDevice, 1, sizeof(rawInputDevice));
		if(status!=TRUE)
		{
			DWORD err = GetLastError();
			printf("%d\n", err);
		}
	}

	return ok;
}

void GearPlatform::update()
{
	ph_assert2(m_hwnd, "Tried to update a window that was not opened.");
	if(m_app->isOpen())
	{
		m_isHandlingMessages = true;
		MSG	msg;

		while(m_app->isOpen() && ::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		m_isHandlingMessages = false;
		if(m_hwnd && m_destroyWindow)
		{
			if(m_app->onClose())
			{
				::DestroyWindow(m_hwnd);
				m_hwnd = 0;
			}
			m_destroyWindow = false;
		}
	}
}

bool GearPlatform::closeWindow()
{
	if(m_hwnd)
	{
		if(m_isHandlingMessages)
		{
			m_destroyWindow = true;
		}
		else if(m_app->onClose())
		{
			::DestroyWindow(m_hwnd);
			m_hwnd = 0;
		}
	}

	return true;
}

bool GearPlatform::hasFocus() const
{
	return m_hasFocus;
}

void GearPlatform::setFocus( bool b )
{
	m_hasFocus = b;
}

bool GearPlatform::isOpen()
{
	if(m_hwnd) 
	{
		return true;
	}
	return false;
}

uint64 GearPlatform::getWindowHandle()
{
	return reinterpret_cast<uint64>(m_hwnd);
}

void GearPlatform::setWindowSize( uint32 width, uint32 height )
{
	bool fullscreen = false;
	ph_assert2(m_hwnd, "Tried to resize a window that was not opened.");
	if(m_hwnd)
	{
		RECT rect;
		::GetWindowRect(m_hwnd, &rect);
		rect.right    = (LONG)(rect.left + width);
		rect.bottom   = (LONG)(rect.top  + height);
		RECT oldrect  = rect;
		DWORD dwstyle = (fullscreen ? g_fullscreenStyle : g_windowStyle);
		::AdjustWindowRect(&rect, dwstyle, 0);
		::MoveWindow(m_hwnd, (int)oldrect.left, (int)oldrect.top, (int)(rect.right-rect.left), (int)(rect.bottom-rect.top), 1);
	}
}

void GearPlatform::getWindowSize( uint32& width, uint32& height )
{
	if(m_hwnd)
	{
		RECT rect;
		GetClientRect(m_hwnd, &rect);
		width  = (uint32)(rect.right  - rect.left);
		height = (uint32)(rect.bottom - rect.top);
	}
}

void GearPlatform::getTitle( char *title, uint32 maxLength ) const
{
	ph_assert2(m_hwnd, "Tried to get the title of a window that was not opened.");
	if(m_hwnd)
	{
		GetWindowTextA(m_hwnd, title, maxLength);
	}
}

void GearPlatform::setTitle( const char *title )
{
	ph_assert2(m_hwnd, "Tried to set the title of a window that was not opened.");
	if(m_hwnd)
	{
		::SetWindowTextA(m_hwnd, title);
	}
}

void GearPlatform::recenterCursor( scalar& deltaMouseX, scalar& deltaMouseY )
{

}

void GearPlatform::showMessage( const char* title, const char* message )
{
	printf("%s: %s\n", title, message);
}

void* GearPlatform::compileProgram( void * context, const char *programPath, 
								   uint64 profile, const char *entry, const char **args )
{
	char fullpath[1024];
	strcpy_s(fullpath, 1024, gShadersDir);
	strcat_s(fullpath, 1024, programPath);
	CGprogram program = cgCreateProgramFromFile(static_cast<CGcontext>(context), CG_SOURCE, fullpath, static_cast<CGprofile>(profile), entry, args);

	return program;
}

void* GearPlatform::initializeD3D9()
{
	m_library   = 0;
	if(m_hwnd)
	{
#if defined(D3D_DEBUG_INFO)
#define D3D9_DLL "d3d9d.dll"
#else
#define D3D9_DLL "d3d9.dll"
#endif
		m_library = LoadLibraryA(D3D9_DLL);
		ph_assert2(m_library, "Could not load " D3D9_DLL ".");
		if(!m_library)
		{
			MessageBoxA(0, "Could not load " D3D9_DLL ". Please install the latest DirectX End User Runtime available at www.microsoft.com/directx.", "Renderer Error.", MB_OK);
		}
#undef D3D9_DLL
		if(m_library)
		{
			typedef IDirect3D9* (WINAPI* LPDIRECT3DCREATE9)(UINT SDKVersion);
			LPDIRECT3DCREATE9 pDirect3DCreate9 = (LPDIRECT3DCREATE9)GetProcAddress(m_library, "Direct3DCreate9");
			ph_assert2(pDirect3DCreate9, "Could not find Direct3DCreate9 function.");
			if(pDirect3DCreate9)
			{
				m_d3d = pDirect3DCreate9(D3D_SDK_VERSION);
			}
		}
	}
	return m_d3d;
}

bool GearPlatform::isD3D9ok()
{
	if(m_library) 
	{
		return true;
	}
	return false;
}

void GearPlatform::initializeCGRuntimeCompiler()
{
	// TODO : cg
}

uint32 GearPlatform::initializeD3D9Display( void * d3dPresentParameters, char* m_deviceName,
										   uint32& width, uint32& height,void * m_d3dDevice_out )
{
	D3DPRESENT_PARAMETERS* m_d3dPresentParams = static_cast<D3DPRESENT_PARAMETERS*>(d3dPresentParameters);

	UINT       adapter    = D3DADAPTER_DEFAULT;
	D3DDEVTYPE deviceType = D3DDEVTYPE_HAL;

	// check to see if fullscreen is requested...
	bool fullscreen = false;
	WINDOWINFO wininfo = {0};
	if(GetWindowInfo(m_hwnd, &wininfo))
	{
		if(wininfo.dwStyle & WS_POPUP)
		{
			fullscreen = true;
		}
	}

	// search for supported adapter mode.
	if(fullscreen)
	{
		RECT rect = {0};
		GetWindowRect(m_hwnd, &rect);
		m_d3dPresentParams->BackBufferFormat = D3DFMT_X8R8G8B8;
		m_d3dPresentParams->BackBufferWidth  = rect.right-rect.left;
		m_d3dPresentParams->BackBufferHeight = rect.bottom-rect.top;

		bool foundAdapterMode = false;
		const UINT numAdapterModes = m_d3d->GetAdapterModeCount(0, m_d3dPresentParams->BackBufferFormat);
		for(UINT i=0; i<numAdapterModes; i++)
		{
			D3DDISPLAYMODE mode = {0};
			m_d3d->EnumAdapterModes(0, m_d3dPresentParams->BackBufferFormat, i, &mode);
			if(mode.Width       == m_d3dPresentParams->BackBufferWidth  &&
				mode.Height      == m_d3dPresentParams->BackBufferHeight &&
				mode.RefreshRate >  m_d3dPresentParams->FullScreen_RefreshRateInHz)
			{
				m_d3dPresentParams->FullScreen_RefreshRateInHz = mode.RefreshRate;
				foundAdapterMode = true;
			}
		}
		ph_assert2(foundAdapterMode, "Unable to find supported fullscreen Adapter Mode.");
		if(!foundAdapterMode) fullscreen = false;
	}

	// enable fullscreen mode.
	if(fullscreen)
	{
		m_d3dPresentParams->Windowed = 0;
	}

#if defined(RENDERER_ENABLE_NVPERFHUD)
	// NvPerfHud Support.
	UINT numAdapters = m_d3d->GetAdapterCount();
	for(UINT i=0; i<numAdapters; i++)
	{
		D3DADAPTER_IDENTIFIER9 identifier;
		m_d3d->GetAdapterIdentifier(i, 0, &identifier);
		if(strstr(identifier.Description, "PerfHUD"))
		{
			adapter    = i;
			deviceType = D3DDEVTYPE_REF;
			break;
		}
	}
#endif

	D3DADAPTER_IDENTIFIER9 adapterIdentifier;
	m_d3d->GetAdapterIdentifier(adapter, 0, &adapterIdentifier);
	strncpy_s(m_deviceName, 256, adapterIdentifier.Description, sizeof(m_deviceName));

	HRESULT res = m_d3d->CreateDevice( adapter, deviceType,
		m_hwnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		m_d3dPresentParams, &m_d3dDevice);
	*(static_cast<IDirect3DDevice9**>(m_d3dDevice_out)) = m_d3dDevice;
	return res;
}

uint32 GearPlatform::D3D9Present()
{
	return m_d3dDevice->Present(0, 0, m_hwnd, 0);
}

void GearPlatform::D3D9BlockUntilNotBusy( void * resource )
{

}

void GearPlatform::D3D9DeviceBlockUntilIdle()
{

}

uint64 GearPlatform::getD3D9TextureFormat( RendererTexture2D::Format format )
{
	D3DFORMAT d3dFormat = D3DFMT_UNKNOWN;
	switch(format)
	{
	case RendererTexture2D::FORMAT_B8G8R8A8: d3dFormat = D3DFMT_A8R8G8B8; break;
	case RendererTexture2D::FORMAT_A8:       d3dFormat = D3DFMT_A8;       break;
	case RendererTexture2D::FORMAT_R32F:     d3dFormat = D3DFMT_R32F;     break;
	case RendererTexture2D::FORMAT_DXT1:     d3dFormat = D3DFMT_DXT1;     break;
	case RendererTexture2D::FORMAT_DXT3:     d3dFormat = D3DFMT_DXT3;     break;
	case RendererTexture2D::FORMAT_DXT5:     d3dFormat = D3DFMT_DXT5;     break;
	case RendererTexture2D::FORMAT_D16:      d3dFormat = D3DFMT_D16;      break;
	case RendererTexture2D::FORMAT_D24S8:    d3dFormat = D3DFMT_D24S8;    break;
	}
	return static_cast<uint64>(d3dFormat);
}

void GearPlatform::postRendererRelease()
{

}

void GearPlatform::preRendererSetup()
{

}

void GearPlatform::postRendererSetup()
{
	if(!m_sf_app->getRenderer())
	{
		exit(1);
	}
	char windowTitle[1024] = {0};
	m_app->getTitle(windowTitle, 1024);
	strcat_s(windowTitle, 1024, " : ");
	strcat_s(windowTitle, 1024, Renderer::getDriverTypeName(
		m_sf_app->getRenderer()->getDriverType()));
	m_app->setTitle(windowTitle);
}

void GearPlatform::setupRendererDescription( RendererDesc& renDesc )
{
	renDesc.driver = Renderer::DRIVER_DIRECT3D9;
	renDesc.windowHandle = reinterpret_cast<uint64>(m_hwnd);
}

void GearPlatform::doInput()
{
	processGamepads();
}

void GearPlatform::processGamepads()
{
	// ÔÝÊ±²»¹Ü
}

_NAMESPACE_END