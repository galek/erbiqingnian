
/********************************************************************
	日期:		2012年2月20日
	文件名: 	RollupBar.h
	创建者:		王延兴
	描述:		RollupBar控件	
*********************************************************************/

#pragma once

#include "RollupCtrl.h"

_NAMESPACE_BEGIN

class CRollupBar : public CWnd
{
public:
	CRollupBar();

	virtual						~CRollupBar();

	void						SetRollUpCtrl( int i,CRollupCtrl *pCtrl,const char *sTooltip );

	void						Select( int num );

	int							GetSelection() { return m_selectedCtrl; };

	CRollupCtrl*				GetCurrentCtrl();

protected:

    afx_msg int					OnCreate(LPCREATESTRUCT lpCreateStruct);

    afx_msg void				OnSize(UINT nType, int cx, int cy);

	afx_msg void				OnTabSelect(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()

	CTabCtrl					m_tabCtrl;

	CImageList					m_tabImageList;

	std::vector<CRollupCtrl*>	m_controls;

	int							m_selectedCtrl;
};

_NAMESPACE_END