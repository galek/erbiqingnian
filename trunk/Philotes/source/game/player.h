//----------------------------------------------------------------------------
// player.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _PLAYER_H_
#define _PLAYER_H_

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _EXCEL_H_
#include "excel.h"
#endif

#ifndef _UNITS_H_
#include "units.h"
#endif

#ifndef __INTERACT_H_
#include "interact.h"
#endif

#ifndef _UNIT_PRIV_H_
#include "unit_priv.h"
#endif

#ifndef __S_GAMEMAPSSERVER_H_
#include "s_GameMapsServer.h"
#endif

#ifndef _COMMONCHATMESSAGES_H_
#include "ServerSuite/Chat/CommonChatMessages.h"
#endif

#ifndef __UNITFILE_H_
#include "unitfile.h"
#endif

#include "ServerSuite/Database/DatabaseGame.h"

//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------
extern BOOL gbSkipTutorial;

//----------------------------------------------------------------------------
// FORWARD REFERENCES
//----------------------------------------------------------------------------
enum UI_ELEMENT;
struct UI_COMPONENT;
enum NPC_INTERACT_TYPE;
struct CLIENT_SAVE_FILE;
class MYTHOS_POINTSOFINTEREST::cPointsOfInterest;

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
#define PLAYER_HEIGHT							1.8f
#define PLAYER_SPEED							1.0f
#define DEFAULT_PLAYER_NAME						L"Marcus Fidelius"
#define MAX_PLAYER_CHARACTER_LEVEL				999
#define	NUM_MINIGAME_SLOTS						3
#define MAX_RESPECS								-1
//----------------------------------------------------------------------------
enum
{
	PLAYER_MOVE_NONE =			0x00000000,
	PLAYER_MOVE_FORWARD =		0x00000001,
	PLAYER_MOVE_BACK =			0x00000002,
	PLAYER_STRAFE_LEFT =		0x00000004,
	PLAYER_STRAFE_RIGHT =		0x00000008,
	PLAYER_ROTATE_LEFT =		0x00000010,
	PLAYER_ROTATE_RIGHT =		0x00000020,
	PLAYER_JUMP =				0x00000040,
	PLAYER_AUTORUN =			0x00000080,
	PLAYER_CROUCH =				0x00000100,

#if (ISVERSION(DEVELOPMENT))
	CAMERA_MOVE_UP =			0x00010000,
	CAMERA_MOVE_DOWN =			0x00020000,
#endif
	PLAYER_HOLD	 =				0x00040000,

	PLAYER_MOVE_FORWD_AND_BACK =			(PLAYER_MOVE_FORWARD | PLAYER_MOVE_BACK),
	PLAYER_MOVE_LEFT_AND_RIGHT =			(PLAYER_STRAFE_LEFT | PLAYER_STRAFE_RIGHT),
	PLAYER_ROTATE_LEFT_AND_RIGHT =			(PLAYER_ROTATE_LEFT | PLAYER_ROTATE_RIGHT),
};

const enum ERESPAWN_TYPE
{
	KRESPAWN_TYPE_PRIMARY,
	KRESPAWN_TYPE_ANCHORSTONE,
	KRESPAWN_TYPE_RETURNPOINT,
	KRESPAWN_TYPE_PLAYERFIRSTLOAD,
	KRESPAWN_COUNT
};

//----------------------------------------------------------------------------
enum
{
	CLASSLEVEL_PROPERTIES =				4,
	MAX_INIT_ITEMS_PER_CLASS =			10,
};

//----------------------------------------------------------------------------
enum PLAYER_CONSTANTS
{
	MAX_HEADSTONES = 1,
};

//----------------------------------------------------------------------------
enum PLAYER_RESPAWN
{
	PLAYER_RESPAWN_INVALID = -1,
	
	PLAYER_RESPAWN_TOWN,			// restart in town
	PLAYER_RESPAWN_GHOST,			// restart level as ghost
	PLAYER_RESPAWN_RESURRECT,		// resurrect on the spot
	PLAYER_RESPAWN_RESURRECT_FREE,	// resurrect on the spot (for free) - used when the player gets kicked back to town while in the resurrect menu
	PLAYER_RESPAWN_ARENA,			// restart in arena (mini game)
	PLAYER_REVIVE_GHOST,			// player a ghost, but can't get to headstone, revive their ghost in town
	PLAYER_RESPAWN_PVP_RANDOM,		// restart at a random spawn location 
	PLAYER_RESPAWN_PVP_FARTHEST,	// restart at the farthest spawn location from opponent player
	PLAYER_RESPAWN_METAGAME_CONTROLLED,	// let the current metagame decide where to respawn the player

	NUM_PLAYER_RESPAWN_TYPES
};

const BOOL g_bClientCanRequestRespawn[NUM_PLAYER_RESPAWN_TYPES] =
{
	TRUE,	// PLAYER_RESPAWN_TOWN
	TRUE,	// PLAYER_RESPAWN_GHOST
	TRUE,	// PLAYER_RESPAWN_RESURRECT
	FALSE,	// PLAYER_RESPAWN_RESURRECT_FREE
	TRUE,	// PLAYER_RESPAWN_ARENA
	TRUE,	// PLAYER_REVIVE_GHOST
	FALSE,	// PLAYER_RESPAWN_PVP_RANDOM
	FALSE,	// PLAYER_RESPAWN_PVP_FARTHEST
	FALSE	// PLAYER_RESPAWN_METAGAME_CONTROLLED
};

//----------------------------------------------------------------------------
enum NEW_PLAYER_FLAGS
{
	NPF_HARDCORE			= MAKE_MASK(0),
	NPF_DISABLE_TUTORIAL	= MAKE_MASK(1),
	NPF_LEAGUE				= MAKE_MASK(2),
	NPF_ELITE				= MAKE_MASK(3),
	NPF_PVPONLY				= MAKE_MASK(4),
};

//----------------------------------------------------------------------------
enum MINIGAME_TYPE
{
	MINIGAME_DMG_TYPE,
	MINIGAME_KILL_TYPE,
	MINIGAME_CRITICAL,
	MINIGAME_QUEST_FINISH,
	MINIGAME_ITEM_TYPE,
	MINIGAME_ITEM_MAGIC_TYPE,
};

//----------------------------------------------------------------------------
enum UNIT_COLLECTION_TYPE
{
	UCT_INVALID = -1,
	
	UCT_CHARACTER,			// load all character data which includes a complete inventory
	UCT_ITEM_COLLECTION,	// a collection of one or more items, each with possibly items inside them
	
	UCT_NUM_TYPES			// keep this last
};

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
////////////////////////////////////////////////////////
#define RM_MAXLENGTH    1024
#define RM_MAGICNUMBER  'HMGR'

#pragma pack(push)
#pragma pack(1)
struct RICH_GAME_MEDIA_HEADER
{
    DWORD       dwMagicNumber;
    DWORD       dwHeaderVersion;
    DWORD       dwHeaderSize;
    LARGE_INTEGER liThumbnailOffset;
    DWORD       dwThumbnailSize;
    GUID        guidGameId;
    WCHAR       szGameName[RM_MAXLENGTH];
    WCHAR       szSaveName[RM_MAXLENGTH];
    WCHAR       szLevelName[RM_MAXLENGTH];
    WCHAR       szComments[RM_MAXLENGTH];
};
#pragma pack(pop)
/////////////////////////////////////////////////////////
struct PLAYERLEVEL_DATA
{
	int			nUnitType;
	int			nLevel;
	int			nExperience;
	int			nHp;
	int			nMana;  //Marsh Added
	int			nAttackRating;
	int			nSfxDefenseRating;
	int			nAIChangerAttack;
	int			nStatPoints;
	int			nSkillPoints;
	int			nCraftingPoints;
	int			nStatRespecCost;
	int			nPowerCostPercent;
	int			nHeadstoneReturnCost;
	int			nDeathExperiencePenaltyPercent;
	int			nRestartHealthPercent;
	int			nRestartPowerPercent;
	int			nRestartShieldPercent;
	PCODE		codeProperty[CLASSLEVEL_PROPERTIES];
	int			nSkillGroup[MAX_SPELL_SLOTS_PER_TIER];					

};

struct PLAYERRANK_DATA
{
	int			nUnitType;
	int			nLevel;
	int			nRankExperience;
	int			nPerkPoints;
};

struct PLAYER_STYLE_DATA
{
	char		szName[ DEFAULT_INDEX_SIZE ];
	int			nSkillTab;
	int			nUnittype;
};

struct PLAYER_RACE_DATA
{
	char		szName[ DEFAULT_INDEX_SIZE ];
	int			nDisplayString;
	WORD		wCode;										


};

struct PLAYER_GUILD_MEMBER_DATA
{
	PGUID		idMemberCharacterId;
	WCHAR		wszMemberName[MAX_CHARACTER_NAME];
	BOOL		bIsOnline;
	GUILD_RANK  eGuildRank;
	COMMON_CLIENT_DATA tMemberClientData;
};




//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
//////////////////////////////////////////////
BYTE * RichSaveFileRemoveHeader(
	BYTE* data, DWORD* size);
BOOL ConvertStringToGUID(
	const WCHAR* strIn, GUID* pGuidOut );

//////////////////////////////////////////////
BOOL ExcelClassLevelsPostProcess(
	struct EXCEL_TABLE * table);

BOOL ExcelClassRanksPostProcess(
	struct EXCEL_TABLE * table);

BOOL ExcelPlayersPostProcess(
	struct EXCEL_TABLE * table);
	
void c_PlayerInitForGame(
	GAME *pGame);

void c_PlayerCloseForGame(
	GAME *pGame);

void c_PlayerAdd(
	GAME *pGame,
	UNIT *pPlayer);

//returns the last room message the player saw
int c_PlayerGetLastRoomMessage();
//displays a room message if it hasn't already
void c_PlayerDisplayRoomMessage( int nRoomID,
								 int nMessage );
//warping into a new level or levelarea on the client. This 
//gets called right before the load screen goes away
void c_PlayerEnteringNewLevelAndFloor( GAME *pGame,
									   UNIT *pPlayer,
									   int LevelDef,
									   int DRLGDef,
									   int LevelAreaID,
									   int LevelAreaDepth );

void s_PlayerRemove( GAME *pGame,
					 UNIT *pPlayer);
void c_PlayerRemove(
	GAME *pGame,
	UNIT *pPlayer);

PGUID c_PlayerGetMyGuid(
	void);

BOOL CharacterDigestSaveToDatabase(
	UNIT * unit );

int PlayerGetAvailableSkillsOfGroup( GAME * pGame,
									 UNIT * pUnit,
									 int nSkillGroup );

int PlayerGetKnownSkillsOfGroup( GAME * pGame,
								 UNIT * pUnit,
								 int nSkillGroup );

int PlayerGetNextFreeSpellSlot( GAME * pGame,
								UNIT * pUnit,
								int nSkillGroup,
								int nSkillID );

int PlayerGetPlayerClass(
	int nPlayerUnit);

int PlayerGetPlayerClass(
	UNIT *pPlayer);
			
BOOL PlayerNameExists(
	const WCHAR * name );

//----------------------------------------------------------------------------
enum PLAYER_LOAD_FLAGS
{
	PLF_FOR_CHARACTER_SELECTION,		// player load is for character selection screen (digest)
};

//----------------------------------------------------------------------------
enum PLAYER_LOAD_TYPE
{
	PLT_INVALID = -1,
	
	PLT_CHARACTER,		// the entire character data (used when entering game)
	PLT_ITEM_TREE,		// just a specific item tree such as 1 item with its mods
	PLT_ACCOUNT_UNITS,	// a collection of account items (like the entire shared stash slot)
	PLT_ITEM_RESTORE,	// an item restore initiated by a customer service rep
	PLT_AH_ITEMSALE_SEND_TO_AH,   // an item about to get sent to AH

	PLT_NUM_TYPES
		
};

UNIT * PlayerLoad(
	GAME * game,
	CLIENT_SAVE_FILE *pClientSaveFile,
	DWORD dwPlayerLoadFlags,			// see PLAYER_LOAD_FLAGS
	GAMECLIENTID idClient);

void PlayerLoadFromDatabase(
	APPCLIENTID idAppClient,
	WCHAR * wszCharName,
	UNIQUE_ACCOUNT_ID idCharAccount = INVALID_UNIQUE_ACCOUNT_ID);

BOOL PlayerLoadUnitCollection(
	UNIT *pPlayer,
	CLIENT_SAVE_FILE *pClientSaveFile);

void PlayerLoadAccountUnitsFromDatabase(
	UNIT *pPlayer);
		
void PlayerLoadUnitTreeFromDatabase(
	APPCLIENTID idAppClient,
	PGUID guidDestPlayer,
	const WCHAR *puszCharacter,
	PGUID guidUnitRoot,
	UNIQUE_ACCOUNT_ID idCharAccount = INVALID_UNIQUE_ACCOUNT_ID,
	PLAYER_LOAD_TYPE eLoadType = PLT_ITEM_TREE);

BOOL PlayerRestoreItemFromWarehouseDatabase(
	PGUID guidPlayer,
	PGUID guidItem,
	ASYNC_REQUEST_ID idRequest);
	
UNIT * s_PlayerNew(
	GAME * game,
	GAMECLIENTID idClient,
	const WCHAR* name,
	int nClass,
	struct WARDROBE_INIT * pWardrobeInit,
	struct APPEARANCE_SHAPE * pAppearanceShape,
	DWORD dwNewPlayerFlags);		// see NEW_PLAYER_FLAGS

BOOL s_PlayerAdd(
	GAME * game,
	struct GAMECLIENT * client,
	UNIT * unit);

BOOL PlayerSaveToFile(
	UNIT * unit);

BOOL PlayerSaveByteBufferToFile(
	WCHAR * szUnitName,	
	BYTE * SaveFile, 
	DWORD SaveSize);

BOOL PlayerSaveToSendBuffer(
	UNIT * unit);

BOOL PlayerSaveToAppClientBuffer(
	UNIT * unit,
	DWORD key);

BOOL PlayerSaveToDatabase(
	UNIT * unit );

unsigned int PlayerSaveToBuffer(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	UNITSAVEMODE savemode,
	BYTE * data,
	unsigned int size);

BOOL UnitBatchedAddToDatabase(
	UNIT * unit,
	VOID * pDBUnitCollection,
	BOOL bNewPlayer = FALSE );

enum UNIT_DATABASE_CONTEXT
{
	UDC_NORMAL,				// a normal operation
	UDC_ITEM_RESTORE,		// an item doing a db operation in the context of "item restore"
};

BOOL UnitAddToDatabase(
	UNIT * unit,
	VOID * pDBUnit,
	BYTE * pDataBuffer,
	UNIT_DATABASE_CONTEXT eContext = UDC_NORMAL,
	struct DB_GAME_CALLBACK *pCallback = NULL);

BOOL UnitFullUpdateInDatabase(
	UNIT * unit,
	VOID * pDBUnit,
	BYTE * pDataBuffer );

BOOL UnitRemoveFromDatabase(
	PGUID unitId,
	PGUID ownerId,
	const UNIT_DATA *pUnitData);

BOOL UnitChangeLocationInDatabase(
	UNIT * unit,
	PGUID unitId,
	PGUID ownerId,
	PGUID containerId,
	INVENTORY_LOCATION * invLoc,
	DWORD dwInvLocationCode,
	BOOL bContainedItemGoingIntoSharedStash );

BOOL UnitUpdateExperienceInDatabase(
	PGUID unitId,
	PGUID ownerId,
	DWORD dwExperience );

BOOL UnitUpdateMoneyInDatabase(
	PGUID unitId,
	PGUID ownerId,
	DWORD dwMoney );

BOOL UnitUpdateQuantityInDatabase(
	PGUID unitId,
	PGUID ownerId,
	WORD wQuantity);

BOOL CharacterUpdateRankExperienceInDatabase(
	PGUID unitId,
	PGUID ownerId,
	DWORD dwRankExperience,
	DWORD dwRank );

void s_PlayerToggleHardcore(
	UNIT * unit,
	BOOL bForceOn);

void s_PlayerTogglePVPOnly(
	UNIT * unit,
	BOOL bForceOn);

void s_PlayerToggleLeague(
	UNIT * unit,
	BOOL bForceOn);

void s_PlayerToggleElite(
	UNIT * unit,
	BOOL bForceOn);

#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_PlayerRemoveFromParty(
	UNIT * unit);
#endif

//----------------------------------------------------------------------------
enum PLAYER_SPEC_FLAGS
{
	PSF_NO_DATABASE_OPERATIONS =	MAKE_MASK(0),
	PSF_IS_CONTROL_UNIT =			MAKE_MASK(1),
};

//----------------------------------------------------------------------------
struct PLAYER_SPEC
{
	GAMECLIENTID idClient;
	UNITID idUnit;
	PGUID guidUnit;
	int nPlayerClass;
	int nTeam;
	ROOM *pRoom;
	VECTOR vPosition;
	VECTOR vDirection;
	int nAngle;
	WARDROBE_INIT *pWardrobeInit;
	APPEARANCE_SHAPE *pAppearanceShape;
	DWORD dwFlags;		// see PLAYER_SPEC_FLAGS
	
	PLAYER_SPEC::PLAYER_SPEC( void )
		:	idClient( INVALID_GAMECLIENTID ),
			idUnit( INVALID_ID ),
			guidUnit( INVALID_GUID ),
			nPlayerClass( INVALID_LINK ),
			nTeam( INVALID_LINK ),
			pRoom( NULL ),
			vPosition( cgvNone ),
			vDirection( cgvNone ),
			nAngle( 0 ),
			pWardrobeInit( NULL ),
			pAppearanceShape( NULL ),
			dwFlags( 0 )
	{ }
	
};

UNIT * PlayerInit(
	GAME *game,
	PLAYER_SPEC &tSpec);
	
VECTOR c_PlayerGetDirectionFromInput(
	int nInput );
	
/*
void c_PlayerGetView(
	GAME * pGame, 
	struct VECTOR * pvPosition, 
	int * pnAngle, 
	int * pnPitch);
*/
void s_PlayerGetView(
	GAME * pGame,
	int * pnX, 
	int * pnY);

int c_PlayerGetMovement(
	GAME* game );

BOOL c_PlayerTestMovement(
	int movement_flag);

void c_PlayerSetMovement(
	GAME * game,
	int movement_flag);

void c_PlayerClearMovement(
	GAME * game,
	int movement_flag);

void c_PlayerClearAllMovement(
	GAME * game);

void c_PlayerToggleMovement(
	GAME * game,
	int movement_flag);

struct ROOM * c_PlayerGetRoom(
	GAME * pGame);

void PlayerGetXYZ(
	GAME * pGame,
	VECTOR * pvPosition);

void c_PlayerSetAngles(
	GAME * pGame,
	int angle,
	float fPitch=0.0f );

void c_PlayerSetAngles(
	GAME * pGame,
	float angle,
	float fPitch=0.0f );

void c_PlayerGetAngles(
	GAME * pGame,
	int * pnAngle,
	int * pnPitch);

void c_PlayerGetAngles(
   GAME * pGame,
   float * pnAngle,
   float * pnPitch);

void PlayerSetMovementAndAngle(
	GAME * game,
	GAMECLIENTID idClient,
	const VECTOR & vFaceDirection,
	const VECTOR pvWeaponPos[ MAX_WEAPONS_PER_UNIT ],
	const VECTOR pvWeaponDir[ MAX_WEAPONS_PER_UNIT ],
	const VECTOR & vMoveDir);

void s_DrawPlayer(
	GAME * pGame,
	int nWorldX,
	int nWorldY);

void PlayerSetVelocity(
	float fVelocity);

void PlayerMouseRotate( 
	GAME * game,
	float delta);

void PlayerMousePitch(
	GAME * game,
	float delta);

void PlayerResetCameraOrbit( 
	GAME * game );

void c_PlayerUpdateClientFX( GAME *pGame );

BOOL GameGetControlPosition(
	GAME * pGame,
	VECTOR * pvPosition,
	float * pfAngle,
	float * pfPitch);

BOOL PlayerDoLevelUp(
	UNIT * unit,
	int nNewLevel);

BOOL PlayerDoRankUp(
	UNIT * unit,
	int nNewRank);

BOOL PlayerCheckLevelUp(
	UNIT * unit);

BOOL PlayerCheckRankUp(
	UNIT * unit);

BOOL c_PlayerCanPath();

void PlayerGiveSkillPoint(
	UNIT *pPlayer,
	int nNumSkillPoints);

void PlayerGiveCraftingPoint(
	UNIT *pPlayer,
	int nNumCraftingPoints);

void PlayerGiveStatPoints(
	UNIT * pPlayer,
	int nNumStatPoints );

void PlayerGivePerkPoint(
	UNIT *pPlayer,
	int nNumPerkPoints);

//----------------------------------------------------------------------------
enum UI_ELEMENT_ACTION
{
	UIEA_OPEN,
	UIEA_CLOSE,
};

void s_PlayerToggleUIElement(
	UNIT *pPlayer,
	UI_ELEMENT eElement,
	UI_ELEMENT_ACTION eAction);

void s_PlayerShowWorldMap(
	UNIT *pPlayer,
	EMAP_ATLAS_MESSAGE Result,
	int nArea );
	
void s_PlayerShowRecipePane( UNIT *pPlayer,						     
						     int nRecipeID );


void PlayerRotationInterpolating(
	GAME * game,
	float fTimeDelta);

// DRB_3RD_PERSON
void PlayerFaceTarget(
	GAME * game,
	UNIT * target );

#if !ISVERSION( CLIENT_ONLY_VERSION )

BOOL s_UnitPutInLimbo(
	UNIT * unit,
	BOOL bSafeOnExit);
	
BOOL s_UnitTakeOutOfLimbo(
	UNIT * unit);
	
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

BOOL s_PlayerWarpLevels(
	UNIT* unit,
	LEVELID idLevelDest,
	LEVELID idLevelLeaving,
	WARP_SPEC &tWarpSpec);

int PlayerCountInLimbo(
	GAME *pGame);
	
void PlayerMovementUpdate(
	GAME *pGame,
	float fTimeDelta );

void c_PlayerMoveToMouse(
	GAME *pGame,
	BOOL bForce = FALSE );

void c_PlayerMoveToLocation(
	GAME *pGame,
	VECTOR* pvDestination, 
	BOOL bIgnoreKeys = FALSE );

void c_PlayerMouseAimLocation(
	GAME *pGame,
	VECTOR* pvDestination );

BOOL PlayerIsInAttackRange(
	GAME *pGame,
	UNIT *pTarget,
	int nSkill,
	BOOL bLineOfSight = FALSE );

void c_PlayerStopPath(
	GAME *pGame );

void c_PlayerMouseAim(
	GAME *pGame,
	BOOL bAimAtWeaponHeight = FALSE );


void c_PlayerInteract( 
	GAME *pGame, 
	UNIT *pTarget, 
	UNIT_INTERACT eInteract = UNIT_INTERACT_DEFAULT,
	BOOL bWarn = FALSE,
	PGUID guid = INVALID_GUID);

WCHAR * c_PlayerGetNameByGuid(
	PGUID guid);

UNIT * PlayerGetByName( 
	GAME *pGame,
	const WCHAR* szName);
	
void c_PlayerSetActiveSkill( GAME * game, int nSkillStat, char * pszName );

void c_PlayerInitControlUnit( UNIT * pUnit );

void AppResetDetachedCamera(
	void);

BOOL AppGetCameraPosition(
	VECTOR* pvPosition,
	float* pfAngle,
	float* pfPitch);

void AppDetachedCameraMouseRotate( 
	int delta);

void AppDetachedCameraMousePitch( 
	int delta);

void AppDetachedCameraSetMovement(
	int movementflag,
	BOOL value);

void AppDetachedCameraClearAllMovement();

void PlayerSetStartingMinigameTags(
	GAME * game,
	UNIT * unit);

#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL s_PlayerCheckMinigame(
	GAME * game, 
	UNIT * unit, 
	int nGoalCategory, 
	int nGoalType);

void s_PlayerRemoveInvalidMinigameEntries(
	GAME *game, 
	UNIT *unit);

#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)
	
void c_PlayerMoveMsg(
	GAME * game,
struct MSG_SCMD_PLAYERMOVE * msg);

void PlayerWarpToStartLocation(
	GAME *pGame,
	UNIT * unit);

VECTOR c_PlayerDoStep(
	GAME * pGame,
	UNIT * unit,
	float fTimeDelta,
	BOOL bRealStep );

void c_PlayerMove(
	GAME * pGame,
	UNIT * unit,
	float fTimeDelta,
	VECTOR & vNewPosition,
	BOOL bRealStep );

void s_PlayerRemoveItemFromWeaponConfigs(
	UNIT *pOwner,
	UNIT *pItem,
	int nHotkey = -1);

int PlayerDeleteOldVersions(
	void);

UNIT *LocalPlayerGet(
	void);

UNITID LocalPlayerGetID(
	void);

//----------------------------------------------------------------------------
// PARTY FUNCTIONS
//----------------------------------------------------------------------------

void c_PlayerRefreshPartyUI(
	void );

void c_PlayerSetParty(
	struct CHAT_PARTY_INFO * partyInfo );

void c_PlayerUpdatePartyInfo(
	struct CHAT_PARTY_INFO * partyInfo );

BOOL c_PlayerGetPartyInfo(
	CHAT_PARTY_INFO & partyInfo );

void c_PlayerUpdatePartyLeader(
	PGUID leaderGuid,
	const WCHAR * leaderName );

void c_PlayerSetPartyClientData(
	struct COMMON_PARTY_CLIENT_DATA * data );

struct COMMON_PARTY_CLIENT_DATA * c_PlayerGetPartyClientData(
	void );

BOOL c_PlayerClearParty(
	void );

CHANNELID c_PlayerGetPartyId(
	void );

PGUID c_PlayerGetPartyLeader(
	void );

void c_PlayerAddPartyInvite(
	PGUID idInviter );

PGUID c_PlayerGetPartyInviter(
	void );

void c_PlayerClearPartyInvites(
	void );

void c_PlayerAddGuildInvite(
	PGUID idInviter );

PGUID c_PlayerGetGuildInviter(
	void );

void c_PlayerClearGuildInvites(
	void );

void c_PlayerAddBuddyInvite(
	LPCWSTR szInviter );

WCHAR * c_PlayerGetBuddyInviter(
	void );

void c_PlayerClearBuddyInvites(
	void );

void c_PlayerAddPartyMember(
	struct CHAT_MEMBER_INFO * );

void c_PlayerUpdatePartyMember(
	struct CHAT_MEMBER_INFO * );

void c_PlayerRemovePartyMember(
	PGUID guidPlayer );

int c_PlayerGetPartyMemberCount(
	void );

int c_PlayerFindPartyMember(
	PGUID guidMember );

UNITID c_PlayerGetPartyMemberUnitId(
	UINT nMember,
	BOOL bSameGameOnly = TRUE);

PGUID c_PlayerGetPartyMemberUnitGuid(
	UINT nMember );

UNITID c_PlayerGetPartyMemberLevelArea(
	UINT nMember,
	BOOL bSameGameOnly  = TRUE );

UNITID c_PlayerGetPartyMemberPVPWorld(
	UINT nMember,
	BOOL bSameGameOnly  = TRUE );

UNITID c_PlayerGetPartyMemberLevelDepth(
	UINT nMember,
	BOOL bSameGameOnly  = TRUE );

PGUID c_PlayerGetPartyMemberGUID(
	UINT nMember);

GAMEID c_PlayerGetPartyMemberGameId(
	UINT nMember);

const WCHAR * c_PlayerGetPartyMemberName(
	UINT nMember);

DWORD c_PlayerGetPartyMemberClientData(
	UINT nMember);

//----------------------------------------------------------------------------
// GUILD FUNCTIONS
//----------------------------------------------------------------------------

void c_PlayerSetGuildInfo(
	CHANNELID guildChannelId,
	const WCHAR * guildName,
	GUILD_RANK eGuildRank );

void c_PlayerClearGuildInfo(
	void );

BOOL c_PlayerIsGuildMemberOnline(
	PGUID guid );

CHANNELID c_PlayerGetGuildChannelId(
	void );

const WCHAR * c_PlayerGetGuildName(
	void );

BOOL c_PlayerGetIsGuildLeader(
	void );

GUILD_RANK c_PlayerGetCurrentGuildRank(
	void );

void c_PlayerSetGuildSettingsInfo(
	const WCHAR * leaderRankName,
	const WCHAR * officerRankName,
	const WCHAR * memberRankName,
	const WCHAR * recruitRankName );

const WCHAR * c_PlayerGetRankStringForCurrentGuild(
	GUILD_RANK );

void c_PlayerSetGuildPermissionsInfo(
	GUILD_RANK promoteMinRank,
	GUILD_RANK demoteMinRank,
	GUILD_RANK emailMinRank,
	GUILD_RANK inviteMinRank,
	GUILD_RANK bootMinRank );

BOOL c_PlayerGetGuildPermissionsInfo(
	GUILD_RANK &promoteMinRank,
	GUILD_RANK &demoteMinRank,
	GUILD_RANK &emailMinRank,
	GUILD_RANK &inviteMinRank,
	GUILD_RANK &bootMinRank );

BOOL c_PlayerCanDoGuildAction(
	GUILD_ACTION action,
	GUILD_RANK targetPlayerRank = GUILD_RANK_INVALID);

void c_PlayerSetGuildLeaderInfo(
	PGUID idLeaderCharacterId,
	const WCHAR * wszLeaderName );

PGUID c_PlayerGetGuildLeaderCharacterId(
	void );

const WCHAR * c_PlayerGetGuildMemberNameByID(
	PGUID guid );

GUILD_RANK c_PlayerGetGuildMemberRankByID(
	PGUID guid );

const WCHAR * c_PlayerGetGuildLeaderName(
	void );

void c_PlayerClearGuildLeaderInfo(
	void );

void c_PlayerInitGuildMemberList(
	void );

void c_PlayerClearGuildMemberList(
	void );

void c_PlayerGuildMemberListAddOrUpdateMember(
	PGUID idMemberCharacterId,
	const WCHAR * wszMemberName,
	BOOL bIsOnline,
	GUILD_RANK rank );

void c_PlayerGuildMemberListAddOrUpdateMemberEx(
	PGUID idMemberCharacterId,
	const WCHAR * wszMemberName,
	BOOL bIsOnline,
	GUILD_RANK rank,
	COMMON_CLIENT_DATA & tMemberData );

void c_PlayerGuildMemberListRemoveMember(
	PGUID idMemberCharacterId,
	const WCHAR * wszMemberName );

DWORD c_PlayerGuildMemberListGetMemberCount(
	void );

//	pass NULL to get 1st member in list
const PLAYER_GUILD_MEMBER_DATA * c_PlayerGuildMemberListGetNextMember(
	const PLAYER_GUILD_MEMBER_DATA * previous,
	BOOL bHideOfflinePlayers = FALSE );

void s_PlayerBroadcastGuildAssociation(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit );

BOOL c_GuildInvite(
	LPCWSTR szName);

void c_GuildInviteAccept(
	PGUID guidInviter,
	void *);

void c_GuildInviteDecline(
	PGUID guidInviter,
	void *);

void c_GuildInviteIgnore(
	PGUID guidInviter,
	void *);

BOOL c_GuildLeave(
	void);

BOOL c_GuildKick(
	PGUID guid);

BOOL c_GuildChangeMemberRank(
	PGUID guid,
	GUILD_RANK newRank);

void c_GuildChangeRankName(
	GUILD_RANK rank,
	const WCHAR * rankName);

void c_GuildChangeActionPermissions(
	GUILD_ACTION guildAction,
	GUILD_RANK minimumRequiredRank );

GUILD_RANK c_PlayerGetGuildRankFromRankString(
	const WCHAR * rankString );

#if ISVERSION(DEVELOPMENT)
void c_GuildPrintSettings(void);
#endif

//----------------------------------------------------------------------------
// ACCESSOR FUNCTIONS
//----------------------------------------------------------------------------
inline const struct UNIT_DATA * PlayerGetData(
	GAME * game,
	int nClass)
{
	return (UNIT_DATA *)ExcelGetData(game, DATATABLE_PLAYERS, nClass);
}


inline const PLAYERLEVEL_DATA * PlayerLevelGetData(
	GAME * game,
	int nUnitType,
	int nLevel)
{
	PLAYERLEVEL_DATA key;
	key.nUnitType = nUnitType;
	key.nLevel    = nLevel;
	return (const PLAYERLEVEL_DATA *)ExcelGetDataByKey(game, DATATABLE_PLAYERLEVELS, &key, sizeof(key));
}

inline const PLAYERRANK_DATA * PlayerRankGetData(
	GAME * game,
	int nUnitType,
	int nRank)
{
	PLAYERRANK_DATA key;
	key.nUnitType = nUnitType;
	key.nLevel    = nRank;
	return (const PLAYERRANK_DATA *)ExcelGetDataByKey(game, DATATABLE_PLAYERRANKS, &key, sizeof(key));
}

inline int PlayerGetByCode(
	GAME * game,
	DWORD dwCode)
{
	return ExcelGetLineByCode(game, DATATABLE_PLAYERS, dwCode);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------



BOOL PlayerGetRespawnLocation( UNIT *pPlayer,	
							   ERESPAWN_TYPE type,
							   int &nLevelAreaDestinationID,
							   int &nLevelDepthDestination,
							   VECTOR &vPosition,
							   LEVEL *pLevel = NULL );

UNIT * PlayerGetRespawnMarker( UNIT *pPlayer,
							   LEVEL *pLevel,
							   VECTOR *pVector = NULL);

void PlayerSetRespawnLocation( ERESPAWN_TYPE,
							   UNIT *pPlayer,
							   UNIT *pTrigger,
							   LEVEL *pLevel );



UNIT *PlayerGetMostRecentHeadstone(
	UNIT *pPlayer);
	
int PlayerGetHeadstones(	
	UNIT *pUnit, 
	UNIT **pHeadstoneBuffer, 
	int nMaxHeadstones);

void PlayerSetHeadstoneLevelDef(
	UNIT *pUnit,
	int nLevelDef);

int PlayerGetLastHeadstoneLevelDef(
	UNIT *pUnit);

BOOL PlayerHasHeadstone( 
	UNIT *pUnit);

cCurrency PlayerGetResurrectCost(
	UNIT *pUnit);
	
cCurrency PlayerGetReturnToHeadstoneCost( 
	UNIT *pUnit);

cCurrency PlayerGetReturnToDungeonCost( 
	UNIT *pUnit);

cCurrency PlayerGetGuildCreationCost( 
	UNIT *pUnit);

cCurrency PlayerGetRespecCost( 
	UNIT *pUnit,
	BOOL bCrafting );

BOOL s_PlayerTryReturnToHeadstone( 
	UNIT *pUnit);

BOOL PlayerCanRespawnAsGhost(
	UNIT *pPlayer);

BOOL PlayerCanResurrect(
	UNIT *pUnit);

#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_PlayerOnDie(
	UNIT *pPlayer,
	UNIT *pKiller);
#endif

void s_PlayerClearExperiencePenalty(
	UNIT *pPlayer);

void s_PlayerRestoreVitals(
	UNIT *pPlayer,
	BOOL bPlayEffects = TRUE);

void s_PlayerRestart(
	UNIT *pUnit,
	PLAYER_RESPAWN eRespawnChoice);

int PlayerGetRestartInTownLevelDef(
	UNIT *pPlayer);
	
UNIT *PlayerFindNearest(
	UNIT *pUnit);

typedef void (* PFN_PLAYER_ITERATE)( UNIT *pUnit, void *pUserData );

void PlayersIterate( 
	GAME *pGame, 
	PFN_PLAYER_ITERATE pfnCallback,
	void *pCallbackData);

int x_PlayerInteractGetChoices(
	UNIT *pUnitInteracting,
	UNIT *pPlayerTarget,
	INTERACT_MENU_CHOICE *ptChoices,
	int nBufferSize,
	UI_COMPONENT *pComponent = NULL,
	PGUID guid = INVALID_GUID);

BOOL s_PlayerPostsOpenLevels(
	UNIT *pPlayer);

BOOL s_PlayerIsAutoPartyEnabled(
	UNIT *pPlayer);

void s_PlayerEnableAutoParty(
	UNIT *pPlayer,
	BOOL bEnabled);

void c_PlayerToggleAutoParty(
	UNIT *pPlayer);	

BOOL PlayerPvPIsEnabled(
	UNIT *pPlayer);

void c_PlayerPvPToggle(
	UNIT *pPlayer);	

void c_PlayerSendRespawn(
	PLAYER_RESPAWN eChoice);

void s_PlayerMoved( 
	UNIT *pPlayer);

void c_PlayerClearMusicInfo(
	UNIT * pPlayer);

void c_PlayerUpdateMusicInfo(
	UNIT * pPlayer,
	const MSG_SCMD_MUSIC_INFO * pMsg);

int c_PlayerGetMusicInfoMonsterPercentRemaining(
	UNIT * pPlayer);

void s_PlayerVersion(
	UNIT *pPlayer);

BOOL PlayerIsHardcore(
	UNIT *pPlayer);

BOOL PlayerIsPVPOnly(
	UNIT *pPlayer);

BOOL PlayerIsHardcoreDead(
	UNIT *pPlayer);

BOOL PlayerIsInPVPWorld(
	UNIT *pPlayer);

BOOL PlayerIsLeague(
	UNIT *pPlayer);

BOOL PlayerIsElite(
	UNIT *pPlayer);

BOOL PlayerIsSubscriber(
	UNIT *pPlayer,
	BOOL bForceBadgeCheck = FALSE);

void c_PlayerSetupWhisperTo(
	UNIT *pPlayerOther);

void c_PlayerSetupWhisperTo(
	LPCWSTR uszUnitName);

void s_GiveBadgeRewards(
	UNIT *player);

void s_PlayerJoinLog(
	GAME *pGame,
	UNIT *pPlayer,
	APPCLIENTID idAppClient,
	const char *pszFormat,
	...);

void c_PlayerInvite(
	WCHAR *puszPlayerName);

void c_PlayerInviteAccept( 
	PGUID guidInviter,
	BOOL bAbandonGame);

void c_PlayerPartyJoinConfirm( 
	BOOL bAbandonGame );

void s_PlayerJoinedParty(
	UNIT *pPlayer,
	PARTYID idParty);

void s_PlayerLeftParty(
	UNIT *pPlayer,
	PARTYID idParty,
	PARTY_LEAVE_REASON eLeaveReason);

void s_PlayerPartyValidation(
	UNIT *pPlayer,
	DWORD dwDelayInTicks);

BOOL s_PlayerMustAbandonGameForParty(
	UNIT *pPlayer);

int s_PlayerLimitInventoryType(
	UNIT *pPlayer,
	int nType,
	int nLimit = -1);

void PlayerCleanUpGuildHeralds(
	GAME * game,
	UNIT * player,
	BOOL bInGuild );

void s_PlayerQueueRoomUpdate(
	UNIT *pPlayer,
	DWORD dwDelayInTicks,
	BOOL bCalledFromEvent = FALSE);

BOOL s_PlayerDoMinigameReset(
	GAME* game,
	UNIT* unit,
	const struct EVENT_DATA& event_data);
	
BOOL s_PlayerMinigameCheckTimeout(
	GAME* game,
	UNIT* unit,
	const struct EVENT_DATA& event_data);

BOOL PlayerAttribsRespec(
	UNIT *pPlayer);

void PlayerInventorySetGameVariants(
	UNIT *pPlayer );

int c_PlayerListAvailableTitles(
	UNIT *pPlayer,
	int *pnTitleArray,
	int nTitleArraySize);

BOOL s_PlayerTitleIsAvailable(
	UNIT *pPlayer,
	int nTitleID);

void s_PlayerSetTitle(
	UNIT *pPlayer,
	int nStringID);

int x_PlayerGetTitleStringID(
	UNIT *pPlayer);

#if !ISVERSION(SERVER_VERSION)
void c_PlayerSetTitle(
	UNIT *pPlayer,
	int nStringID);
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerCSRChatEnter(
	void);

void PlayerCSRChatExit(
	void);

BOOL PlayerInCSRChat(
	void);

BOOL PlayerHasRecipe( UNIT *pPlayer,
					  int nRecipeID );

void PlayerRemoveRecipe( UNIT *pPlayer,
						 int nRecipeID );

void PlayerLearnRecipe( UNIT *pPlayer,
					   int nRecipeID );

BOOL PlayerCanBeInspected(
						 UNIT *pPlayer,
						 UNIT *pOtherPlayer);



BOOL PlayerCanWarpTo(
	UNIT * pPlayer,
	int nLevelDefDest);

BOOL c_PlayerCanWarpTo(
	PGUID guid);

int c_GetPlayerUnitDataRowByUnitTypeAndGender(
	GENDER eGender,
	int nFactionUnitType );

BOOL s_MakeNewPlayer(
	GAME * game,
	struct GAMECLIENT * client,
	int playerClass,
	const WCHAR * strName);

UNIQUE_ACCOUNT_ID PlayerGetAccountID(
	UNIT *pPlayer);	


MYTHOS_POINTSOFINTEREST::cPointsOfInterest * PlayerGetPointsOfInterest( UNIT *pPlayer );

//----------------------------------------------------------------------------
// Externals
//----------------------------------------------------------------------------
extern const BOOL gbEnablePlayerJoinLog;
		
#endif // _PLAYER_H_

