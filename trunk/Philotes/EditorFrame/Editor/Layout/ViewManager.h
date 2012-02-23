
/********************************************************************
	日期:		2012年2月23日
	文件名: 	ViewManager.h
	创建者:		王延兴
	描述:		视图管理器	
*********************************************************************/

#pragma once

#include "ViewPaneInterface.h"
#include "Viewport.h"

class CLayoutWnd;
class CViewport;

class CViewportDesc : public CRefCountBase
{
public:
	EViewportType	type;

	CString			name;

	CRuntimeClass*	rtClass;

	IViewPaneClass*	pViewClass;

	CViewportDesc( EViewportType _type, const CString &_name,CRuntimeClass* _rtClass )
	{
		pViewClass	= NULL;
		type		= _type;
		name		= _name;
		rtClass		= _rtClass;
	}
};

//////////////////////////////////////////////////////////////////////////

class CViewManager
{
public:
	CViewport*			CreateView( EViewportType type,CWnd *pParentWnd );

	void				ReleaseView( CViewport *pViewport );
	
	CViewport*			GetViewport( EViewportType type ) const;

	CViewport*			GetViewport( const CString &name ) const;

	CViewport*			GetActiveViewport() const;

	CViewport*			GetViewportAtPoint( CPoint point ) const;

	void				ResetViews();

	void				UpdateViews( int flags=0xFFFFFFFF );

	virtual void		RegisterViewportDesc( CViewportDesc *desc );

	virtual void		GetViewportDescriptions( std::vector<CViewportDesc*> &descriptions );

	virtual int			GetViewCount() { return m_viewports.size(); };

	virtual CViewport*	GetView( int index ) { return m_viewports[index]; }

	virtual CLayoutWnd* GetLayout() const;

	// 获取主视口，即透视摄像机视口
	CViewport*			GetGameViewport() const;

private:
	friend class 		CViewport;
	friend class		EditorRoot;

	void 				IdleUpdate();

	void 				RegisterViewport( CViewport *vp );
	
	void 				UnregisterViewport( CViewport *vp );

private:
	
	CViewManager();
	
	~CViewManager();

	std::vector<TSmartPtr<CViewportDesc> > m_viewportDesc;
	
	std::vector<CViewport*> m_viewports;
};