
#include "Clipboard.h"
#include "Xml.h"

void CClipboard::Put( XmlNodeRef &node,const CString &title /*= "" */ )
{
	m_title = title;
	if (m_title.IsEmpty())
	{
		m_title = node->GetTag();
	}
	m_node = node;

	PutString( m_node->GetXML().c_str(),title );
}

XmlNodeRef CClipboard::Get() const
{
	CString str = GetString();

	XmlParser parser;
	XmlNodeRef node = parser.parseBuffer( str );
	return node;
}

void CClipboard::PutString( const CString &text,const CString &title /*= "" */ )
{
	if (!OpenClipboard(NULL))
	{
		AfxMessageBox( "Cannot open the Clipboard" );
		return;
	}
	if( !EmptyClipboard() )
	{
		AfxMessageBox( "Cannot empty the Clipboard" );
		return;
	}

	CSharedFile	sf(GMEM_MOVEABLE|GMEM_DDESHARE|GMEM_ZEROINIT);

	sf.Write(text, text.GetLength());

	HGLOBAL hMem = sf.Detach();
	if (!hMem)
		return;

	if ( ::SetClipboardData( CF_TEXT,hMem ) == NULL )
	{
		AfxMessageBox( "Unable to set Clipboard data" );
		CloseClipboard();
		return;
	}
	CloseClipboard();
}

CString CClipboard::GetString() const
{
	CString buffer;
	if(OpenClipboard(NULL))
	{
		buffer = (char*)GetClipboardData(CF_TEXT);
	}
	CloseClipboard();

	return buffer;
}

bool CClipboard::IsEmpty() const
{
	return GetString().IsEmpty();
}

CString		CClipboard::m_title;
XmlNodeRef	CClipboard::m_node;
