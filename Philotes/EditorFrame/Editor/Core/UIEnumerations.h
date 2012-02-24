
#pragma once

_NAMESPACE_BEGIN

class CUIEnumerations
{
	public:
		typedef std::vector<CString>		TDValues;
		typedef std::map<CString,TDValues>	TDValuesContainer;
		typedef std::vector<CString>		TDSelectedAnimations;

	public:
		static CUIEnumerations& GetUIEnumerationsInstance();

		TDSelectedAnimations&	GetSelectedAnimations();

		TDValuesContainer&		GetStandardNameContainer();
};

_NAMESPACE_END