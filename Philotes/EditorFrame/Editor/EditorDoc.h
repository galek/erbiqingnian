/********************************************************************
	日期:		2012年2月20日
	文件名: 	EditorDoc.h
	创建者:		王延兴
	描述:		MFC文档类	
*********************************************************************/

#pragma once

class CEditorDoc : public CDocument
{
protected:
	CEditorDoc();
	DECLARE_DYNCREATE(CEditorDoc)

public:
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

public:
	virtual ~CEditorDoc();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

protected:
	DECLARE_MESSAGE_MAP()
};
