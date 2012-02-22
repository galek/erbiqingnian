
/********************************************************************
	日期:		2012年2月22日
	文件名: 	InfoBar.h
	创建者:		王延兴
	描述:		视图下方的信息栏，包括选中物体的位置等信息	
*********************************************************************/

#pragma once

#include "ColorCheckBox.h"
#include "NumberCtrl.h"

class CEditTool;
class CInfoBar : public CDialog
{
	// Construction
public:
	CInfoBar();
	~CInfoBar();

	// Dialog Data
	enum { IDD = IDD_INFO_BAR };

	CColorCheckBox 	m_vectorLock;
	CColorCheckBox 	m_lockSelection;
	CColorCheckBox 	m_terrainCollision;
	CColorCheckBox 	m_physicsBtn;
	CCustomButton 	m_gotoPos;
	CCustomButton 	m_speed_01;
	CCustomButton 	m_speed_1;
	CCustomButton 	m_speed_10;

protected:

	virtual void DoDataExchange(CDataExchange* pDX); 

protected:

	void			IdleUpdate();

	virtual void 	OnOK() {};

	virtual void 	OnCancel() {};

	void 			OnVectorUpdate( bool followTerrain );

	void 			OnVectorUpdateX();
	void 			OnVectorUpdateY();
	void 			OnVectorUpdateZ();

	void 			SetVector( const Float3 &v );
	void 			SetVectorRange( float min,float max );
	Float3			GetVector();
	void 			EnableVector( bool enable );
	void 			SetVectorLock( bool bVectorLock );

	afx_msg void 	OnBnClickedGotoPosition();
	afx_msg void 	OnBnClickedSpeed01();
	afx_msg void 	OnBnClickedSpeed1();
	afx_msg void 	OnBnClickedSpeed10();
	afx_msg void 	OnBeginVectorUpdate();
	afx_msg void 	OnEndVectorUpdate();

	afx_msg void 	OnVectorLock();
	afx_msg void 	OnLockSelection();
	afx_msg void 	OnUpdateLockSelection(CCmdUI* pCmdUI);
	afx_msg void 	OnUpdateMoveSpeed();
	afx_msg void 	OnBnClickedTerrainCollision();

	virtual BOOL	OnInitDialog();

	DECLARE_MESSAGE_MAP()

	CNumberCtrl		m_posCtrl[3];
	bool			m_enabledVector;

	CNumberCtrl		m_moveSpeed;

	float			m_width,m_height;

	int				m_prevEditMode;
	int				m_numSelected;
	float			m_prevMoveSpeed;

	bool			m_bVectorLock;
	bool			m_bSelectionLocked;

	bool			m_bDragMode;
	CString			m_sLastText;
	
	CEditTool*		m_editTool;
	Float3			m_lastValue;
	Float3			m_currValue;
};
