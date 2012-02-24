/********************************************************************
	����:		2012��2��20��
	�ļ���: 	EditorDoc.h
	������:		������
	����:		�ĵ���	
*********************************************************************/

#pragma once

#include "XmlInterface.h"

_NAMESPACE_BEGIN

class CEditorDoc : public CDocument
{
protected:
	
	CEditorDoc();

	DECLARE_DYNCREATE(CEditorDoc)

public:
	virtual BOOL 	OnNewDocument();

	virtual void 	Serialize(CArchive& ar);

	virtual			~CEditorDoc();

#ifdef _DEBUG
	virtual void	AssertValid() const;
	virtual void	Dump(CDumpContext& dc) const;
#endif

public:

	XmlNodeRef&		GetEnvironmentTemplate() { return m_environmentTemplate; }

protected:

	XmlNodeRef		m_environmentTemplate;

protected:

	DECLARE_MESSAGE_MAP()
};

_NAMESPACE_END