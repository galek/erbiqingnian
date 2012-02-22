
#include "EditorTool.h"
#include "ClassPlugin.h"
#include "EditorRoot.h"

IMPLEMENT_DYNAMIC(CEditTool,CObject);

class CEditTool_ClassDesc : public CRefCountClassDesc
{
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_EDITTOOL; }

	virtual REFGUID ClassID() 
	{
		static const GUID guid = { 0xa43ab8e, 0xb1ae, 0x44aa, { 0x93, 0xb1, 0x22, 0x9f, 0x73, 0xd5, 0x8c, 0xa4 } };
		return guid;
	}

	virtual const char* ClassName() { return "EditTool.Default"; };

	virtual const char* Category() { return "EditTool"; };

	virtual CRuntimeClass* GetRuntimeClass() { return 0; }
};

CEditTool_ClassDesc g_stdClassDesc;

CEditTool::CEditTool()
{
	m_pClassDesc = &g_stdClassDesc;
	m_nRefCount = 0;
};

void CEditTool::SetParentTool( CEditTool *pTool )
{
	m_pParentTool = pTool;
}

CEditTool* CEditTool::GetParentTool()
{
	return m_pParentTool;
}

void CEditTool::Abort()
{
	if (m_pParentTool)
	{
		EditorRoot::Get().SetEditTool(m_pParentTool);
	}
	else
	{
		EditorRoot::Get().SetEditTool(0);
	}
}