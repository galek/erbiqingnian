#pragma once
//-------------------------------------------------------------------------------------------------
#define DRLG_FILE_MAGIC_NUMBER			0xd2d21d25
#define DRLG_FILE_VERSION				(29 + GLOBAL_FILE_VERSION)
#define DRLG_FILE_EXTENSION				"drl"
#define MAX_REPLACEMENTS_PER_RULE		32
#define MAX_TEMPLATES_PER_FILE			10
#define MAX_RULES_PER_FILE				256
#define MAX_ROOMS_PER_GROUP				512
#define MAX_ROOMS_PER_GROUP_LARGE		3200
#define MAX_HULL_VERTICES				256

//-------------------------------------------------------------------------------------------------
struct DRLG_ROOM
{
	char pszFileName[ MAX_XML_STRING_LENGTH ];
	int nSameNameIndex;						// this helps with file saving
	SAFE_PTR(ROOM*, pRoom);

	VECTOR vLocation;
	float fRotation;
	SUBLEVELID idSubLevel;		// some rooms are considered a sublevel of a level (like hellrifts)
//	int nRegion;						// not in the main area. used for hell-rift sub-levels/regions

	BOUNDING_BOX tBoundingBox;

	SAFE_PTR(CONVEXHULL*, pHull);
};

//-------------------------------------------------------------------------------------------------
struct DRLG_LEVEL_TEMPLATE 
{
	int nRoomCount;
	SAFE_PTR(DRLG_ROOM*, pRooms);
};

//-------------------------------------------------------------------------------------------------
struct DRLG_SUBSTITUTION_RULE
{
	int nRuleRoomCount;						// A Rule is a configuration of rooms that can be replaced
	SAFE_PTR(DRLG_ROOM*, pRuleRooms);
	int nReplacementCount;					// A replacement is a configuration of rooms to replace the rule rooms with
	int pnReplacementRoomCount   [ MAX_REPLACEMENTS_PER_RULE ];
	SAFE_PTR(DRLG_ROOM*, ppReplacementRooms[ MAX_REPLACEMENTS_PER_RULE ]);
};

//-------------------------------------------------------------------------------------------------
struct DRLG_LEVEL_DEFINITION
{
	DEFINITION_HEADER		tHeader;
	SAFE_PTR(BYTE*,			pbyFileStart);
	char					pszFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	int						nTemplateCount;
	SAFE_PTR(DRLG_LEVEL_TEMPLATE*, pTemplates);
	int						nSubstitutionCount;
	SAFE_PTR(DRLG_SUBSTITUTION_RULE*, pSubsitutionRules);	
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DrlgPathingMeshAddModel(
	GAME* game,
	ROOM* room,
	struct ROOM_DEFINITION* prop,
	struct ROOM_LAYOUT_GROUP* model,
	MATRIX* matrixPropToWorld);
