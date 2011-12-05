//----------------------------------------------------------------------------
// FILE: characterclass.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifdef __CHARACTER_CLASS_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define __CHARACTER_CLASS_H_

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct UNIT;

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------
#define MAX_CHARACTER_RACES		8
//----------------------------------------------------------------------------
enum GENDER
{
	GENDER_INVALID = -1,

	// genders that are selectable for your character	
	GENDER_MALE,
	GENDER_FEMALE,	
	GENDER_NUM_CHARACTER_GENDERS,
	
	// additional language genders
	GENDER_NEUTER = GENDER_NUM_CHARACTER_GENDERS,

	// total genders	
	GENDER_NUM_GENDERS
	
};

//----------------------------------------------------------------------------
struct CHARACTER_CLASS
{
	int nPlayerClass[ MAX_CHARACTER_RACES ];			// row in players.xls
	BOOL bEnabled;				// is enabled for play
};

//----------------------------------------------------------------------------
struct CHARACTER_CLASS_DATA
{
	char szName[ DEFAULT_INDEX_SIZE ];
	CHARACTER_CLASS tClasses[ GENDER_NUM_CHARACTER_GENDERS ];
	int nUnitVersionToGetSkillRespec;
	int nUnitVersionToGetPerkRespec;
	int nStringOneLetterCode;
	BOOL bDefault;
	int nItemClassScrapSpecial;			// items for this character class can be dismantled into these 
};

//----------------------------------------------------------------------------
// Function Prototypes
//----------------------------------------------------------------------------

BOOL CharacterClassIsEnabled( 
	int nCharacterClass, 
	int nCharacterRace, 
	GENDER eGender);

const CHARACTER_CLASS_DATA *CharacterClassGetData(
	int nCharacterClass);

int CharacterClassNumClasses(
	void);
	
int CharacterClassGet(
	int nUnitClass);
	
int CharacterClassGet(
	UNIT *pPlayer);

int CharacterClassGetPlayerClass( 
	int nCharacterClass, 
	int nCharacterRace,
	GENDER eGender);

int CharacterClassGetDefault(	
	void);
		
#endif