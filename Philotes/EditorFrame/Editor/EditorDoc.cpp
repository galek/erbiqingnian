
#include "Editor.h"
#include "EditorDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CEditorDoc, CDocument)

BEGIN_MESSAGE_MAP(CEditorDoc, CDocument)
END_MESSAGE_MAP()


CEditorDoc::CEditorDoc()
{
}

CEditorDoc::~CEditorDoc()
{
}

BOOL CEditorDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
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


