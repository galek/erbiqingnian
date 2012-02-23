
#include "Editor.h"
#include "EditorDoc.h"
#include "EditorRoot.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CEditorDoc, CDocument)

BEGIN_MESSAGE_MAP(CEditorDoc, CDocument)
END_MESSAGE_MAP()


CEditorDoc::CEditorDoc()
{
	EditorRoot::Get().SetDocument(this);

	m_environmentTemplate = EditorRoot::Get().FindTemplate( "Environment" );
	if (m_environmentTemplate)
	{
	}
	else
	{
		m_environmentTemplate = EditorRoot::Get().CreateXmlNode( "Environment" );
	}
}

CEditorDoc::~CEditorDoc()
{
}

BOOL CEditorDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	EditorRoot::Get().ReloadTemplates();
	m_environmentTemplate = EditorRoot::Get().FindTemplate( "Environment" );

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


