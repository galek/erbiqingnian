/********************************************************************
	����:		2012��2��20��
	�ļ���: 	EditorDoc.h
	������:		������
	����:		MFC�ĵ���	
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
