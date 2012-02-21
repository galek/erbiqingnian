
#include "StringUtils.h"

void StringUtil::Trim( std::string& str, bool left /*= true*/, bool right /*= true */ )
{
	static const std::string delims = " \t\r";
	if(right)
		str.erase(str.find_last_not_of(delims)+1); // trim right
	if(left)
		str.erase(0, str.find_first_not_of(delims)); // trim left
}

std::vector<std::string> StringUtil::Split( const std::string& str, const std::string& delims /*= "\t\n "*/, 
										   unsigned int maxSplits /*= 0*/, bool preserveDelims /*= false*/ )
{
	std::vector<std::string> ret;

	ret.reserve(maxSplits ? maxSplits+1 : 10);    // 10 is guessed capacity for most case

	unsigned int numSplits = 0;

	// Use STL methods 
	size_t start, pos;
	start = 0;
	do 
	{
		pos = str.find_first_of(delims, start);
		if (pos == start)
		{
			// Do nothing
			start = pos + 1;
		}
		else if (pos == std::string::npos || (maxSplits && numSplits == maxSplits))
		{
			// Copy the rest of the string
			ret.push_back( str.substr(start) );
			break;
		}
		else
		{
			// Copy up to delimiter
			ret.push_back( str.substr(start, pos - start) );

			if(preserveDelims)
			{
				// Sometimes there could be more than one delimiter in a row.
				// Loop until we don't find any more delims
				size_t delimStart = pos, delimPos;
				delimPos = str.find_first_not_of(delims, delimStart);
				if (delimPos == std::string::npos)
				{
					// Copy the rest of the string
					ret.push_back( str.substr(delimStart) );
				}
				else
				{
					ret.push_back( str.substr(delimStart, delimPos - delimStart) );
				}
			}

			start = pos + 1;
		}
		// parse up to next real data
		start = str.find_first_not_of(delims, start);
		++numSplits;

	} while (pos != std::string::npos);



	return ret;
}

std::vector<std::string> StringUtil::Tokenise( const std::string& str, const std::string& singleDelims /*= "\t\n "*/, 
											  const std::string& doubleDelims /*= "\""*/, unsigned int maxSplits /*= 0*/ )
{
	std::vector<std::string> ret;

	ret.reserve(maxSplits ? maxSplits+1 : 10);    // 10 is guessed capacity for most case

	unsigned int numSplits = 0;
	std::string delims = singleDelims + doubleDelims;

	// Use STL methods 
	size_t start, pos;
	char curDoubleDelim = 0;
	start = 0;
	do 
	{
		if (curDoubleDelim != 0)
		{
			pos = str.find(curDoubleDelim, start);
		}
		else
		{
			pos = str.find_first_of(delims, start);
		}

		if (pos == start)
		{
			char curDelim = str.at(pos);
			if (doubleDelims.find_first_of(curDelim) != std::string::npos)
			{
				curDoubleDelim = curDelim;
			}
			// Do nothing
			start = pos + 1;
		}
		else if (pos == std::string::npos || (maxSplits && numSplits == maxSplits))
		{
			if (curDoubleDelim != 0)
			{
				//Missing closer. Warn or throw exception?
			}
			// Copy the rest of the string
			ret.push_back( str.substr(start) );
			break;
		}
		else
		{
			if (curDoubleDelim != 0)
			{
				curDoubleDelim = 0;
			}

			// Copy up to delimiter
			ret.push_back( str.substr(start, pos - start) );
			start = pos + 1;
		}
		if (curDoubleDelim == 0)
		{
			// parse up to next real data
			start = str.find_first_not_of(singleDelims, start);
		}

		++numSplits;

	} while (pos != std::string::npos);

	return ret;
}

void StringUtil::ToLowerCase( std::string& str )
{
	std::transform(
		str.begin(),
		str.end(),
		str.begin(),
		tolower);
}

void StringUtil::ToUpperCase( std::string& str )
{
	std::transform(
		str.begin(),
		str.end(),
		str.begin(),
		toupper);
}

bool StringUtil::StartsWith( const std::string& str, const std::string& pattern, bool lowerCase /*= true*/ )
{
	size_t thisLen = str.length();
	size_t patternLen = pattern.length();
	if (thisLen < patternLen || patternLen == 0)
		return false;

	std::string startOfThis = str.substr(0, patternLen);
	if (lowerCase)
		StringUtil::ToLowerCase(startOfThis);

	return (startOfThis == pattern);
}

bool StringUtil::EndsWith( const std::string& str, const std::string& pattern, bool lowerCase /*= true*/ )
{
	size_t thisLen = str.length();
	size_t patternLen = pattern.length();
	if (thisLen < patternLen || patternLen == 0)
		return false;

	std::string endOfThis = str.substr(thisLen - patternLen, patternLen);
	if (lowerCase)
		StringUtil::ToLowerCase(endOfThis);

	return (endOfThis == pattern);
}

std::string StringUtil::StandardisePath( const std::string &init )
{
	std::string path = init;

	std::replace( path.begin(), path.end(), '\\', '/' );
	if( path[path.length() - 1] != '/' )
		path += '/';

	return path;
}

bool StringUtil::Match( const std::string& str, const std::string& pattern, bool caseSensitive /*= true*/ )
{
	std::string tmpStr = str;
	std::string tmpPattern = pattern;
	if (!caseSensitive)
	{
		StringUtil::ToLowerCase(tmpStr);
		StringUtil::ToLowerCase(tmpPattern);
	}

	std::string::const_iterator strIt = tmpStr.begin();
	std::string::const_iterator patIt = tmpPattern.begin();
	std::string::const_iterator lastWildCardIt = tmpPattern.end();
	while (strIt != tmpStr.end() && patIt != tmpPattern.end())
	{
		if (*patIt == '*')
		{
			lastWildCardIt = patIt;
			++patIt;
			if (patIt == tmpPattern.end())
			{
				strIt = tmpStr.end();
			}
			else
			{
				while(strIt != tmpStr.end() && *strIt != *patIt)
					++strIt;
			}
		}
		else
		{
			if (*patIt != *strIt)
			{
				if (lastWildCardIt != tmpPattern.end())
				{
					patIt = lastWildCardIt;
					lastWildCardIt = tmpPattern.end();
				}
				else
				{
					return false;
				}
			}
			else
			{
				++patIt;
				++strIt;
			}
		}

	}
	if (patIt == tmpPattern.end() && strIt == tmpStr.end())
	{
		return true;
	}
	else
	{
		return false;
	}
}

const std::string StringUtil::ReplaceAll( const std::string& source, const std::string& replaceWhat, 
										 const std::string& replaceWithWhat )
{
	std::string result = source;
	std::string::size_type pos = 0;
	while(1)
	{
		pos = result.find(replaceWhat,pos);
		if (pos == std::string::npos) break;
		result.replace(pos,replaceWhat.size(),replaceWithWhat);
		pos += replaceWithWhat.size();
	}
	return result;
}
