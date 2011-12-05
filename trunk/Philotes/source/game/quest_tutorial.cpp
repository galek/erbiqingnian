//----------------------------------------------------------------------------
// FILE: quest_tutorial.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_quests.h"
#include "game.h"
#include "globalindex.h"
#include "npc.h"
#include "quest_tutorial.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "monsters.h"
#include "offer.h"
#include "ai.h"
#include "treasure.h"
#include "items.h"
#include "units.h"
#include "c_message.h"
#include "uix.h"
#include "states.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

struct QUEST_DATA_TUTORIAL
{
	int		nMurmur;
	int		nHurtTemplar;

	int		nTutorialZombie;
	int		nZombie;

	int		nLevelTutorial;
	int		nLevelHolbornStation;
	int		nFollowAI;

	int		nDestinationTip;
	
	int		nTreasureTutorialFirst;

	int		nObjectWarpH_RS;
	int		nObjectWarpRS_H;

	UNITID	idFirstItem;

	BOOL	bTipActive;

	int		client_tip_index;

	DWORD	dwTipFlags;
};

//----------------------------------------------------------------------------

struct TUTORIAL_DATA
{
	int		nString;		// link to string
	BOOL	bNextTip;		// is there a tip to follow, or wait until another event
	int		nDuration;		// length before fadeout (in ticks)
	int		nSkipFlag;		// skip this tip if a flag is set
	int		nWaitFlag;		// wait on this tip until a flag is set
	int		nShortenFlag;	// shorten length (by 1 second) if this flag is set
};

//----------------------------------------------------------------------------
// static data
//----------------------------------------------------------------------------

enum
{
	TUTORIAL_TIP_STARTWELCOME,
	TUTORIAL_TIP_STARTABOUT,
	TUTORIAL_TIP_STARTMANUAL,
	TUTORIAL_TIP_VIDEOSTART,
	TUTORIAL_TIP_VIDEOOPEN,
	TUTORIAL_TIP_VIDEOMURMUR,
	TUTORIAL_TIP_VIDEOWHO,
	TUTORIAL_TIP_VIDEOREAD,
	TUTORIAL_TIP_VIDEOARROW,
	TUTORIAL_TIP_VIDEOCOLORED,
	TUTORIAL_TIP_VIDEOACCEPT,
	TUTORIAL_TIP_VIDEODONE,
	TUTORIAL_TIP_QUESTLOGSTART,
	TUTORIAL_TIP_QUESTLOGOPEN,
	TUTORIAL_TIP_QUESTLOGUP,
	TUTORIAL_TIP_QUESTLOGUPCLICK,
	TUTORIAL_TIP_QUESTLOGTRACK,
	TUTORIAL_TIP_QUESTLOGTRACKCLICK,
	TUTORIAL_TIP_QUESTLOGTRACKDISPLAY,
	TUTORIAL_TIP_QUESTLOGCLOSE,
	TUTORIAL_TIP_QUESTLOGEND,
	TUTORIAL_TIP_AUTOMAPPRIMER,
	TUTORIAL_TIP_AUTOMAPOPEN,
	TUTORIAL_TIP_AUTOMAPICONS,
	TUTORIAL_TIP_MOVEFORWARD,
	TUTORIAL_TIP_MOVESIDE,
	TUTORIAL_TIP_MOVEBACK,
	TUTORIAL_TIP_MOVEMOUSELOOK,
	TUTORIAL_TIP_MOVEZOOM,
	TUTORIAL_TIP_MOVEJUMP,
	TUTORIAL_TIP_MOVEEND,
	TUTORIAL_TIP_FIGHTSTART,
	TUTORIAL_TIP_FIGHTPRACTICE,
	TUTORIAL_TIP_FIGHTLEFTBUTTON,
	TUTORIAL_TIP_FIGHTLATER,
	TUTORIAL_TIP_FIGHTFORNOW,
	TUTORIAL_TIP_FIGHTCROSSHAIRS,
	TUTORIAL_TIP_FIGHTENEMYINFO,
	TUTORIAL_TIP_FIGHTREDHUD,
	TUTORIAL_TIP_FIGHTGREENHUD,
	TUTORIAL_TIP_FIGHTKILL,
	TUTORIAL_TIP_FIGHTCONGRATS,
	TUTORIAL_TIP_FIGHTCRATES,
	TUTORIAL_TIP_GETSTART,
	TUTORIAL_TIP_GETMETHOD,
	TUTORIAL_TIP_GETALT,
	TUTORIAL_TIP_GETALTPICKUP,
	TUTORIAL_TIP_GETFPICKUP,
	TUTORIAL_TIP_GETEND,
	TUTORIAL_TIP_INVENTORYSTART,
	TUTORIAL_TIP_INVENTORYOPEN,
	TUTORIAL_TIP_INVENTORYLITTLE,
	TUTORIAL_TIP_INVENTORYMOVE,
	TUTORIAL_TIP_INVENTORYSLOTS,
	TUTORIAL_TIP_INVENTORYRADIAL,
	TUTORIAL_TIP_INVENTORYRADIALEXPLAIN,
	TUTORIAL_TIP_INVENTORYCLOSE,
	TUTORIAL_TIP_ESC,
	TUTORIAL_TIP_HEALSTART,
	TUTORIAL_TIP_HEALREDORB,
	TUTORIAL_TIP_HEALEND,

	TUTORIAL_TIP_SKILLS,
	TUTORIAL_TIP_SKILLSBLUEORB,
	TUTORIAL_TIP_SKILLSOPEN,
	TUTORIAL_TIP_SKILLSLITTLE,
	TUTORIAL_TIP_SKILLSLATER,
	TUTORIAL_TIP_SKILLSCOME,
	TUTORIAL_TIP_SKILLSCLOSE,
	TUTORIAL_TIP_SHIFTSTART,
	TUTORIAL_TIP_SHIFTCONTEXT,
	TUTORIAL_TIP_SHIFTSPRINT,
	TUTORIAL_TIP_SHIFTCOOLDOWN,
	TUTORIAL_TIP_SHIFTEND,
	TUTORIAL_TIP_HOTBARSTART,
	TUTORIAL_TIP_HOTBARINFO,
	TUTORIAL_TIP_HOTBAR1AND2,
	TUTORIAL_TIP_HOTBARPRESS1AND2,
	TUTORIAL_TIP_HOTBAREND,

	TUTORIAL_TIP_MURMURSTART,
	TUTORIAL_TIP_MURMURICON,
	TUTORIAL_TIP_MURMURICONPURPLE,
	TUTORIAL_TIP_MURMURICONYELLOW,
	TUTORIAL_TIP_MURMURICONQUESTION,
	TUTORIAL_TIP_MURMURICONOTHER,
	TUTORIAL_TIP_MURMURSPEAK,
	TUTORIAL_TIP_KNIGHTFIND,
	TUTORIAL_TIP_KNIGHTTALK,
	TUTORIAL_TIP_KNIGHTGET,
	TUTORIAL_TIP_KNIGHTTAKEITEMS,
	TUTORIAL_TIP_HOLBORNSTART,
	TUTORIAL_TIP_HOLBORNWHERE,
	TUTORIAL_TIP_HOLBORNMAPOPEN,
	TUTORIAL_TIP_HOLBORNMAPABOUT,
	TUTORIAL_TIP_HOLBORNMAPCLOSE,
	TUTORIAL_TIP_PORTAL,
	TUTORIAL_TIP_PORTALOPENED,
	TUTORIAL_TIP_PORTALCLOSED,
	TUTORIAL_TIP_PORTALMAP,
	TUTORIAL_TIP_PORTALFIND,
	TUTORIAL_TIP_PORTALEXIT,
	TUTORIAL_TIP_END,

	NUM_TUTORIAL_TIPS
};

#define TUTORIAL_TIP_LONG_DURATION				( 5 * GAME_TICKS_PER_SECOND )
#define TUTORIAL_TIP_STANDARD_DURATION			( 4 * GAME_TICKS_PER_SECOND )
#define TUTORIAL_TIP_SHORT_DURATION				( 3 * GAME_TICKS_PER_SECOND )
#define TUTORIAL_TIP_QUICK_DURATION				( 2 * GAME_TICKS_PER_SECOND )
#define TUTORIAL_TIP_SHORTEN_DURATION			( 1 * GAME_TICKS_PER_SECOND )
#define TUTORIAL_TIP_NO_DURATION				( -1 )

static TUTORIAL_DATA sgTutorialTipList[ NUM_TUTORIAL_TIPS ] =
{
	//	Global String						next	length								skip if flag set				wait on flag clear				shorten duration on flag set
	{ GS_TUTORIALTIPSTARTWELCOME,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPSTARTABOUT,				TRUE,	TUTORIAL_TIP_SHORT_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPSTARTMANUAL,			TRUE,	TUTORIAL_TIP_SHORT_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPVIDEOSTART,				TRUE,	TUTORIAL_TIP_SHORT_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_VIDEO_ENTER },
	{ GS_TUTORIALTIPVIDEOOPEN,				TRUE,	TUTORIAL_TIP_NO_DURATION,			TUTORIAL_INPUT_VIDEO_ENTER,		TUTORIAL_INPUT_VIDEO_ENTER,		TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPVIDEOMURMUR,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_VIDEO_CLOSE },
	{ GS_TUTORIALTIPVIDEOWHO,				TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_VIDEO_CLOSE },
	{ GS_TUTORIALTIPVIDEOREAD,				TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_VIDEO_CLOSE },
	{ GS_TUTORIALTIPVIDEOARROW,				TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_VIDEO_CLOSE },
	{ GS_TUTORIALTIPVIDEOCOLORED,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_VIDEO_CLOSE },
	{ GS_TUTORIALTIPVIDEOACCEPT,			FALSE,	TUTORIAL_TIP_NO_DURATION,			TUTORIAL_INPUT_VIDEO_CLOSE,		TUTORIAL_INPUT_VIDEO_CLOSE,		TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPVIDEODONE,				TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPQUESTLOGSTART,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_QUESTLOG },
	{ GS_TUTORIALTIPQUESTLOGOPEN,			TRUE,	TUTORIAL_TIP_NO_DURATION,			TUTORIAL_INPUT_QUESTLOG,		TUTORIAL_INPUT_QUESTLOG,		TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPQUESTLOGUP,				TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_QUESTLOG_CLOSE },
	{ GS_TUTORIALTIPQUESTLOGUPCLICK,		TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_QUESTLOG_CLOSE },
	{ GS_TUTORIALTIPQUESTLOGTRACK,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_QUESTLOG_CLOSE },
	{ GS_TUTORIALTIPQUESTLOGTRACKCLICK,		TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_QUESTLOG_CLOSE },
	{ GS_TUTORIALTIPQUESTLOGTRACKDISPLAY,	TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_QUESTLOG_CLOSE },
	{ GS_TUTORIALTIPQUESTLOGCLOSE,			TRUE,	TUTORIAL_TIP_NO_DURATION,			TUTORIAL_INPUT_QUESTLOG_CLOSE,	TUTORIAL_INPUT_QUESTLOG_CLOSE,	TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPQUESTLOGEND,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPAUTOMAPPRIMER,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_AUTOMAP },
	{ GS_TUTORIALTIPAUTOMAPOPEN,			TRUE,	TUTORIAL_TIP_NO_DURATION,			TUTORIAL_INPUT_AUTOMAP,			TUTORIAL_INPUT_AUTOMAP,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPAUTOMAPICONS,			TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPMOVEFORWARD,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_MOVE_FORWARD,	TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPMOVESIDE,				TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_MOVE_SIDES,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPMOVEBACK,				TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_MOVE_BACK,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPMOVEMOUSELOOK,			TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPMOVEZOOM,				TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPMOVEJUMP,				TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_SPACEBAR,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPMOVEEND,				TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPFIGHTSTART,				TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_LEFTBUTTON },
	{ GS_TUTORIALTIPFIGHTPRACTICE,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_LEFTBUTTON,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPFIGHTLEFTBUTTON,		TRUE,	TUTORIAL_TIP_NO_DURATION,			TUTORIAL_INPUT_LEFTBUTTON,		TUTORIAL_INPUT_LEFTBUTTON,		TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPFIGHTLATER,				TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPFIGHTFORNOW,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPFIGHTCROSSHAIRS,		TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPFIGHTENEMYINFO,			TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPFIGHTREDHUD,			TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPFIGHTGREENHUD,			TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPFIGHTKILL,				FALSE,	TUTORIAL_TIP_NO_DURATION,			TUTORIAL_INPUT_FIRST_KILL,		TUTORIAL_INPUT_FIRST_KILL,		TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPFIGHTCONGRATS,			TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPFIGHTCRATES,			TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPGETSTART,				TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPGETMETHOD,				TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_FIRST_ITEM,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPGETALT,					TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_FIRST_ITEM,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPGETALTPICKUP,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_FIRST_ITEM,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPGETFPICKUP,				TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_FIRST_ITEM,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPGETEND,					FALSE,	TUTORIAL_TIP_NO_DURATION,			TUTORIAL_INPUT_FIRST_ITEM,		TUTORIAL_INPUT_FIRST_ITEM,		TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPINVENTORYSTART,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPINVENTORYOPEN,			TRUE,	TUTORIAL_TIP_NO_DURATION,			TUTORIAL_INPUT_INVENTORY,		TUTORIAL_INPUT_INVENTORY,		TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPINVENTORYLITTLE,		TRUE,	TUTORIAL_TIP_SHORT_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_INVENTORY_CLOSE },
	{ GS_TUTORIALTIPINVENTORYMOVE,			TRUE,	TUTORIAL_TIP_LONG_DURATION,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_INVENTORY_CLOSE },
	{ GS_TUTORIALTIPINVENTORYSLOTS,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_INVENTORY_CLOSE },
	{ GS_TUTORIALTIPINVENTORYRADIAL,		TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_INVENTORY_CLOSE },
	{ GS_TUTORIALTIPINVENTORYRADIALEXPLAIN,	TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_INVENTORY_CLOSE },
	{ GS_TUTORIALTIPINVENTORYCLOSE,			TRUE,	TUTORIAL_TIP_NO_DURATION,			TUTORIAL_INPUT_INVENTORY_CLOSE,	TUTORIAL_INPUT_INVENTORY_CLOSE,	TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPESC,					TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPHEALSTART,				TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPHEALREDORB,				TRUE,	TUTORIAL_TIP_SHORT_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPHEALEND,				TRUE,	TUTORIAL_TIP_SHORT_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },

	{ GS_TUTORIALTIPSKILLS,					TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPSKILLSBLUEORB,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPSKILLSOPEN,				TRUE,	TUTORIAL_TIP_NO_DURATION,			TUTORIAL_INPUT_SKILLS,			TUTORIAL_INPUT_SKILLS,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPSKILLSLITTLE,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_SKILLS_CLOSE },
	{ GS_TUTORIALTIPSKILLSLATER,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_SKILLS_CLOSE },
	{ GS_TUTORIALTIPSKILLSCOME,				TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_SKILLS_CLOSE },
	{ GS_TUTORIALTIPSKILLSCLOSE,			TRUE,	TUTORIAL_TIP_NO_DURATION,			TUTORIAL_INPUT_SKILLS_CLOSE,	TUTORIAL_INPUT_SKILLS_CLOSE,	TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPSHIFTSTART,				TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPSHIFTCONTEXT,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPSHIFTSPRINT,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPSHIFTCOOLDOWN,			TRUE,	TUTORIAL_TIP_SHORT_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPSHIFTEND,				TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPHOTBARSTART,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPHOTBARINFO,				TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPHOTBAR1AND2,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPHOTBARPRESS1AND2,		TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPHOTBAREND,				TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },

	{ GS_TUTORIALTIPMURMURSTART,			FALSE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_APPROACH_MURMUR,	TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPMURMURICON,				TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_TALK_MURMUR },
	{ GS_TUTORIALTIPMURMURICONPURPLE,		TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_TALK_MURMUR },
	{ GS_TUTORIALTIPMURMURICONYELLOW,		TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_TALK_MURMUR },
	{ GS_TUTORIALTIPMURMURICONQUESTION,		TRUE,	TUTORIAL_TIP_SHORT_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_TALK_MURMUR },
	{ GS_TUTORIALTIPMURMURICONOTHER,		TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_TALK_MURMUR },
	{ GS_TUTORIALTIPMURMURSPEAK,			FALSE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_TALK_MURMUR,		TUTORIAL_INPUT_TALK_MURMUR,		TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPKNIGHTFIND,				FALSE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_APPROACH_KNIGHT,	TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPKNIGHTTALK,				TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPKNIGHTGET,				TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPKNIGHTTAKEITEMS,		FALSE,	TUTORIAL_TIP_NO_DURATION,			TUTORIAL_INPUT_OFFER_DONE,		TUTORIAL_INPUT_OFFER_DONE,		TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPHOLBORNSTART,			TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPHOLBORNWHERE,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPHOLBORNMAPOPEN,			TRUE,	TUTORIAL_TIP_NO_DURATION,			TUTORIAL_INPUT_WORLDMAP,		TUTORIAL_INPUT_WORLDMAP,		TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPHOLBORNMAPABOUT,		TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_WORLDMAP_CLOSE },
	{ GS_TUTORIALTIPHOLBORNMAPCLOSE,		TRUE,	TUTORIAL_TIP_NO_DURATION,			TUTORIAL_INPUT_WORLDMAP_CLOSE,	TUTORIAL_INPUT_WORLDMAP_CLOSE,	TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPPORTAL,					TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPPORTALOPENED,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPPORTALCLOSED,			TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPPORTALMAP,				TRUE,	TUTORIAL_TIP_STANDARD_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPPORTALFIND,				FALSE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_APPROACH_PORTAL,	TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPPORTALEXIT,				TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
	{ GS_TUTORIALTIPEND,					TRUE,	TUTORIAL_TIP_QUICK_DURATION,		TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE,			TUTORIAL_INPUT_NONE },
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_TUTORIAL * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_TUTORIAL *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSendNextTutorialTip( QUEST * pQuest )
{
	if ( !UnitIsInTutorial( pQuest->pPlayer ) )
		return;

	QUEST_DATA_TUTORIAL * pQuestData = sQuestDataGet( pQuest );
	ASSERT_RETURN( pQuestData->bTipActive == FALSE );

	UNIT * pPlayer = pQuest->pPlayer;
	int index = UnitGetStat( pPlayer, STATS_TUTORIAL, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ) );
	ASSERTX_RETURN( index >= 0, "Tutorial tip index not valid" );
	ASSERT_RETURN( pQuestData->nDestinationTip >= index );

	if ( index >= NUM_TUTORIAL_TIPS )
		return;

	int nDuration = sgTutorialTipList[index].nDuration;
	if ( ( sgTutorialTipList[index].nShortenFlag != TUTORIAL_INPUT_NONE ) && ( nDuration != TUTORIAL_TIP_NO_DURATION ) )
	{
		DWORD dwMask = MAKE_MASK( sgTutorialTipList[index].nShortenFlag );
		if ( pQuestData->dwTipFlags & dwMask )
			nDuration -= TUTORIAL_TIP_SHORTEN_DURATION;
	}

	if ( ( nDuration == TUTORIAL_TIP_NO_DURATION ) && ( sgTutorialTipList[index].nWaitFlag != TUTORIAL_INPUT_NONE ) )
	{
		DWORD dwMask = MAKE_MASK( sgTutorialTipList[index].nWaitFlag );
		if ( pQuestData->dwTipFlags & dwMask )
			nDuration = TUTORIAL_TIP_STANDARD_DURATION;
	}

	MSG_SCMD_UISHOWMESSAGE tMessage;
	tMessage.bType = (BYTE)QUIM_TUTORIAL_TIP;
	tMessage.nDialog = sgTutorialTipList[index].nString;
	tMessage.nParam1 = nDuration;
	tMessage.nParam2 = index;
	if ( nDuration == TUTORIAL_TIP_NO_DURATION )
		tMessage.bFlags = (BYTE)UISMS_NOFADE;
	else
		tMessage.bFlags = (BYTE)UISMS_FADEOUT;

	// send message
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendMessage( UnitGetGame( pPlayer ), idClient, SCMD_UISHOWMESSAGE, &tMessage );

	pQuestData->bTipActive = TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// the server is setting which tip to end at

static void sSetDestinationTip( QUEST * pQuest, int nTip )
{
	if ( !UnitIsInTutorial( pQuest->pPlayer ) )
		return;

	// advance to the end of the tip sequence
	while ( ( nTip < NUM_TUTORIAL_TIPS ) && sgTutorialTipList[nTip].bNextTip )
		nTip++;

	QUEST_DATA_TUTORIAL * pQuestData = sQuestDataGet( pQuest );
	if ( nTip > pQuestData->nDestinationTip )
	{
		pQuestData->nDestinationTip = nTip;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSetPlayerTipIndex( UNIT * pPlayer, QUEST_DATA_TUTORIAL * pQuestData, int index )
{
	if ( AppIsDemo() && ( index == TUTORIAL_TIP_STARTMANUAL ) )
		index++;
	if ( index > NUM_TUTORIAL_TIPS )
		return NUM_TUTORIAL_TIPS;
	BOOL bDone = FALSE;
	while ( ( index < NUM_TUTORIAL_TIPS ) && !bDone )
	{
		if ( sgTutorialTipList[index].nSkipFlag != TUTORIAL_INPUT_NONE )
		{
			DWORD dwMask = MAKE_MASK( sgTutorialTipList[index].nSkipFlag );
			if ( pQuestData->dwTipFlags & dwMask )
				index++;
			else
				bDone = TRUE;
		}
		else
		{
			bDone = TRUE;
		}
	}
	UnitSetStat( pPlayer, STATS_TUTORIAL, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ), index );
	return index;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTryContinueTips( QUEST * pQuest, int nInput )
{
	UNIT * pPlayer = pQuest->pPlayer;
	QUEST_DATA_TUTORIAL * pQuestData = sQuestDataGet( pQuest );
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	if ( nDifficulty != DIFFICULTY_NORMAL )
		return;
	int index = UnitGetStat( pPlayer, STATS_TUTORIAL, nDifficulty );
	DWORD dwInputMask = MAKE_MASK( nInput );
	pQuestData->dwTipFlags |= dwInputMask;
	if ( sgTutorialTipList[index].nWaitFlag == nInput )
	{
		// we are going forward with the next one!
		pQuestData->bTipActive = FALSE;
		// advance...
		sSetDestinationTip( pQuest, index + 1 );
		sSetPlayerTipIndex( pPlayer, pQuestData, index + 1 );
		sSendNextTutorialTip( pQuest );
	}
}

////----------------------------------------------------------------------------
////----------------------------------------------------------------------------
//
//#define FLYIN_TIME			8.0f
//
//static VECTOR sgvFlyInNodes[7];
//
////----------------------------------------------------------------------------
////----------------------------------------------------------------------------
//static BOOL sFlyInDone( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
//{
//	ASSERT_RETFALSE( game && unit );
//	ASSERT( IS_CLIENT( game ) );
//	UnitClearFlag( unit, UNITFLAG_QUESTSTORY );
//	UISetCinematicMode(FALSE);
//
//	ROOM * room = UnitGetRoom( unit );
//	if ( room )
//	{
//		ROOM_LAYOUT_GROUP * nodeset = RoomGetLabelNode( room, "FlyInNodes" );
//		ASSERT( nodeset );
//		ROOM_LAYOUT_GROUP * node = s_QuestNodeSetGetNode( nodeset, 7 );
//		ASSERT( node );
//		float fAngle;
//		s_QuestNodeGetPosAndDir( room, node, NULL, NULL, &fAngle, NULL );
//		c_PlayerSetAngles( game, fAngle, 0.0f );
//	}
//
//	c_CameraSetViewType( VIEW_3RDPERSON );
//	AppCommonSetStepSpeed(APP_STEP_SPEED_DEFAULT);
//	QUEST * pQuest = ( QUEST * )tEventData.m_Data1;
//	QuestStateSet( pQuest, QUEST_STATE_TUTORIAL_WELCOME, QUEST_STATE_COMPLETE );
//	return TRUE;
//}
//
////----------------------------------------------------------------------------
////----------------------------------------------------------------------------
//static void sHandleFlyIn( GAME * game, QUEST * pQuest )
//{
//	ASSERT_RETURN( IS_CLIENT( game ) );
//	UNIT * player = GameGetControlUnit( game );
//	ASSERT_RETURN( player );
//	ROOM * room = UnitGetRoom( player );
//	ASSERT_RETURN( room );
//
//	ROOM_LAYOUT_GROUP * nodeset = RoomGetLabelNode( room, "FlyInNodes" );
//	ASSERT_RETURN( nodeset );
//	for ( int i = 0; i < 7; i++ )
//	{
//		ROOM_LAYOUT_GROUP * node = s_QuestNodeSetGetNode( nodeset, i+1 );
//		ASSERT_RETURN( node );
//		s_QuestNodeGetPosAndDir( room, node, &sgvFlyInNodes[i], NULL, NULL, NULL );
//	}
//	UnitSetFlag( player, UNITFLAG_QUESTSTORY );
//	c_CameraFollowPath( game, &sgvFlyInNodes[0], 7, FLYIN_TIME );
//	AppCommonSetStepSpeed(APP_STEP_SPEED_DEFAULT + 1);		// 1/2 speed
//	UnitRegisterEventTimed( player, sFlyInDone, EVENT_DATA((DWORD_PTR)pQuest), (int)FLOOR( ( FLYIN_TIME * GAME_TICKS_PER_SECOND_FLOAT ) + 0.5f ) );
//}
//
////----------------------------------------------------------------------------
////----------------------------------------------------------------------------
//static void sAbortFlyIn( GAME * game, QUEST * pQuest )
//{
//	ASSERT_RETURN( IS_CLIENT( game ) );
//	UNIT * player = GameGetControlUnit( game );
//	if ( !player )
//		return;
//
//	if ( UnitHasTimedEvent( player, sFlyInDone ) )
//	{
//		UnitUnregisterTimedEvent( player, sFlyInDone );
//		EVENT_DATA event_data;
//		event_data.m_Data1 = (DWORD_PTR)pQuest;
//		sFlyInDone( game, player, event_data );
//	}
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTransitionLevel( 
	QUEST * pQuest,
	const QUEST_MESSAGE_ENTER_LEVEL * pMessage)
{
	if ( QuestIsComplete( pQuest ) )
		return QMR_OK;

	UNIT * player = pQuest->pPlayer;
	GAME * game = UnitGetGame( player );
	ASSERT_RETVAL( IS_SERVER( game ), QMR_OK );

	QUEST_DATA_TUTORIAL * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelTutorial )
	{
		if ( s_QuestIsPrereqComplete( pQuest ) )
		{
			if ( !QuestIsActive( pQuest ) )
			{
				s_QuestActivate( pQuest );
			}
			// init tips
			UNIT * pPlayer = pQuest->pPlayer;
			UnitSetStat( pPlayer, STATS_TUTORIAL, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ), 0 );
			QUEST_DATA_TUTORIAL * pQuestData = sQuestDataGet( pQuest );
			pQuestData->nDestinationTip = -1;
			pQuestData->dwTipFlags = 0;
			sSetDestinationTip( pQuest, TUTORIAL_TIP_STARTWELCOME );
			sSendNextTutorialTip( pQuest );
			s_NPCVideoStart( game, player, GI_MONSTER_MURMUR_TUTORIAL, NPC_VIDEO_INSTANT_INCOMING, DIALOG_TUTORIAL_MURMUR_TRANSMISSION );
			// re-init quest states
			s_QuestClearAllStates( pQuest, TRUE );
			// remove all quest items
			s_QuestRemoveItem( pQuest->pPlayer, GlobalIndexGet( GI_ITEM_FAWKES_MESSAGE ) );
			return QMR_OK;
		}
	}

	if ( pMessage->nLevelNewDef == pQuestData->nLevelHolbornStation )
	{
		if ( !QuestIsActive( pQuest ) )
		{
			s_QuestForceActivate( pQuest->pPlayer, QUEST_TUTORIAL );
		}

		QuestStateSet( pQuest, QUEST_STATE_TUTORIAL_TRAVEL, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_TUTORIAL_FIND_MURMUR, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_TUTORIAL_FIRST_KILL, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_TUTORIAL_SEEK_TEMPLAR, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_TUTORIAL_TALK_MURMUR, QUEST_STATE_COMPLETE );
		s_QuestComplete( pQuest );
		return QMR_OK;
	}

	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );
	if ( eStatus != QUEST_STATUS_ACTIVE )
		return QMR_OK;

	QUEST_DATA_TUTORIAL * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	// quest active
	if (UnitIsMonsterClass( pSubject, pQuestData->nMurmur ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TUTORIAL_FIND_MURMUR ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		if ( ( LevelGetDefinitionIndex( UnitGetLevel( pQuest->pPlayer ) ) == pQuestData->nLevelHolbornStation ) &&
			 ( QuestStateGetValue( pQuest, QUEST_STATE_TUTORIAL_TALK_MURMUR ) == QUEST_STATE_ACTIVE ) )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_FINISHED;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nHurtTemplar ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TUTORIAL_SEEK_TEMPLAR ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_OK;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	QUEST_DATA_TUTORIAL * pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nMurmur ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TUTORIAL_FIND_MURMUR ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TUTORIAL_MURMUR_TALK );
			return QMR_NPC_TALK;
		}
		if ( ( LevelGetDefinitionIndex( UnitGetLevel( pQuest->pPlayer ) ) == pQuestData->nLevelHolbornStation ) &&
			 ( QuestStateGetValue( pQuest, QUEST_STATE_TUTORIAL_TALK_MURMUR ) == QUEST_STATE_ACTIVE ) )
		{
			//s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TUTORIAL_MURMUR_TALK );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nHurtTemplar ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TUTORIAL_SEEK_TEMPLAR ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TUTORIAL_TEMPLAR_TALK, INVALID_LINK, GI_INVALID, TRUE );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sChangeMurmurAI( QUEST * pQuest, UNIT * npc )
{
	QUEST_DATA_TUTORIAL * pQuestData = sQuestDataGet( pQuest );
	UNIT * player = pQuest->pPlayer;
	ASSERT_RETURN( player );
	GAME * game = UnitGetGame( player );
	ASSERT_RETURN( game );
	ASSERT_RETURN( npc );

	ASSERT_RETURN( UnitGetClass( npc ) == pQuestData->nMurmur );

	int ai = UnitGetStat( npc, STATS_AI );
	if ( ai == pQuestData->nFollowAI )
		return;

	// found!
	AI_Free( game, npc );
	UnitSetStat( npc, STATS_AI, pQuestData->nFollowAI );
	UnitClearFlag( npc, UNITFLAG_FACE_DURING_INTERACTION );
	UnitSetAITarget( npc, player, TRUE );
	AI_Init( game, npc );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasNotes(
	void *pUserData)
{
	QUEST *pQuest = (QUEST *)pUserData;

	QuestStateSet( pQuest, QUEST_STATE_TUTORIAL_SEEK_TEMPLAR, QUEST_STATE_COMPLETE );
	QuestStateSet( pQuest, QUEST_STATE_TUTORIAL_TRAVEL, QUEST_STATE_ACTIVE );

	QUEST_DATA_TUTORIAL * pQuestData = sQuestDataGet( pQuest );
	UNIT * pPlayer = pQuest->pPlayer;
	s_QuestOpenWarp( UnitGetGame( pPlayer ), pPlayer, pQuestData->nObjectWarpRS_H );

	s_QuestMapReveal( pQuest, GI_LEVEL_HOLBORN_STATION );
	s_QuestMapReveal( pQuest, GI_LEVEL_CG_APPROACH );
	s_QuestMapReveal( pQuest, GI_LEVEL_COVENT_GARDEN_STATION );

	sTryContinueTips( pQuest, TUTORIAL_INPUT_OFFER_DONE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sVideoDone( QUEST * pQuest )
{
	QuestStateSet( pQuest, QUEST_STATE_TUTORIAL_FIND_MURMUR, QUEST_STATE_ACTIVE );
	QuestStateSet( pQuest, QUEST_STATE_TUTORIAL_FIRST_KILL, QUEST_STATE_ACTIVE );
	sTryContinueTips( pQuest, TUTORIAL_INPUT_VIDEO_CLOSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );
	int nDialogStopped = pMessage->nDialog;
	
	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_TUTORIAL_MURMUR_TRANSMISSION:
		{
			sVideoDone( pQuest );
			return QMR_FINISHED;
		}

		//----------------------------------------------------------------------------
		case DIALOG_TUTORIAL_MURMUR_TALK:
		{
			QuestStateSet( pQuest, QUEST_STATE_TUTORIAL_FIND_MURMUR, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TUTORIAL_SEEK_TEMPLAR, QUEST_STATE_ACTIVE );
			sTryContinueTips( pQuest, TUTORIAL_INPUT_TALK_MURMUR );
			sChangeMurmurAI( pQuest, pTalkingTo );
			return QMR_FINISHED;
		}

		//----------------------------------------------------------------------------
		case DIALOG_TUTORIAL_TEMPLAR_TALK:
		{
			// setup actions
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sHasNotes;
			tActions.pAllTakenUserData = pQuest;

			// offer to player
			s_OfferGive( pQuest->pPlayer, pTalkingTo, OFFERID_QUEST_TUTORIAL_NOTES, tActions );

			return QMR_OFFERING;
		}

		//----------------------------------------------------------------------------
		// drb - there needs to be 1 more conversation here. not sure what happened. i thought i talked to ivan about this
		// but there doesn't seem to be one. anyhow, this is where the quest completes. When it does complete, it should
		// auto-force all the states (kill 1st zombie) to complete beforehand.
		//case DIALOG_TUTORIAL_MURMUR_COMPLETE:
		//{
		//}
	}
	return QMR_OK;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRetryNPCVideoStart(QUEST *pQuest, GAME* pGame, UNIT* pPlayer)
{
	if ( QuestIsComplete( pQuest ) )
		return FALSE;

	ASSERT_RETFALSE( IS_SERVER( pGame ) );

	QUEST_DATA_TUTORIAL * pQuestData = sQuestDataGet( pQuest );

	// recheck any assumptions from initial video start code
	if ( UnitGetLevelDefinitionIndex( pPlayer ) == pQuestData->nLevelTutorial )
	{
		if ( s_QuestIsPrereqComplete( pQuest ) )
		{
			// don't bother restarting the video if they finished it
			if ( QuestStateGetValue( pQuest, QUEST_STATE_TUTORIAL_FIND_MURMUR ) == QUEST_STATE_HIDDEN )
			{
				s_NPCVideoStart( pGame, pPlayer, GI_MONSTER_MURMUR_TUTORIAL, NPC_VIDEO_INSTANT_INCOMING, DIALOG_TUTORIAL_MURMUR_TRANSMISSION );
				return TRUE;
			}
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkCancelled(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_CANCELLED *pMessage )
{
	int nDialogStopped = pMessage->nDialog;
	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_TUTORIAL_MURMUR_TRANSMISSION:
		{
			sVideoDone( pQuest );
		}
		break;
	}
	return QMR_OK;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_TUTORIAL_REWARDS			5

static QUEST_MESSAGE_RESULT sQuestMessageMonsterKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_KILL *pMessage)
{
	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	QUEST_DATA_TUTORIAL * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );

	if (pVictim &&
		QuestStateGetValue( pQuest, QUEST_STATE_TUTORIAL_FIRST_KILL ) == QUEST_STATE_ACTIVE && 
		UnitIsMonsterClass( pVictim, pQuestData->nTutorialZombie ))
	{
		QuestStateSet( pQuest, QUEST_STATE_TUTORIAL_FIRST_KILL, QUEST_STATE_COMPLETE );
		int nTreasureClass = pQuestData->nTreasureTutorialFirst;
		ASSERT( nTreasureClass != INVALID_LINK );
		ITEM_SPEC tSpec;
		tSpec.unitOperator = pQuest->pPlayer;
		tSpec.guidRestrictedTo = UnitGetGuid( pQuest->pPlayer );
		SETBIT( tSpec.dwFlags, ISF_PLACE_IN_WORLD_BIT );
		SETBIT( tSpec.dwFlags, ISF_IDENTIFY_BIT );
		SETBIT( tSpec.dwFlags, ISF_RESTRICTED_TO_GUID_BIT );
		UNIT * pRewards[ MAX_TUTORIAL_REWARDS ];
		ITEMSPAWN_RESULT tResult;
		tResult.nTreasureBufferSize = MAX_TUTORIAL_REWARDS;
		tResult.pTreasureBuffer = pRewards;
		s_TreasureSpawnClass( pVictim, nTreasureClass, &tSpec, &tResult );
		ASSERT( tResult.nTreasureBufferCount == 1 );
		pQuestData->idFirstItem = UnitGetId( tResult.pTreasureBuffer[0] );
		sTryContinueTips( pQuest, TUTORIAL_INPUT_FIRST_KILL );
		return QMR_OK;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessagePlayerApproach(
	QUEST *pQuest,
	const QUEST_MESSAGE_PLAYER_APPROACH *pMessage )
{
	if (QuestIsActive( pQuest ))
	{
		QUEST_DATA_TUTORIAL * pQuestData = sQuestDataGet( pQuest );
		UNIT *pTarget = UnitGetById( QuestGetGame( pQuest ), pMessage->idTarget );

		if ( UnitGetGenus( pTarget ) == GENUS_MONSTER )
		{
			int unit_class = UnitGetClass( pTarget );
			if ( unit_class == pQuestData->nTutorialZombie )
			{
				if ( QuestStateGetValue( pQuest, QUEST_STATE_TUTORIAL_FIRST_KILL ) != QUEST_STATE_ACTIVE )
				{
					QuestStateSet( pQuest, QUEST_STATE_TUTORIAL_FIND_MURMUR, QUEST_STATE_ACTIVE );
					QuestStateSet( pQuest, QUEST_STATE_TUTORIAL_FIRST_KILL, QUEST_STATE_ACTIVE );
				}
				return QMR_OK;
			}
			if ( unit_class == pQuestData->nMurmur )
			{
				UNIT * pPlayer = pQuest->pPlayer;
				if ( ( UnitGetStat( pPlayer, STATS_TUTORIAL, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ) ) <= TUTORIAL_TIP_MURMURICON ) &&
					 ( pQuestData->nDestinationTip <= TUTORIAL_TIP_MURMURICON ) &&
					 ( QuestStateGetValue( pQuest, QUEST_STATE_TUTORIAL_FIND_MURMUR ) >= QUEST_STATE_ACTIVE ) )
				{
					sTryContinueTips( pQuest, TUTORIAL_INPUT_APPROACH_MURMUR );
				}
				if ( QuestStateGetValue( pQuest, QUEST_STATE_TUTORIAL_FIND_MURMUR ) != QUEST_STATE_ACTIVE )
				{
					QuestStateSet( pQuest, QUEST_STATE_TUTORIAL_FIND_MURMUR, QUEST_STATE_ACTIVE );
					QuestStateSet( pQuest, QUEST_STATE_TUTORIAL_FIRST_KILL, QUEST_STATE_ACTIVE );
				}
				return QMR_OK;
			}
			if ( unit_class == pQuestData->nHurtTemplar )
			{
				UNIT * pPlayer = pQuest->pPlayer;
				if ( ( UnitGetStat( pPlayer, STATS_TUTORIAL, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ) ) <= TUTORIAL_TIP_KNIGHTTALK ) &&
					 ( pQuestData->nDestinationTip <= TUTORIAL_TIP_KNIGHTTALK ) &&
					 ( QuestStateGetValue( pQuest, QUEST_STATE_TUTORIAL_SEEK_TEMPLAR ) >= QUEST_STATE_ACTIVE ) )
				{
					sTryContinueTips( pQuest, TUTORIAL_INPUT_APPROACH_KNIGHT );
				}
			}
		}

		if ( UnitGetGenus( pTarget ) == GENUS_OBJECT )
		{
			if ( UnitGetClass( pTarget ) == pQuestData->nObjectWarpRS_H )
			{
				UNIT * pPlayer = pQuest->pPlayer;
				if ( ( UnitGetStat( pPlayer, STATS_TUTORIAL, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ) ) <= TUTORIAL_TIP_PORTALEXIT ) &&
					 ( pQuestData->nDestinationTip <= TUTORIAL_TIP_PORTALEXIT ) &&
					 ( QuestStateGetValue( pQuest, QUEST_STATE_TUTORIAL_TRAVEL ) >= QUEST_STATE_ACTIVE ) )
				{
					sTryContinueTips( pQuest, TUTORIAL_INPUT_APPROACH_PORTAL );
				}
			}
			return QMR_FINISHED;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageCanOperateObject( 
	QUEST *pQuest,
	const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pObject = UnitGetById( pGame, pMessage->idObject );
	QUEST_DATA_TUTORIAL *pQuestData = sQuestDataGet( pQuest );

	if ( UnitIsObjectClass( pObject, pQuestData->nObjectWarpH_RS ) )
	{
		// never go back!
		return QMR_OPERATE_FORBIDDEN;
	}

	if ( UnitIsObjectClass( pObject, pQuestData->nObjectWarpRS_H ) )
	{
		if ( QuestGetStatus( pQuest ) >= QUEST_STATUS_COMPLETE )
			return QMR_OPERATE_OK;

		if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
			return QMR_OPERATE_FORBIDDEN;

		if ( QuestStateGetValue( pQuest, QUEST_STATE_TUTORIAL_TRAVEL ) >= QUEST_STATE_ACTIVE )
			return QMR_OPERATE_OK;

		return QMR_OPERATE_FORBIDDEN;
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSetTipState( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETURN( pPlayer );
	UNITID idPlayer = UnitGetId( pPlayer );

	if ( ( UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ) <= DIFFICULTY_NORMAL ) &&
		 ( QuestGetStatus( pQuest ) < QUEST_STATUS_COMPLETE ) &&
		 ( UnitGetStat( pPlayer, STATS_NO_TUTORIAL_TIPS ) == 0 ) )
	{
		s_StateSet( pPlayer, pPlayer, STATE_TUTORIAL_TIPS_ACTIVE, 0 );
	}
	else
	{
		s_StateClear( pPlayer, idPlayer, STATE_TUTORIAL_TIPS_ACTIVE, 0 );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageQuestStatus( 
	QUEST *pQuest,
	const QUEST_MESSAGE_STATUS *pMessage)
{
	UNIT * pPlayer = pQuest->pPlayer;

	if ( IS_CLIENT( UnitGetGame( pPlayer ) ) )
		return QMR_OK;

	sSetTipState( pQuest );

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT s_sQuestMessagePickup( 
	QUEST* pQuest,
	const QUEST_MESSAGE_PICKUP* pMessage)
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	UNITID idPickup = UnitGetId( pMessage->pPickedUp );
	QUEST_DATA_TUTORIAL * pQuestData = sQuestDataGet( pQuest );

	if ( pQuestData->idFirstItem == idPickup )
	{
		sTryContinueTips( pQuest, TUTORIAL_INPUT_FIRST_ITEM );
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageReloadUI( 
	QUEST* pQuest )
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;
	
	// resend the tip, if there was one displaying
	QUEST_DATA_TUTORIAL * pQuestData = sQuestDataGet( pQuest );

	if ( !pQuestData->bTipActive )
		return QMR_OK;

	UNIT *pPlayer = QuestGetPlayer( pQuest );
	int index = UnitGetStat( pPlayer, STATS_TUTORIAL, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ) );
	if ( pQuestData->nDestinationTip >= index )
	{
		pQuestData->bTipActive = FALSE;
		sSendNextTutorialTip( pQuest );
	}

	// display the video again, if they haven't finished with it
	sRetryNPCVideoStart( pQuest, UnitGetGame( pPlayer ), pPlayer );
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageHandler(
	QUEST *pQuest,
	QUEST_MESSAGE_TYPE eMessageType,
	const void *pMessage)
{
	switch (eMessageType)
	{

		//----------------------------------------------------------------------------
		case QM_SERVER_INTERACT_INFO:
		{
			const QUEST_MESSAGE_INTERACT_INFO *pHasInfoMessage = (const QUEST_MESSAGE_INTERACT_INFO *)pMessage;
			return sQuestMessageUnitHasInfo( pQuest, pHasInfoMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_INTERACT:
		{
			const QUEST_MESSAGE_INTERACT *pInteractMessage = (const QUEST_MESSAGE_INTERACT *)pMessage;
			return sQuestMessageInteract( pQuest, pInteractMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_TALK_STOPPED:
		{
			const QUEST_MESSAGE_TALK_STOPPED *pTalkStoppedMessage = (const QUEST_MESSAGE_TALK_STOPPED *)pMessage;
			return sQuestMessageTalkStopped( pQuest, pTalkStoppedMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_TALK_CANCELLED:
		{
			const QUEST_MESSAGE_TALK_CANCELLED *pTalkCancelledMessage = (const QUEST_MESSAGE_TALK_CANCELLED *)pMessage;
			return sQuestMessageTalkCancelled( pQuest, pTalkCancelledMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pTransitionMessage = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;
			return sQuestMessageTransitionLevel( pQuest, pTransitionMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_MONSTER_KILL:
		{
			const QUEST_MESSAGE_MONSTER_KILL *pMonsterKillMessage = (const QUEST_MESSAGE_MONSTER_KILL *)pMessage;
			return sQuestMessageMonsterKill( pQuest, pMonsterKillMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PLAYER_APPROACH:
		{
			const QUEST_MESSAGE_PLAYER_APPROACH *pPlayerApproachMessage = (const QUEST_MESSAGE_PLAYER_APPROACH *)pMessage;
			return sQuestMessagePlayerApproach( pQuest, pPlayerApproachMessage );
		}

		//----------------------------------------------------------------------------
		case QM_CAN_OPERATE_OBJECT:
		{
			const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pCanOperateObjectMessage = (const QUEST_MESSAGE_CAN_OPERATE_OBJECT *)pMessage;
			return sQuestMessageCanOperateObject( pQuest, pCanOperateObjectMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PICKUP:
		{
			const QUEST_MESSAGE_PICKUP *pMessagePickup = (const QUEST_MESSAGE_PICKUP*)pMessage;
			return s_sQuestMessagePickup( pQuest, pMessagePickup);
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_QUEST_STATUS:
		case QM_CLIENT_QUEST_STATUS:
		{
			const QUEST_MESSAGE_STATUS * pMessageStatus = ( const QUEST_MESSAGE_STATUS * )pMessage;
			return sQuestMessageQuestStatus( pQuest, pMessageStatus );
		}	

		//----------------------------------------------------------------------------
		case QM_SERVER_RELOAD_UI:
			return sQuestMessageReloadUI( pQuest );
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeTutorial(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	
	// free quest data
	ASSERTX_RETURN( pQuest->pQuestData, "Expected quest data" );
	GFREE( UnitGetGame( pQuest->pPlayer ), pQuest->pQuestData );
	pQuest->pQuestData = NULL;	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestDataInit(
	QUEST *pQuest, 
	QUEST_DATA_TUTORIAL * pQuestData)
{
	pQuestData->nMurmur					= QuestUseGI( pQuest, GI_MONSTER_MURMUR_TUTORIAL );
	pQuestData->nHurtTemplar			= QuestUseGI( pQuest, GI_MONSTER_TUTORIAL_TEMPLAR );
	pQuestData->nTutorialZombie			= QuestUseGI( pQuest, GI_MONSTER_TUTORIAL_ZOMBIE );
	pQuestData->nZombie					= QuestUseGI( pQuest, GI_MONSTER_ZOMBIE );
	pQuestData->nLevelHolbornStation	= QuestUseGI( pQuest, GI_LEVEL_HOLBORN_STATION );
	pQuestData->nLevelTutorial			= QuestUseGI( pQuest, GI_LEVEL_RUSSELL_SQUARE );
	pQuestData->nFollowAI				= QuestUseGI( pQuest, GI_AI_TECH_FOLLOW );
	pQuestData->nTreasureTutorialFirst	= QuestUseGI( pQuest, GI_TREASURE_TUTORIAL_FIRSTDROP );
	pQuestData->nObjectWarpH_RS			= QuestUseGI( pQuest, GI_OBJECT_WARP_HOLBORN_RUSSELLSQUARE );
	pQuestData->nObjectWarpRS_H			= QuestUseGI( pQuest, GI_OBJECT_WARP_RUSSELLSQUARE_HOLBORN );

	pQuestData->idFirstItem				= INVALID_ID;
	pQuestData->bTipActive = FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitTutorial(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	

	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		

	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_TUTORIAL * pQuestData = ( QUEST_DATA_TUTORIAL * )GMALLOCZ( pGame, sizeof( QUEST_DATA_TUTORIAL ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nMurmur );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nHurtTemplar );

	// quest message flags
	pQuest->dwQuestMessageFlags |= MAKE_MASK( QSMF_APPROACH_RADIUS );

	int index = UnitGetStat( pQuest->pPlayer, STATS_TUTORIAL, UnitGetStat( pQuest->pPlayer, STATS_DIFFICULTY_CURRENT ) );
	if ( ( index >= 0 ) && ( index < NUM_TUTORIAL_TIPS ) )
	{
		pQuestData->nDestinationTip = index;
	}
	else
	{
		pQuestData->nDestinationTip = 0;
	}

	UNIT * pPlayer = pQuest->pPlayer;
	if ( IS_SERVER( UnitGetGame( pPlayer ) ) )
	{
		sSetTipState( pQuest );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// in uix.cpp
BOOL sTutorialFadeClear( GAME * pGame, UNIT * pUnit, const struct EVENT_DATA & event_data );

#define TUTORIALTIP_FADEOUT_TIME		1.0f

//----------------------------------------------------------------------------

void c_TutorialSetClientMessage( UNIT * player, int tip_index )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN( player );
	ASSERT_RETURN( tip_index >= 0 );
	ASSERT_RETURN( tip_index < NUM_TUTORIAL_TIPS );

	// only if the state is set on the player
	if ( !UnitIsInTutorial( player ) )
		return;

	// check if player has no tips enabled
	if ( UnitGetStat( player, STATS_NO_TUTORIAL_TIPS ) == 1 )
		return;

	// get quest info
	QUEST * pQuest = QuestGetQuest( player, QUEST_TUTORIAL );
	ASSERT_RETURN( pQuest );
	QUEST_DATA_TUTORIAL * pQuestData = sQuestDataGet( pQuest );
	ASSERT_RETURN( pQuest );

	pQuestData->client_tip_index = tip_index;
#endif
}
//----------------------------------------------------------------------------

void c_TutorialInputOk( UNIT * player, int input )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN( player );
	ASSERT_RETURN( input >= 0 );
	ASSERT_RETURN( input < NUM_TUTORIAL_INPUTS );

	// only if the state is set on the player
	if ( !UnitIsInTutorial( player ) )
		return;

	// check if player has no tips enabled
	if ( UnitGetStat( player, STATS_NO_TUTORIAL_TIPS ) == 1 )
		return;

	// get quest info
	QUEST * pQuest = QuestGetQuest( player, QUEST_TUTORIAL );
	ASSERT_RETURN( pQuest );
	QUEST_DATA_TUTORIAL * pQuestData = sQuestDataGet( pQuest );
	ASSERT_RETURN( pQuest );

	// only if the player is on the right level
	LEVEL * pLevel = UnitGetLevel( player );
	if ( LevelGetDefinitionIndex( pLevel ) != pQuestData->nLevelTutorial )
		return;

	int nDifficulty = UnitGetStat( player, STATS_DIFFICULTY_CURRENT );
	if ( nDifficulty != DIFFICULTY_NORMAL )
		return;

	// set flag on tip
	int index = UnitGetStat( player, STATS_TUTORIAL, nDifficulty );

	// send flags
	DWORD dwInputMask = MAKE_MASK( input );
	if ( ( pQuestData->dwTipFlags & dwInputMask ) == 0 )
	{
		pQuestData->dwTipFlags |= dwInputMask;
		MSG_CCMD_TUTORIAL_UPDATE message;
		message.nType = TUTORIAL_MSG_INPUT;
		message.nData = input;
		c_SendMessage( CCMD_TUTORIAL_UPDATE, &message );
	}

	if ( sgTutorialTipList[index].nWaitFlag == input )
	{
		if ( UnitHasTimedEvent( player, sTutorialFadeClear ) )
		{
			UnitUnregisterTimedEvent( player, sTutorialFadeClear );			
		}
		UIFadeOutQuickMessage( TUTORIALTIP_FADEOUT_TIME );
		UnitRegisterEventTimed( player, sTutorialFadeClear, EVENT_DATA( index ), (int)FLOOR( TUTORIALTIP_FADEOUT_TIME * GAME_TICKS_PER_SECOND ) );
	}

#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_TutorialTipsOff( UNIT * pPlayer )
{
	if ( IS_CLIENT( UnitGetGame( pPlayer ) ) )
		return;

	UNITID idPlayer = UnitGetId( pPlayer );

	s_StateClear( pPlayer, idPlayer, STATE_TUTORIAL_TIPS_ACTIVE, 0 );
	UnitSetStat( pPlayer, STATS_NO_TUTORIAL_TIPS, 1 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_TutorialUpdate( UNIT * player, int type, int data )
{
	ASSERTX_RETURN( player, "Expected player" );
	QUEST * pQuest = QuestGetQuest( player, QUEST_TUTORIAL );
	ASSERTX_RETURN( pQuest, "Tutorial not found on player" );
	QUEST_DATA_TUTORIAL * pQuestData = sQuestDataGet( pQuest );
	ASSERT_RETURN( pQuestData );

	if ( type == TUTORIAL_MSG_INPUT )
	{
		// set key as pressed
		ASSERTX_RETURN( ( ( data >= TUTORIAL_INPUT_VIDEO_ENTER ) && ( data < NUM_TUTORIAL_INPUTS ) ), "Invalid data sent from client during tutorial" );
		DWORD dwMask = MAKE_MASK( data );
		pQuestData->dwTipFlags |= dwMask;
	}

	if ( type == TUTORIAL_MSG_TIP_DONE )
	{
		if ( !pQuestData->bTipActive )
			return;

		int nDifficulty = UnitGetStat( player, STATS_DIFFICULTY_CURRENT );
		if ( nDifficulty != DIFFICULTY_NORMAL )
			return;

		int index = UnitGetStat( player, STATS_TUTORIAL, nDifficulty );
		if ( sgTutorialTipList[index].nWaitFlag != TUTORIAL_INPUT_NONE )
		{
			DWORD dwMask = MAKE_MASK( sgTutorialTipList[index].nWaitFlag );
			if ( ( pQuestData->dwTipFlags & dwMask ) == 0 )
				return;
			// advance...
			sSetDestinationTip( pQuest, index + 1 );
		}
	
		pQuestData->bTipActive = FALSE;
		if ( data >= index )
		{
			index = sSetPlayerTipIndex( player, pQuestData, index + 1 );
			if ( index > pQuestData->nDestinationTip )
				sSetDestinationTip( pQuest, index );
			sSendNextTutorialTip( pQuest );
		}
	}
}
