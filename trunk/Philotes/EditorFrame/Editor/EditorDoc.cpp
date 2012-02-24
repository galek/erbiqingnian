
#include "Editor.h"
#include "EditorDoc.h"
#include "EditorRoot.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

_NAMESPACE_BEGIN

IMPLEMENT_DYNCREATE(CEditorDoc, CDocument)

BEGIN_MESSAGE_MAP(CEditorDoc, CDocument)
END_MESSAGE_MAP()


CEditorDoc::CEditorDoc()
{
	CEditorRoot::Get().SetDocument(this);

	m_environmentTemplate = CEditorRoot::Get().FindTemplate( "Environment" );
	if (m_environmentTemplate)
	{
	}
	else
	{
		m_environmentTemplate = CEditorRoot::Get().CreateXmlNode( "Environment" );
	}
}

CEditorDoc::~CEditorDoc()
{
}

BOOL CEditorDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	CEditorRoot::Get().ReloadTemplates();
	m_environmentTemplate = CEditorRoot::Get().FindTemplate( "Environment" );

	return TRUE;
}

void CEditorDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// CEditorDoc diagnostics

#ifdef _DEBUG
void CEditorDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CEditorDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


_NAMESPACE_END