
#include "EnvironmentPanel.h"
#include "EditorRoot.h"
#include "../EditorDoc.h"

#define HEIGHT_OFFSET 4

_NAMESPACE_BEGIN

CEnvironmentPanel::CEnvironmentPanel(CWnd* pParent /*=NULL*/)
: CDialog(CEnvironmentPanel::IDD, pParent)
{
	Create( IDD,pParent );
}

CEnvironmentPanel::~CEnvironmentPanel()
{
}

void CEnvironmentPanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_APPLY, m_applyBtn);
}


BEGIN_MESSAGE_MAP(CEnvironmentPanel, CDialog)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_APPLY, OnBnClickedApply)
END_MESSAGE_MAP()


void CEnvironmentPanel::OnDestroy() 
{
	CDialog::OnDestroy();
}

//////////////////////////////////////////////////////////////////////////
BOOL CEnvironmentPanel::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_node = CEditorRoot::Get().GetDocument()->GetEnvironmentTemplate();

 	if (m_node)
 	{	
 		CRect rc;
 		m_wndProps.Create( WS_VISIBLE|WS_CHILD|WS_BORDER,rc,this );
 		m_wndProps.CreateItems( m_node );
 		m_wndProps.SetUpdateCallback( functor(*this,&CEnvironmentPanel::OnPropertyChanged) );
 		m_wndProps.ExpandAll();
 
 		GetClientRect( rc );
 		int h = m_wndProps.GetVisibleHeight();
 		SetWindowPos( NULL,0,0,rc.right,h+HEIGHT_OFFSET*2 + 34,SWP_NOMOVE );
 
 		GetClientRect( rc );
 		CRect rcb;
 		m_applyBtn.GetWindowRect( rcb );
 		ScreenToClient( rcb );
 		m_applyBtn.SetWindowPos( NULL,rcb.left,rc.bottom-28,0,0,SWP_NOSIZE );
 	}

	return TRUE; 
}

//////////////////////////////////////////////////////////////////////////
void CEnvironmentPanel::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);

	if (m_wndProps.m_hWnd)
	{
		int h = m_wndProps.GetVisibleHeight();
		CRect rc( 2,HEIGHT_OFFSET,cx-2,h+7 );
		m_wndProps.MoveWindow( rc,TRUE );
	}
}

//////////////////////////////////////////////////////////////////////////
void CEnvironmentPanel::OnPropertyChanged( XmlNodeRef node )
{
	
}

void CEnvironmentPanel::OnBnClickedApply()
{

}

_NAMESPACE_END