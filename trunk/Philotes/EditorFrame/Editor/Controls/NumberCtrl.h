
#pragma once

#include "NumberCtrlEdit.h"

_NAMESPACE_BEGIN

#define EN_BEGIN_DRAG EN_CHANGE+0x1000
#define EN_END_DRAG EN_CHANGE+0x1001

class CNumberCtrl : public CWnd
{
public:
	typedef Functor1<CNumberCtrl*>	UpdateCallback;

	enum Flags 
	{
		LEFTARROW		= 0x01,
		NOBORDER		= 0x02,
		LEFTALIGN		= 0x04,
		CENTER_ALIGN	= 0x08,
	};

	CNumberCtrl();

	virtual 		~CNumberCtrl();

public:

	void 			Create( CWnd* parentWnd,UINT ctrlID,int nFlags=0 );

	void 			Create( CWnd* parentWnd,CRect &rc,UINT nID,int nFlags=0 );

	void 			EnableUndo( const CString& undoText );

	void 			SetUpdateCallback( const UpdateCallback &cb ) { m_updateCallback = cb; };

	void 			SetMultiplier( double fMultiplier );

	void			SetValue( double val );

	double			GetValue() const;

	CString 		GetValueAsString() const;

	void			SetStep( double step );

	double			GetStep() const;

	void			SetRange( double min,double max );

	void			SetInteger( bool enable );

	void			SetLeftAlign( bool left );

	void			SetInternalPrecision( int iniDigits );

	void			SetFont( CFont* pFont,BOOL bRedraw = TRUE);

	void			SetFloatFormat( const char* fmt);

protected:
	void			DrawButtons( CDC &dc );
	
	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);

	afx_msg void 	OnPaint();

	afx_msg void 	OnMouseMove(UINT nFlags, CPoint point);

	afx_msg void 	OnLButtonDown(UINT nFlags, CPoint point);

	afx_msg void 	OnLButtonUp(UINT nFlags, CPoint point);

	afx_msg void 	OnEnable(BOOL bEnable);

	afx_msg void 	OnSetFocus(CWnd* pOldWnd);

	afx_msg void 	OnEditSetFocus();

	afx_msg void 	OnEditKillFocus();
	
	DECLARE_MESSAGE_MAP()

	virtual BOOL	OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

	void			GetBtnRect( int btn,CRect &rc );

	int				GetBtn( CPoint point );

	void 			SetBtnStatus( int s );

	void 			NotifyUpdate( bool tracking );

	void 			OnEditChanged();

	void			SetInternalValue( double val );

	double			GetInternalValue() const;

	CNumberCtrlEdit m_edit;

	int				m_nFlags;
	double			m_step;
	double			m_min,m_max;
	mutable double	m_value;
	double			m_lastUpdateValue;
	double			m_multiplier;
	int				m_btnStatus; // 0，无按键按下 1，Up按键按下 2，Down按键按下
	int				m_btnWidth;
	CPoint			m_mousePos;
	bool			m_draggin;
	HICON			m_upArrow,m_downArrow;
	HCURSOR			m_upDownCursor;
	bool			m_enabled;
	bool			m_noNotify;
	bool			m_integer;

	bool 			m_bUndoEnabled;
	bool 			m_bDragged;
	bool 			m_bLocked;
	bool 			m_bInitialized;

	CString 		m_undoText;
	CString 		m_fmt;

	UpdateCallback m_updateCallback;
	
	static int CalculateDecimalPlaces( double infNumber, int iniMaxPlaces );

public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

_NAMESPACE_END