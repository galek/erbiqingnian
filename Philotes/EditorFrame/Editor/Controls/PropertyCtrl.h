
/********************************************************************
日期:		2012年2月20日
文件名: 	PropertyCtrl.h
创建者:		王延兴
描述:		属性控件	
*********************************************************************/

#pragma once

#include "Functor.h"
#include "XmlInterface.h"



#define  PROPERTYCTRL_ONSELECT (0x0001)

_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////

struct CPropertyCtrlNotify
{
	NMHDR hdr;
	CPropertyItem *pItem;
	IVariable *pVariable;

	CPropertyCtrlNotify() : pItem(0),pVariable(0) {}
};

//////////////////////////////////////////////////////////////////////////

class CPropertyCtrl : public CWnd
{
	DECLARE_DYNAMIC(CPropertyCtrl)
public:
	typedef std::vector<CPropertyItem*> Items;

	enum Flags
	{
		F_VARIABLE_HEIGHT   = 0x0010,
		F_VS_DOT_NET_STYLE  = 0x0020,
		F_READ_ONLY         = 0x0040,
		F_GRAYED            = 0x0080,
		F_EXTENDED          = 0x0100,
	};

	typedef Functor1<XmlNodeRef> UpdateCallback;
	typedef Functor1<XmlNodeRef> SelChangeCallback;
	typedef Functor1<IVariable*> UpdateVarCallback;
	typedef Functor1<IVariable*> UpdateObjectCallback;


	CPropertyCtrl();

	virtual						~CPropertyCtrl();

	virtual void				Create( DWORD dwStyle,const CRect &rc,CWnd *pParent=NULL,UINT nID=0 );

	void						SetFlags( int flags ) { m_nFlags = flags; };
	
	int							GetFlags() const { return m_nFlags; };

	void 						CreateItems( XmlNodeRef &node );

	void 						DeleteAllItems();

	virtual void				DeleteItem( CPropertyItem *pItem );

	virtual CPropertyItem* 		AddVarBlock( CVarBlock *varBlock,const char *szCategory=NULL );

	virtual CPropertyItem* 		AddVarBlockAt( CVarBlock *varBlock, const char *szCategory, CPropertyItem *pRoot, bool bFirstVarIsRoot = false);

	virtual void 				UpdateVarBlock(CVarBlock *varBlock,CPropertyItem* poRootItem=NULL);

	virtual void 				ReplaceVarBlock( CPropertyItem* pCategoryItem,CVarBlock *varBlock );

	void 						SetUpdateCallback( UpdateCallback &callback ) { m_updateFunc = callback; }

	void 						SetUpdateCallback( UpdateVarCallback &callback ) { m_updateVarFunc = callback; }

	void 						SetUpdateObjectCallback( UpdateObjectCallback &callback ) { m_updateObjectFunc = callback; }

	void 						ClearUpdateObjectCallback() { m_updateObjectFunc = 0; }

	bool 						EnableUpdateCallback( bool bEnable );

	void 						SetSelChangeCallback( SelChangeCallback &callback ) { m_selChangeFunc = callback; }

	bool						EnableSelChangeCallback( bool bEnable );

	virtual void				ExpandAll();

	virtual void 				ExpandAllChilds( CPropertyItem *item,bool bRecursive );

	virtual void 				ExpandVariableAndChilds( IVariable *varBlock,bool bRecursive );

	virtual void 				RemoveAllChilds( CPropertyItem *pItem );

	virtual void				Expand( CPropertyItem *item,bool bExpand,bool bRedraw=true );

	CPropertyItem*				GetRootItem() const { return m_root; };

	virtual void				ReloadValues();

	void						SetSplitter( int splitter ) { m_splitter = splitter; };

	int							GetSplitter() const { return m_splitter; };

	virtual int					GetVisibleHeight();

	static void					RegisterWindowClass();

	virtual void				OnItemChange( CPropertyItem *item );

	BOOL						EnableWindow( BOOL bEnable = TRUE );

	void						SetDisplayOnlyModified( bool bEnable ) { m_bDisplayOnlyModified = bEnable; };

	CRect						GetItemValueRect( const CRect &rect );
	void						GetItemRect( CPropertyItem *item,CRect &rect );

	void						SetItemHeight( int nItemHeight );

	int							GetItemHeight( CPropertyItem *item ) const;

	void						ClearSelection();

	CPropertyItem*				GetSelectedItem() { return m_selected; }

	void						SetRootName( const CString &rootName );

	CPropertyItem*				FindItemByVar( IVariable *pVar );

	void 						GetVisibleItems( CPropertyItem *root,Items &items );
	bool 						IsCategory( CPropertyItem *item );

	virtual CPropertyItem*		GetItemFromPoint( CPoint point );
	virtual void				SelectItem( CPropertyItem *item );

	void 						MultiSelectItem( CPropertyItem *pItem );
	void 						MultiUnselectItem( CPropertyItem *pItem );
	void 						MultiSelectRange( CPropertyItem *pAnchorItem );
	void 						MultiClearAll();

	void 						RestrictToItemsContaining(const CString &searchName);

	bool 						IsReadOnly();
	bool 						IsExtenedUI() const { return (m_nFlags&F_EXTENDED) != 0; };

	void 						RemoveAllItems();

	void						Flush();

protected:
	friend class CPropertyItem;

	void						SendNotify( int code,CPropertyCtrlNotify &notify );
	void						DrawItem( CPropertyItem *item,CDC &dc,CRect &itemRect );
	int							CalcOffset( CPropertyItem *item );
	void						DrawSign( CDC &dc,CPoint point,bool plus );

	virtual void				CreateInPlaceControl();
	virtual void				DestroyControls( CPropertyItem *pItem );
	bool						IsOverSplitter( CPoint point );
	void						ProcessTooltip( CPropertyItem *item );

	void 						CalcLayout();
	void 						Init();

	virtual void 				InvalidateCtrl();
	virtual void 				InvalidateItems();
	virtual void 				InvalidateItem( CPropertyItem *pItem );
	virtual void 				SwitchUI();

	void 						CopyItem( XmlNodeRef rootNode,CPropertyItem *pItem,bool bRecursively );
	void 						OnCopy( bool bRecursively );
	void 						OnCopyAll();
	void 						OnPaste();

	DECLARE_MESSAGE_MAP()

	afx_msg UINT 				OnGetDlgCode(); 
	afx_msg void 				OnDestroy();
	afx_msg void 				OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void 				OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL 				OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void 				OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void 				OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void 				OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void 				OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void 				OnKillFocus(CWnd* pNewWnd);
	afx_msg void 				OnSetFocus(CWnd* pOldWnd);
	afx_msg void 				OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL 				OnEraseBkgnd(CDC* pDC);
	afx_msg void 				OnPaint();
	afx_msg void 				OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void 				OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL 				OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg LRESULT				OnGetFont(WPARAM wParam, LPARAM);
	afx_msg int					OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void				OnTimer(UINT_PTR nIDEvent);

	virtual BOOL 				PreTranslateMessage(MSG* pMsg);
	virtual void 				PreSubclassWindow();
	virtual BOOL 				OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

protected:

	TSmartPtr<CPropertyItem>	m_root;
	XmlNodeRef					m_xmlRoot;
	bool						m_bEnableCallback;
	UpdateCallback				m_updateFunc;
	bool						m_bEnableSelChangeCallback;
	SelChangeCallback 			m_selChangeFunc;
	UpdateVarCallback 			m_updateVarFunc;
	UpdateObjectCallback		m_updateObjectFunc;
	CImageList					m_icons;

	TSmartPtr<CPropertyItem>	m_selected;
	CBitmap						m_offscreenBitmap;

	CPropertyItem*				m_prevTooltipItem;
	std::vector<CPropertyItem*> m_multiSelectedItems;

	HCURSOR						m_leftRightCursor;
	CBrush						m_bgBrush;
	int							m_splitter;

	CPoint						m_mouseDownPos;
	bool						m_bSplitterDrag;

	CPoint						m_scrollOffset;

	CToolTipCtrl				m_tooltip;

	CFont*						m_pBoldFont;

	bool						m_bDisplayOnlyModified;

	int 						m_nTimer;

	int 						m_nItemHeight;

	int 						m_nFlags;
	bool 						m_bLayoutChange;
	bool 						m_bLayoutValid;

	static std::map<CString,bool> m_expandHistory;

	bool						m_bIsCanExtended;

	CString						m_sNameRestriction;
	TSmartPtr<CVarBlock>		m_pVarBlock;
};

_NAMESPACE_END