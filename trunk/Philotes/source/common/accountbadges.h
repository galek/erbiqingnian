//----------------------------------------------------------------------------
// accountbadges.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _ACCOUNTBADGES_H_
#define _ACCOUNTBADGES_H_


//----------------------------------------------------------------------------
// BADGE TYPES DEFINES
//----------------------------------------------------------------------------

// invalid badge type
#define BADGE_TYPE_INVALID		((int)-1)

// badge type groups
enum BADGE_TYPES
{
	ACCOUNT_TITLES,
	ACCOUNT_MODIFIERS,
	ACCOUNT_STATUS_MARKERS,
	ACCOUNT_ACCOMPLISHMENT_MARKERS,

	NUM_ACCOUNT_BADGE_TYPES
};

// badge limits
#define BADGES_PER_TYPE_GROUP	64
#define MAX_BADGES				NUM_ACCOUNT_BADGE_TYPES*BADGES_PER_TYPE_GROUP


//----------------------------------------------------------------------------
// BADGES
// !!NOTE!! If any of the following enumerations are altered in any way,
// you must make sure the database schema is modified to reflect the update.
//----------------------------------------------------------------------------

// account title badges.
//	DO NOT ALTER ENUM W/O MATCHING DATABASE UPDATE.
enum ACCT_TITLE_BADGE
{
	ACCT_TITLE_ADMINISTRATOR				= ACCOUNT_TITLES * BADGES_PER_TYPE_GROUP,
	ACCT_TITLE_DEVELOPER,
	ACCT_TITLE_FSSPING0_EMPLOYEE,
	ACCT_TITLE_CUSTOMER_SERVICE_REPRESENTATIVE,
	ACCT_TITLE_SUBSCRIBER,
	ACCT_TITLE_BOT,
	ACCT_TITLE_TESTER,
	ACCT_TITLE_CSR_TIER_2,
	ACCT_TITLE_CSR_TIER_3,
	ACCT_TITLE_EA_EMPLOYEE,
	ACCT_TITLE_CSR_EA,
	ACCT_TITLE_VIP,
	ACCT_TITLE_PROMO_SUBSCRIPTION_1,
	ACCT_TITLE_TEST_CENTER_SUBSCRIBER,
	ACCT_TITLE_MONITOR,
	ACCT_TITLE_TRIAL_USER,
	ACCT_TITLE_RMT_ADMINISTRATOR
};

// account modifier badges.
//	DO NOT ALTER ENUM W/O MATCHING DATABASE UPDATE.
enum ACCT_MODIFIER_BADGE
{
	ACCT_MODIFIER_TRIAL_SUBSCRIPTION		= ACCOUNT_MODIFIERS * BADGES_PER_TYPE_GROUP,
	ACCT_MODIFIER_STANDARD_SUBSCRIPTION,
	ACCT_MODIFIER_LIFETIME_SUBSCRIPTION,
	ACCT_MODIFIER_FOUNDERS_OFFER,
	ACCT_MODIFIER_PREORDER_BESTBUY,
	ACCT_MODIFIER_PREORDER_WALMART,
	ACCT_MODIFIER_PREORDER_EBGS,
	ACCT_MODIFIER_PREORDER_GEN,
	ACCT_MODIFIER_UNDERAGE,
	ACCT_MODIFIER_COLLECTORS_EDITION,
	ACCT_MODIFIER_ASIA_DK_01,
	ACCT_MODIFIER_ASIA_DK_02,
	ACCT_MODIFIER_ASIA_DK_03,
	ACCT_MODIFIER_ASIA_DK_04,
	ACCT_MODIFIER_ASIA_DK_05,
	ACCT_MODIFIER_ASIA_DK_06,
	ACCT_MODIFIER_ASIA_DK_07,
	ACCT_MODIFIER_ASIA_DK_08,
	ACCT_MODIFIER_ASIA_DK_09,
	ACCT_MODIFIER_ASIA_DK_10,
	ACCT_MODIFIER_ASIA_DK_11,
	ACCT_MODIFIER_ASIA_DK_12,
	ACCT_MODIFIER_PC_GAMER,
	ACCT_MODIFIER_BETA_FULL_GAME,			// beta account has access to full game 
	ACCT_MODIFIER_TEST_CENTER_USER,			// account has access to the test center
	ACCT_MODIFIER_BETA_GRACE_PERIOD_USER,	// beta user without product key has access during grace period
	ACCT_MODIFIER_IAH_CUSTOMER_HALLOWEEN,	// give user access to zombot and flaming skull
	ACCT_MODIFIER_HBS_COCO_PET,		// HBS - Special CokoMoko Pet
	ACCT_MODIFIER_HBS_PC_BANG_5EXP,		// HBS - PC BANG Badge Adds 5% EXP Bonus
	ACCT_MODIFIER_HBS_CUSTOMER_HALLOWEEN,	// HBS - Give customers access to zombot and flaming skull
	ACCT_MODIFIER_HBS_PC_BANG_10EXP,		// HBS - PC BANG Badge Adds 10% EXP Bonus
};

// account status marking badges.
//	DO NOT ALTER ENUM W/O MATCHING DATABASE UPDATE.
enum ACCT_STATUS_BADGE
{
	ACCT_STATUS_SUSPENDED					= ACCOUNT_STATUS_MARKERS * BADGES_PER_TYPE_GROUP,
	ACCT_STATUS_BANNED_FROM_GUILDS,
	ACCT_STATUS_UNDERAGE
};

// account accomplishment badges.
//	DO NOT ALTER ENUM W/O MATCHING DATABASE UPDATE.
enum ACCT_ACCOMPLISHMENT_BADGE
{
	ACCT_ACCOMPLISHMENT_ALPHA_TESTER		= ACCOUNT_ACCOMPLISHMENT_MARKERS * BADGES_PER_TYPE_GROUP,
	ACCT_ACCOMPLISHMENT_BETA_TESTER,
	ACCT_ACCOMPLISHMENT_HARDCORE_MODE_BEATEN,
	ACCT_ACCOMPLISHMENT_REGULAR_MODE_BEATEN,
	ACCT_ACCOMPLISHMENT_CAN_PLAY_ELITE_MODE,
	ACCT_ACCOMPLISHMENT_CAN_PLAY_HARDCORE_MODE
};

inline BOOL BadgeIsAccomplishment(int nBadge)
{
	return (nBadge / BADGES_PER_TYPE_GROUP) == ACCOUNT_ACCOMPLISHMENT_MARKERS;
}

//----------------------------------------------------------------------------
// BADGE COLLECTION TYPE
//----------------------------------------------------------------------------
struct BADGE_COLLECTION
{
public:  // making this public makes it much easier for the definition system to create a global set of override Badges
	UINT64 m_badges[NUM_ACCOUNT_BADGE_TYPES];

public:
	BADGE_COLLECTION(
		const BADGE_COLLECTION & other );
	BADGE_COLLECTION(
		int * badges,
		DWORD badgeCount );
	BADGE_COLLECTION(
		const char * permissionName,
		... );
	BADGE_COLLECTION();
	void Init(
		const BADGE_COLLECTION & other );
	BOOL AddBadge(
		int badge );
	BOOL RemoveBadge(
		int badge );
	BOOL HasBadge(
		int badge ) const;
	BOOL HasPermission(
		const BADGE_COLLECTION & permission ) const;
	BOOL operator==(const BADGE_COLLECTION &other) const;
};

struct PROTECTED_BADGE_COLLECTION : protected BADGE_COLLECTION
{
public:
	inline BOOL GetBadgeCollection(BADGE_COLLECTION &output)
	{
		return MemoryCopy(output.m_badges, sizeof(output.m_badges),
			m_badges, sizeof(m_badges));
	}
	inline void AddAccomplishmentBadge(int idBadge)
	{	//Note: ONLY for accomplishments.  we do not allow non-accomplishments added directly.
		if(BadgeIsAccomplishment(idBadge))
			AddBadge(idBadge);
	}
	inline PROTECTED_BADGE_COLLECTION(const BADGE_COLLECTION &other)
		:BADGE_COLLECTION(other)
	{
	}
};

//----------------------------------------------------------------------------
// BADGE Reward stuff
//----------------------------------------------------------------------------
struct BADGE_REWARDS_DATA
{
	char	pszName[DEFAULT_INDEX_SIZE];
	WORD	m_wCode;
	int		m_nBadge;
	int		m_nItem;
	int		m_nDontApplyIfPlayerHasRewardItemFor;
	int		m_nState;
};

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
BOOL BadgeCollectionWriteToBuf(BIT_BUF &buf, const BADGE_COLLECTION &tBadges);
BOOL BadgeCollectionReadFromBuf(BIT_BUF &buf, BADGE_COLLECTION &tBadgesOutput);

//----------------------------------------------------------------------------
// Since we have so many badges associated with subscriber-dom...
//----------------------------------------------------------------------------
BOOL BadgeCollectionHasSubscriberBadge(
	const BADGE_COLLECTION &badges);

BOOL BadgeCollectionIsFlagshipEmployee(
	const BADGE_COLLECTION &badges);
	
#endif	//_ACCOUNTBADGES_H_

#ifndef _ACCOUNT_BADGE_PERMISSIONS_
#define _ACCOUNT_BADGE_PERMISSIONS_


//----------------------------------------------------------------------------
// BADGE PERMISSION COLLECTIONS
//----------------------------------------------------------------------------

#ifndef BADGE_PERMISSION
#define BADGE_PERMISSION(name,...) extern const BADGE_COLLECTION name;
#endif

BADGE_PERMISSION( BADGE_PERMISSION_CAN_SEND_CHAT_ANNOUNCEMENTS,	ACCT_TITLE_ADMINISTRATOR )
BADGE_PERMISSION( BADGE_PERMISSION_MEMBER_OF_ADMIN_CHAT_GUILD,	ACCT_TITLE_ADMINISTRATOR )
BADGE_PERMISSION( BADGE_PERMISSION_BETA_FULL_GAME,				ACCT_MODIFIER_BETA_FULL_GAME )
BADGE_PERMISSION( BADGE_PERMISSION_DEVELOPER_COMMANDS,			ACCT_TITLE_SUBSCRIBER, ACCT_ACCOMPLISHMENT_REGULAR_MODE_BEATEN )

//----------------------------------------------------------------------------
// Externals
//----------------------------------------------------------------------------

#if ISVERSION(DEVELOPMENT)
	extern BOOL gbAllBadges;
#endif


#endif
