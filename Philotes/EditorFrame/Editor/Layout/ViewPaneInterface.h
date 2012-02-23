
/********************************************************************
	日期:		2012年2月22日
	文件名: 	ViewPaneInterface.h
	创建者:		王延兴
*********************************************************************/

#pragma once

#include "ClassFactory.h"

struct __declspec( uuid("{7E13EC7C-F621-4aeb-B642-67D78ED468F8}") ) IViewPaneClass : public IClassDesc
{
	enum EDockingDirection
	{
		DOCK_TOP,
		DOCK_LEFT,
		DOCK_RIGHT,
		DOCK_BOTTOM,
		DOCK_FLOAT,
	};
	virtual CRuntimeClass*		GetRuntimeClass() = 0;

	virtual const char*			GetPaneTitle() = 0;

	virtual EDockingDirection	GetDockingDirection() = 0;

	virtual CRect				GetPaneRect() = 0;

	virtual CSize				GetMinSize() { return CSize(0,0); }

	virtual bool				SinglePane() = 0;

	virtual bool				WantIdleUpdate() = 0;

	HRESULT STDMETHODCALLTYPE	QueryInterface( const IID &riid, void **ppvObj )
	{
		if (riid == __uuidof(IViewPaneClass))
		{
			*ppvObj = this;
			return S_OK;
		}
		return E_NOINTERFACE ;
	}
};
