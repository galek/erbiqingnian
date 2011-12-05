#ifndef _COLORSET_H_
#define _COLORSET_H_

#define COLOR_SET_COLOR_COUNT 6

struct COLOR_DEFINITION
{
	int				nId;
	int				nUnittype;
	DWORD			pdwColors[ COLOR_SET_COLOR_COUNT ];
	COLOR_DEFINITION	* pNext;
};

struct COLOR_SET_DEFINITION 
{
	DEFINITION_HEADER		tHeader;

	int						nColorDefinitionCount;
	COLOR_DEFINITION *		pColorDefinitions;
};

struct COLORSET_DATA
{
	char szColor[DEFAULT_INDEX_SIZE];
	SAFE_PTR(COLOR_DEFINITION*, pFirstDef);
	WORD wCode;
	BOOL bCanBeRandomPick;
};

void ColorSetClose();

int ColorSetPickRandom(
	UNIT * pUnit );

const COLOR_DEFINITION * ColorSetGetColorDefinition( int nExcelIndex, int nUnittype );

BOOL ColorSetDefinitionPostXMLLoad(void * pDef, BOOL bForceSyncLoad);

BOOL ColorSetsExcelPostProcess(
	struct EXCEL_TABLE * table);


#endif
