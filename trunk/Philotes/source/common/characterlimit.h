#ifndef _CHARACTERLIMIT_H_
#define _CHARACTERLIMIT_H_

//NOTE: The server side of this information is stored in the database in tbl_conf.
// Make sure that gets changed when changing these
#define MAX_CHARACTERS_PER_ACCOUNT 24
//#define MAX_CHARACTERS_PER_NON_SUBSCRIBER 3

#define MAX_CHARACTERS_PER_SINGLE_PLAYER 40

#define MAX_CHARACTERS_PER_GAME_SERVER 4096

enum CHARACTER_SLOT_TYPE
{
	CHARACTER_SLOT_FREE,
	CHARACTER_SLOT_SUBSCRIPTION,
	CHARACTER_SLOT_LAST
};


#endif