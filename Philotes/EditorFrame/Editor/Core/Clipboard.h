
/********************************************************************
	日期:		2012年2月20日
	文件名: 	Clipboard.h
	创建者:		王延兴
	描述:		剪贴板	
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