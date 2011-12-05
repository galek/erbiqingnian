//----------------------------------------------------------------------------
// FILE: skill_strings.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifdef __SKILLEFFECT_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define __SKILLEFFECT_H_

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct UNIT;

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------
#define MAX_SKILL_TOOLTIP (4096)

//----------------------------------------------------------------------------
enum SKILL_TOOLTIP_TYPE
{
	STT_INVALID = -1,
	
	STT_CURRENT_LEVEL,
	STT_NEXT_LEVEL,
};
#define	SKILL_STRING_FLAG_NO_TITLE				MAKE_MASK( 1 )
#define SKILL_STRING_FLAG_NO_POWER_COST_IF_SAME	MAKE_MASK( 2 )
#define SKILL_STRING_FLAG_NO_RANGE_IF_SAME		MAKE_MASK( 3 )
#define SKILL_STRING_FLAG_NO_COOLDOWN_IF_SAME	MAKE_MASK( 4 )

#define SKILL_STRING_MASK_DESCRIPTION			(0)
#define SKILL_STRING_MASK_PER_LEVEL				(SKILL_STRING_FLAG_NO_POWER_COST_IF_SAME | SKILL_STRING_FLAG_NO_RANGE_IF_SAME | SKILL_STRING_FLAG_NO_COOLDOWN_IF_SAME)


//----------------------------------------------------------------------------
// Function Prototypes
//----------------------------------------------------------------------------
BOOL SkillGetString(
	int nSkill,
	UNIT *pUnit,
	SKILL_STRING_TYPES eStringType,	
	SKILL_TOOLTIP_TYPE eTooltipType,
	WCHAR *puszBuffer,
	int nBufferSize,
	DWORD dwFlags,
	BOOL bWithFormatting = TRUE,
	int nSkillLevel	= 0 );

void c_SkillStringsToggleDebug();

#endif  // end __SKILLEFFECT_H_