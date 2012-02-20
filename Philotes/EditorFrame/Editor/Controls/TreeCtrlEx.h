
/********************************************************************
	日期:		2012年2月20日
	文件名: 	TreeCtrlEx.h
	创建者:		王延兴
	描述:		扩展树形控件	
*********************************************************************/

#pragma once

class CTreeCtrlEx : public CTreeCtrl
{
	DECLARE_DYNAMIC(CTreeCtrlEx)

public:
	CTreeCtrlEx();
	virtual ~CTreeCtrlEx();
	
	HTREEITEM GetNextItem( HTREEITEM hItem, UINT nCode );

	HTREEITEM GetNextItem( HTREEITEM hItem);

	HTREEITEM FindNextItem(TV_ITEM* pItem, HTREEITEM hItem);

	HTREEITEM CopyItem( HTREEITEM hItem, HTREEITEM htiNewParent, HTREEITEM htiAfter=TVI_LAST );

	HTREEITEM CopyBranch( HTREEITEM htiBranch, HTREEITEM htiNewParent,HTREEITEM htiAfter=TVI_LAST );

	void SetOnlyLeafsDrag( bool bEnable ) { m_bOnlyLeafsDrag = bEnable; }

	void SetNoDrag( bool bNoDrag ){ m_bNoDrag = bNoDrag; }

protected:
	DECLARE_MESSAGE_MAP()

	virtual void OnItemCopied( HTREEITEM hItem, HTREEITEM hNewItem );

	virtual BOOL IsDropSource( HTREEITEM hItem );

	virtual HTREEITEM GetDropTarget(HTREEITEM hItem);

	BOOL CompareItems(TV_ITEM* pItem, TV_ITEM& tvTempItem);

	//////////////////////////////////////////////////////////////////////////
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);

	afx_msg void OnMouseMove(UINT nFlags, CPoint point);

	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

	virtual BOOL PreTranslateMessage(MSG* pMsg);

	//////////////////////////////////////////////////////////////////////////
	CImageList*	m_pDragImage;
	bool		m_bLDragging;
	HTREEITEM	m_hitemDrag,m_hitemDrop;
	HCURSOR		m_dropCursor,m_noDropCursor;
	bool		m_bOnlyLeafsDrag;
	bool		m_bNoDrag;

};

