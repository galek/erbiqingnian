
/********************************************************************
	日期:		2012年2月22日
	文件名: 	EditorTool.h
	创建者:		王延兴
	描述:		编辑器编辑物件基类	
*********************************************************************/

#pragma once

struct IClassDesc;
struct ITransformManipulator;

enum EEditToolType
{
	EDIT_TOOL_TYPE_PRIMARY,
	EDIT_TOOL_TYPE_SECONDARY,
};

class CEditTool : public CObject
{
public:
	DECLARE_DYNAMIC(CEditTool);

	CEditTool();

	void 					AddRef() { m_nRefCount++; };

	void 					Release() { if (--m_nRefCount <= 0) DeleteThis(); };

	IClassDesc*				GetClassDesc() const { return m_pClassDesc; }

	virtual void			SetParentTool( CEditTool *pTool );

	virtual CEditTool*		GetParentTool();

	virtual EEditToolType	GetType() { return EDIT_TOOL_TYPE_PRIMARY; }

	virtual void			Abort();

	void					SetStatusText( const CString &text ) { m_statusText = text; }

	const char*				GetStatusText() { return m_statusText; }

	virtual bool			Activate( CEditTool *pPreviousTool ) { return true; }
	
	virtual void			SetUserData( void *userData ) {}

	virtual bool 			IsNeedMoveTool() { return false; }

	virtual void 			BeginEditParams(int flags ) {}

	virtual void 			EndEditParams() {};

protected:
	virtual ~CEditTool() {};

	virtual void			DeleteThis() = 0;

protected:
	TSmartPtr<CEditTool>	m_pParentTool;
	CString					m_statusText;
	IClassDesc*				m_pClassDesc;
	int						m_nRefCount;
};