
#include "InfoBar.h"

#include "EditorTool.h"
#include "Settings.h"


//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CInfoBar, CDialog)
	ON_BN_CLICKED(IDC_VECTOR_LOCK, OnVectorLock)
	ON_BN_CLICKED(ID_LOCK_SELECTION, OnLockSelection)
	ON_UPDATE_COMMAND_UI(ID_LOCK_SELECTION, OnUpdateLockSelection)
	ON_EN_UPDATE(IDC_MOVE_SPEED, OnUpdateMoveSpeed)
	ON_EN_UPDATE(IDC_POSX,OnVectorUpdateX)
	ON_EN_UPDATE(IDC_POSY,OnVectorUpdateY)
	ON_EN_UPDATE(IDC_POSZ,OnVectorUpdateZ)

	ON_CONTROL(EN_BEGIN_DRAG, IDC_POSX, OnBeginVectorUpdate)
	ON_CONTROL(EN_BEGIN_DRAG, IDC_POSY, OnBeginVectorUpdate)
	ON_CONTROL(EN_BEGIN_DRAG, IDC_POSZ, OnBeginVectorUpdate)
	ON_CONTROL(EN_END_DRAG, IDC_POSX, OnEndVectorUpdate)
	ON_CONTROL(EN_END_DRAG, IDC_POSY, OnEndVectorUpdate)
	ON_CONTROL(EN_END_DRAG, IDC_POSZ, OnEndVectorUpdate)

	ON_BN_CLICKED(IDC_TERRAIN_COLLISION, OnBnClickedTerrainCollision)
	ON_BN_CLICKED(IDC_GOTOPOSITION, OnBnClickedGotoPosition)
	ON_BN_CLICKED(IDC_SPEED_01, OnBnClickedSpeed01)
	ON_BN_CLICKED(IDC_SPEED_1, OnBnClickedSpeed1)
	ON_BN_CLICKED(IDC_SPEED_10, OnBnClickedSpeed10)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInfoBar dialog
CInfoBar::CInfoBar()
{
	//{{AFX_DATA_INIT(CInfoBar)
	//}}AFX_DATA_INIT

	m_enabledVector = false;
	m_bVectorLock = false;
	m_prevEditMode = 0;
	m_bSelectionLocked = false;
	m_editTool = 0;
	m_bDragMode = false;
	m_prevMoveSpeed = 0;
	m_currValue = Float3(-111,+222,-333);
}

//////////////////////////////////////////////////////////////////////////
CInfoBar::~CInfoBar()
{
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInfoBar)
	DDX_Control(pDX, IDC_VECTOR_LOCK, m_vectorLock);
	DDX_Control(pDX, ID_LOCK_SELECTION, m_lockSelection);
	DDX_Control(pDX, IDC_TERRAIN_COLLISION, m_terrainCollision);
	DDX_Control(pDX, IDC_VECTOR_LOCK, m_vectorLock);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_GOTOPOSITION, m_gotoPos);
	DDX_Control(pDX, IDC_SPEED_01, m_speed_01);
	DDX_Control(pDX, IDC_SPEED_1, m_speed_1);
	DDX_Control(pDX, IDC_SPEED_10, m_speed_10);
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnVectorUpdateX()
{
	if (m_bVectorLock)
	{
		Float3 v = GetVector();
		SetVector( Float3(v.x,v.x,v.x) );
	}
	OnVectorUpdate(false);
}

void CInfoBar::OnVectorUpdateY()
{
	if (m_bVectorLock)
	{
		Float3 v = GetVector();
		SetVector( Float3(v.y,v.y,v.y) );
	}
	OnVectorUpdate(false);
}

void CInfoBar::OnVectorUpdateZ()
{
	if (m_bVectorLock)
	{
		Float3 v = GetVector();
		SetVector( Float3(v.z,v.z,v.z) );
	}
	OnVectorUpdate(false);
}

void CInfoBar::OnVectorUpdate( bool followTerrain )
{
	
}

void CInfoBar::IdleUpdate()
{
	
}

inline double Round( double fVal, double fStep )
{
	return MathUtil::IntRound(fVal / fStep) * fStep;
}

void CInfoBar::SetVector( const Float3 &v )
{
	if (!m_bDragMode)
		m_lastValue = m_currValue;

	if (m_currValue != v)
	{
		static const double fPREC = 1e-4;
		m_posCtrl[0].SetValue( Round(v.x, fPREC) );
		m_posCtrl[1].SetValue( Round(v.y, fPREC) );
		m_posCtrl[2].SetValue( Round(v.z, fPREC) );
		m_currValue = v;
	}
}
	
Float3 CInfoBar::GetVector()
{
	Float3 v;
	v.x = m_posCtrl[0].GetValue();
	v.y = m_posCtrl[1].GetValue();
	v.z = m_posCtrl[2].GetValue();
	m_currValue = v;
	return v;
}

void CInfoBar::EnableVector( bool enable )
{
	if (m_enabledVector != enable)
	{
		m_enabledVector = enable;
		m_posCtrl[0].EnableWindow(enable);
		m_posCtrl[1].EnableWindow(enable);
		m_posCtrl[2].EnableWindow(enable);

		m_vectorLock.EnableWindow(enable);
	}
}

void CInfoBar::SetVectorLock( bool bVectorLock )
{
	m_bVectorLock = bVectorLock;
	m_vectorLock.SetCheck( bVectorLock ? 1 : 0 );
	//GetIEditor()->SetAxisVectorLock( bVectorLock );
}

void CInfoBar::SetVectorRange( float min,float max )
{
	m_posCtrl[0].SetRange( min,max );
	m_posCtrl[1].SetRange( min,max );
	m_posCtrl[2].SetRange( min,max );
}


void CInfoBar::OnVectorLock() 
{
	m_vectorLock.SetPushedBkColor(::GetSysColor(COLOR_HIGHLIGHT)); // Getting set by the current skin
	m_bVectorLock = !m_vectorLock.GetCheck();
	m_vectorLock.SetCheck(m_bVectorLock);
}

void CInfoBar::OnLockSelection() 
{
	BOOL locked = !m_lockSelection.GetCheck();
	m_lockSelection.SetCheck(locked);
	m_bSelectionLocked = (locked)?true:false;
	//GetIEditor()->LockSelection( m_bSelectionLocked );
}

void CInfoBar::OnUpdateLockSelection(CCmdUI* pCmdUI)
{
// 	if (GetIEditor()->GetSelection()->IsEmpty())
// 		pCmdUI->Enable(FALSE);
// 	else
// 		pCmdUI->Enable(TRUE);
	//bool bSelLocked = GetIEditor()->IsSelectionLocked();
	//pCmdUI->SetCheck( (bSelLocked)?1:0 );

	pCmdUI->Enable(FALSE);
	pCmdUI->SetCheck(FALSE);
}

void CInfoBar::OnUpdateMoveSpeed() 
{
	EditorSettings::Get().cameraMoveSpeed = m_moveSpeed.GetValue();
}

BOOL CInfoBar::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_posCtrl[0].Create( this,IDC_POSX );
	m_posCtrl[1].Create( this,IDC_POSY );
	m_posCtrl[2].Create( this,IDC_POSZ );

	m_posCtrl[0].EnableWindow(FALSE);
	m_posCtrl[1].EnableWindow(FALSE);
	m_posCtrl[2].EnableWindow(FALSE);

	m_moveSpeed.Create( this,IDC_MOVE_SPEED );
	m_moveSpeed.SetRange( 0.01,100 );

	GetDlgItem(IDC_INFO_WH)->SetFont( CFont::FromHandle( (HFONT)EditorSettings::Get().Gui.hSystemFont) );
	SetDlgItemText(IDC_INFO_WH,"");

	m_vectorLock.SetIcon( MAKEINTRESOURCE(IDI_LOCK) );
	m_terrainCollision.SetIcon( MAKEINTRESOURCE(IDI_TERRAIN_COLLISION) );

	m_moveSpeed.SetValue( EditorSettings::Get().cameraMoveSpeed );
	
	
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnBeginVectorUpdate()
{
	m_lastValue = GetVector();
	m_bDragMode = true;
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnEndVectorUpdate()
{
	m_bDragMode = false;
}

void CInfoBar::OnBnClickedTerrainCollision()
{
	m_terrainCollision.SetPushedBkColor(::GetSysColor(COLOR_HIGHLIGHT));
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnBnClickedGotoPosition()
{
	//AfxGetMainWnd()->SendMessage( WM_COMMAND,MAKEWPARAM(ID_DISPLAY_GOTOPOSITION,0),0 );
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnBnClickedSpeed01()
{
	m_moveSpeed.SetValue(0.1);
	OnUpdateMoveSpeed();
	AfxGetMainWnd()->SetFocus();
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnBnClickedSpeed1()
{
	m_moveSpeed.SetValue(1);
	OnUpdateMoveSpeed();
	AfxGetMainWnd()->SetFocus();
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnBnClickedSpeed10()
{
	m_moveSpeed.SetValue(10);
	OnUpdateMoveSpeed();
	AfxGetMainWnd()->SetFocus();
}