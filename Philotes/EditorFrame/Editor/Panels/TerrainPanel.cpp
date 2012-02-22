
#include "TerrainPanel.h"
#include "EnvironmentEditor.h"

CTerrainPanel::CTerrainPanel(CWnd* pParent /*=NULL*/)
: CDialog(CTerrainPanel::IDD, pParent)
{
	Create(MAKEINTRESOURCE(IDD),pParent);
}

void CTerrainPanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TEXTURE,		m_textureBtn);
	DDX_Control(pDX, IDC_ENVIRONMENT,	m_environmentBtn);
	DDX_Control(pDX, IDC_VEGETATION,	m_vegetationBtn);
	DDX_Control(pDX, IDC_MODIFY,		m_modifyBtn);
	DDX_Control(pDX, IDC_MINIMAP,		m_minimapBtn);
}

BEGIN_MESSAGE_MAP(CTerrainPanel, CDialog)
END_MESSAGE_MAP()

BOOL CTerrainPanel::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	return CDialog::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CTerrainPanel::OnIdleUpdate()
{
	
}

BOOL CTerrainPanel::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
// 	m_modifyBtn.SetToolClass( RUNTIME_CLASS(CTerrainModifyTool) );
// 	m_vegetationBtn.SetToolClass( RUNTIME_CLASS(CVegetationTool) );
 	m_environmentBtn.SetToolClass( RUNTIME_CLASS(CEnvironmentTool) );
// 	m_textureBtn.SetToolClass( RUNTIME_CLASS(CTerrainTexturePainter) );
// 	m_minimapBtn.SetToolClass( RUNTIME_CLASS(CTerrainMiniMapTool) );
	
	return TRUE;
}
