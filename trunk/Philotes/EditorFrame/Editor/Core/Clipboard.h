
/********************************************************************
	����:		2012��2��20��
	�ļ���: 	Clipboard.h
	������:		������
	����:		������	
*********************************************************************/

#include "XmlInterface.h"

_NAMESPACE_BEGIN

class CClipboard
{
public:
	void				Put( XmlNodeRef &node,const CString &title = "" );

	XmlNodeRef			Get() const;

	void				PutString( const CString &text,const CString &title = "" );
	
	CString				GetString() const;

	CString				GetTitle() const { return m_title; };

	bool				IsEmpty() const;

private:
	
	static XmlNodeRef	m_node;
	
	static CString		m_title;
};

_NAMESPACE_END