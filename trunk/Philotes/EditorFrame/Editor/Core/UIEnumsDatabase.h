
/********************************************************************
	����:		2012��2��21��
	�ļ���: 	UIEnumsDatabase.h
	������:		������
	����:		����ö��
*********************************************************************/

#pragma once

_NAMESPACE_BEGIN

struct CUIEnumsDatabase_SEnum 
{
	CString name;
	std::vector<CString> strings;
	std::vector<CString> values;

	CString NameToValue( const CString &name );
	CString ValueToName( const CString &value );
};

//////////////////////////////////////////////////////////////////////////

class CUIEnumsDatabase
{
public:
	CUIEnumsDatabase();

	~CUIEnumsDatabase();

	void SetEnumStrings( const CString &enumName,const char **sStringsArray,int nStringCount );

	CUIEnumsDatabase_SEnum* FindEnum( const CString &enumName );

private:
	typedef std::map<CString,CUIEnumsDatabase_SEnum*> Enums;
	Enums m_enums;
};

_NAMESPACE_END