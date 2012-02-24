
#include "UIEnumsDatabase.h"
#include "StlUtil.h"

_NAMESPACE_BEGIN

CString CUIEnumsDatabase_SEnum::NameToValue( const CString &name )
{
	int n = (int)strings.size();
	for (int i = 0; i < n; i++)
	{
		if (name == strings[i])
			return values[i];
	}
	return name;
}

CString CUIEnumsDatabase_SEnum::ValueToName( const CString &value )
{
	int n = (int)strings.size();
	for (int i = 0; i < n; i++)
	{
		if (value == values[i])
			return strings[i];
	}
	return value;
}

CUIEnumsDatabase::CUIEnumsDatabase()
{
}

CUIEnumsDatabase::~CUIEnumsDatabase()
{
	for (Enums::iterator it = m_enums.begin(); it != m_enums.end(); ++it)
	{
		delete it->second;
	}
}

void CUIEnumsDatabase::SetEnumStrings( const CString &enumName,const char **sStringsArray,int nStringCount )
{
	CUIEnumsDatabase_SEnum *pEnum = stl::FindInMap(m_enums,enumName,0);
	if (!pEnum)
	{
		pEnum = new CUIEnumsDatabase_SEnum;
		pEnum->name = enumName;
		m_enums[enumName] = pEnum;
	}
	pEnum->strings.resize(nStringCount);
	pEnum->values.resize(nStringCount);
	for (int i = 0; i < nStringCount; i++)
	{
		CString str = sStringsArray[i];
		CString value = str;
		int pos = str.Find('=');
		if (pos >= 0)
		{
			value = str.Mid(pos+1);
			str = str.Mid(0,pos);
		}
		pEnum->strings[i] = str;
		pEnum->values[i] = value;
	}
}

CUIEnumsDatabase_SEnum* CUIEnumsDatabase::FindEnum( const CString &enumName )
{
	CUIEnumsDatabase_SEnum *pEnum = stl::FindInMap(m_enums,enumName,0);
	return pEnum;
}

_NAMESPACE_END