//----------------------------------------------------------------------------
// FILE: gossip.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifdef __GOSSIP_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define __GOSSIP_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct UNIT;
enum QUEST_STATUS;
enum UNIT_INTERACT;

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct GOSSIP_DATA 
{
	int nNPCMonsterClass;
	int nDialog;
	int nQuest;
	int nQuestTugboat;
	int nPlayerLevel;
	int nPriority;
	QUEST_STATUS eQuestStatus;
};

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

const GOSSIP_DATA *GossipGetData(
	int nGossip);

UNIT_INTERACT s_GossipStart(
	UNIT *pPlayer,
	UNIT *pNPC);

BOOL s_GossipHasGossip(
	UNIT *pPlayer,
	UNIT *pNPC);
	
#endif  // end __GOSSIP_H_