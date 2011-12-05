//----------------------------------------------------------------------------
// FILE:
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#ifndef __s_QUESTNPCSERVER_H_
#define __s_QUESTNPCSERVER_H_
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef __QUEST_H_
#include "Quest.h"
#endif


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
//STATIC FUNCTIONS and Defines

//----------------------------------------------------------------------------
// structs
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------


int s_QuestNPCGetSecondaryDialogInActiveTask( GAME *pGame,
											  UNIT *pPlayer,
											  UNIT *pNPC,
											  const QUEST_TASK_DEFINITION_TUGBOAT *pReturnsQuestTask );


QUEST_MESSAGE_RESULT s_QuestTalkToNPC( UNIT *pPlayer,
									   UNIT *pNPC,
									   int nQuestID );

BOOL s_QuestUpdateQuestNPCs( GAME *pGame,
							 UNIT *pPlayer,
							 UNIT *pNPC );

int s_QuestGiveLevelAreaForTalkingWithNPC( UNIT *pPlayer,
										   UNIT *pNPC,
										   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );



//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------

#endif
