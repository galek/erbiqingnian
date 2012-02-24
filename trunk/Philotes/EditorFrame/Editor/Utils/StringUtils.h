
/********************************************************************
	日期:		2012年2月24日
	文件名: 	StringUtils.h
	创建者:		王延兴
	描述:		常用字符串操作	
*********************************************************************/

#pragma once

_NAMESPACE_BEGIN

class StringUtil
{
public:

	static void						Trim( std::string& str, bool left = true, bool right = true );

	static std::vector<std::string> Split( const std::string& str, const std::string& delims = "\t\n ", 
		unsigned int maxSplits = 0, bool preserveDelims = false);

	static std::vector<std::string> Tokenise( const std::string& str, const std::string& delims = "\t\n ", 
		const std::string& doubleDelims = "\"", unsigned int maxSplits = 0);

	static void						ToLowerCase( std::string& str );

	static void 					ToUpperCase( std::string& str );

	static bool 					StartsWith(const std::string& str, const std::string& pattern, bool lowerCase = true);

	static bool 					EndsWith(const std::string& str, const std::string& pattern, bool lowerCase = true);

	static std::string				StandardisePath( const std::string &init);
	
	static bool						Match(const std::string& str, const std::string& pattern, bool caseSensitive = true);

	static const std::string		ReplaceAll(const std::string& source, const std::string& replaceWhat, const std::string& replaceWithWhat);

	static const std::string		BLANK;
};

_NAMESPACE_END