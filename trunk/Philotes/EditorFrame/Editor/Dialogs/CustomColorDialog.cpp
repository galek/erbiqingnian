
#include "CustomColorDialog.h"

#include <ColorDlg.h>

_NAMESPACE_BEGIN

IMPLEMENT_DYNAMIC(CCustomColorDialog, CColorDialog)

CCustomColorDialog::CCustomColorDialog(COLORREF clrInit, DWORD dwFlags, CWnd* pParentWnd) :
CColorDialog(clrInit, dwFlags, pParentWnd)
{
	m_cc.Flags |= CC_ENABLETEMPLATE|CC_FULLOPEN|CC_SHOWHELP;
	m_cc.hInstance = (HWND)AfxGetInstanceHandle();
	m_cc.lpTemplateName = MAKEINTRESOURCE(IDD_CHOOSE_COLOR);
	//MCC_ENABLETEMPLATE

	m_callback = 0;

	m_bPickMode = false;
	m_pickerCusror = ::LoadCursor(AfxGetResourceHandle(),MAKEINTRESOURCE(IDC_COLOR_PICKER));
}

CCustomColorDialog::~CCustomColorDialog()
{
	DeleteObject(m_pickerCusror);
}

BEGIN_MESSAGE_MAP(CCustomColorDialog, CColorDialog)
	//{{AFX_MSG_MAP(CCustomColorDialog)
	ON_BN_CLICKED(IDC_PICK_COLOR, OnPickColor)
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CCustomColorDialog::OnPickColor() 
{
	PickMode( m_pickColor.GetCheck()==1 );
}

BOOL CCustomColorDialog::OnInitDialog() 
{
	CColorDialog::OnInitDialog();

	m_pickColor.SubclassDlgItem( IDC_PICK_COLOR,this );

	CBitmap bmp;
	m_pickColor.SetBitmap( ::LoadBitmap(AfxGetResourceHandle(),MAKEINTRESOURCE(IDB_PICK_COLOR)) );

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
void CCustomColorDialog::OnMouseMove(UINT nFlags, CPoint point) 
{
	CColorDialog::OnMouseMove(nFlags, point);

	int r = GetDlgItemInt(COLOR_RED);
	int g = GetDlgItemInt(COLOR_GREEN);
	int b = GetDlgItemInt(COLOR_BLUE);

	COLORREF col(RGB(r,g,b));
	if (col != m_cc.rgbResult)
	{
		OnColorChange(col);
	}
}

//////////////////////////////////////////////////////////////////////////
void CCustomColorDialog::OnLButtonDown(UINT nFlags, CPoint point) 
{
	if (m_bPickMode)
	{
		CDC dcScreen;
		CDC dc;
		CBitmap bmp;
		dcScreen.CreateDC("DISPLAY", NULL, NULL, NULL); 
		dc.CreateCompatibleDC( &dcScreen );

		int width = GetDeviceCaps( dcScreen,HORZRES);
		int height = GetDeviceCaps( dcScreen,VERTRES);
		bmp.CreateCompatibleBitmap( &dcScreen,width,height );
		if (bmp.m_hObject)
		{
			dc.SelectObject(bmp);

			dc.BitBlt( 0,0,width,height, &dcScreen,	0,0, SRCCOPY );

			UINT *pImage = new UINT[width*height*4];

			BITMAPINFO bi;
			ZeroMemory(&bi,sizeof(BITMAPINFO));
			bi.bmiHeader.biSize = sizeof(bi);
			bi.bmiHeader.biWidth = width;
			bi.bmiHeader.biHeight = -height;
			bi.bmiHeader.biBitCount = 32;
			bi.bmiHeader.biPlanes = 1;
			bi.bmiHeader.biCompression = BI_RGB;
			bi.bmiHeader.biXPelsPerMeter = width;
			bi.bmiHeader.biYPelsPerMeter = height;

			GetDIBits( dc,bmp,0,height,pImage,&bi,DIB_RGB_COLORS );

			CPoint p;
			GetCursorPos( &p );

			UINT c = pImage[p.y*width + p.x];
			COLORREF col = c;

			SetCurrentColor( RGB(GetBValue(col),GetGValue(col),GetRValue(col)) );
			OnColorChange( RGB(GetBValue(col),GetGValue(col),GetRValue(col)) );

			delete []pImage;
		}
		bmp.DeleteObject();
		dc.DeleteDC();
		dcScreen.DeleteDC();

		PickMode(false);
		return;
	}

	CColorDialog::OnLButtonDown(nFlags, point);

	int r = GetDlgItemInt(COLOR_RED);
	int g = GetDlgItemInt(COLOR_GREEN);
	int b = GetDlgItemInt(COLOR_BLUE);
	COLORREF col(RGB(r,g,b));
	if (col != m_cc.rgbResult)
	{
		OnColorChange(col);
	}
}

void CCustomColorDialog::PickMode( bool bEnable )
{
	if (m_bPickMode != bEnable)
	{
		m_bPickMode = bEnable;
		if (m_bPickMode)
		{
			SetCapture();
			SetCursor( m_pickerCusror );
			m_pickColor.SetCheck(1);
		}
		else
		{
			m_pickColor.SetCheck(0);
			ReleaseCapture();
		}
	}
}

int CCustomColorDialog::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CColorDialog::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
void CCustomColorDialog::OnColorChange( COLORREF col )
{
	m_cc.rgbResult = col;
	if (m_callback)
	{
		m_callback(col);
	}
}

_NAMESPACE_END