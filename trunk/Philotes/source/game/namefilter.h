#ifndef _NAMEFILTER_H_
#define _NAMEFILTER_H_

enum NAME_TYPE
{
	NAME_TYPE_PLAYER,
	NAME_TYPE_GUILD,
	NAME_TYPE_MAX
};

BOOL ValidateName(
	WCHAR * name,
	unsigned int buflen,
	NAME_TYPE eNameType,
	BOOL bFixCapitalization = FALSE);

void NameFilterAllowKoreanCharacters(
	BOOL bAllow);
void NameFilterAllowChineseCharacters(
	BOOL bAllow);
void NameFilterAllowJapaneseCharacters(
	BOOL bAllow);

#endif
