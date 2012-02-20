/********************************************************************
	日期:		2012年2月20日
	文件名: 	EditorView.h
	创建者:		王延兴
	描述:		MFC试图类	
*********************************************************************/

#pragma once


class CEditorView : public CView
{
protected:
	CEditorView();
	DECLARE_DYNCREATE(CEditorView)

public:
	CEditorDoc* GetDocument() const;

public:

	virtual void OnDraw(CDC* pDC);
	
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

public:
	virtual ~CEditorView();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG 
inline CEditorDoc* CEditorView::GetDocument() const
   { return reinterpret_cast<CEditorDoc*>(m_pDocument); }
#endif
