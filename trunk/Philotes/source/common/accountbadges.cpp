//----------------------------------------------------------------------------
// accountbadges.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------

#include "accountbadges.h"

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
	BOOL gbAllBadges = FALSE;
#endif


//----------------------------------------------------------------------------
// BADGE COLLECTION METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BADGE_COLLECTION::BADGE_COLLECTION(
	const BADGE_COLLECTION & other )
{
	Init(other);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BADGE_COLLECTION::Init(
	const BADGE_COLLECTION & other )
{
	m_badges[0] = other.m_badges[0];
	m_badges[1] = other.m_badges[1];
	m_badges[2] = other.m_badges[2];
	m_badges[3] = other.m_badges[3];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BADGE_COLLECTION::BADGE_COLLECTION(
	int * badges,
	DWORD badgeCount )
{
	ASSERT_RETURN(badges != NULL);
	structclear(m_badges);

	for(DWORD ii = 0; ii < badgeCount; ++ii)
	{
		ASSERT_CONTINUE(badges[ii] < (4 * BADGES_PER_TYPE_GROUP));
		m_badges[badges[ii] / 64] |= ((UINT64)1 << ((UINT64)badges[ii] % (UINT64)64));
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BADGE_COLLECTION::BADGE_COLLECTION(
	const char * permissionName,
	... )
{
	UNREFERENCED_PARAMETER(permissionName);
	structclear(m_badges);

	va_list permissions;
	va_start(permissions, permissionName);

	int badge = BADGE_TYPE_INVALID;
	while( (badge = va_arg(permissions, int)) != BADGE_TYPE_INVALID )
	{
		ASSERT_CONTINUE(badge < (4 * BADGES_PER_TYPE_GROUP));
		m_badges[badge / 64] |= ((UINT64)1 << ((UINT64)badge % (UINT64)64));
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BADGE_COLLECTION::BADGE_COLLECTION()
{
	ZeroMemory( m_badges, sizeof( m_badges ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL BADGE_COLLECTION::AddBadge(
	int badge )
{
	ASSERT_RETFALSE(badge < (4 * BADGES_PER_TYPE_GROUP));
	UINT64 mask = ((UINT64)1 << ((UINT64)badge % (UINT64)64));
	m_badges[badge / 64] |= mask;
	return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL BADGE_COLLECTION::RemoveBadge(
	int badge )
{
	ASSERT_RETFALSE(badge < (4 * BADGES_PER_TYPE_GROUP));
	UINT64 mask = ((UINT64)1 << ((UINT64)badge % (UINT64)64));
	m_badges[badge / 64] &= ~mask;
	return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL BADGE_COLLECTION::HasBadge(
	int badge ) const
{
	ASSERT_RETFALSE(badge < (4 * BADGES_PER_TYPE_GROUP));
#if ISVERSION(DEVELOPMENT)
	if (gbAllBadges)
	{
		return TRUE;
	}
#endif

	UINT64 mask = ((UINT64)1 << ((UINT64)badge % (UINT64)64));
	return ((m_badges[badge / 64] & mask) == mask);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL BADGE_COLLECTION::HasPermission(
	const BADGE_COLLECTION & permission ) const
{
#if ISVERSION(DEVELOPMENT)
	if (gbAllBadges)
	{
		return TRUE;
	}
#endif
	return (((m_badges[0] & permission.m_badges[0]) == permission.m_badges[0]) &&
			((m_badges[1] & permission.m_badges[1]) == permission.m_badges[1]) &&
			((m_badges[2] & permission.m_badges[2]) == permission.m_badges[2]) &&
			((m_badges[3] & permission.m_badges[3]) == permission.m_badges[3]) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL BADGE_COLLECTION::operator==(const BADGE_COLLECTION &other) const
{
	for(int i = 0; i < NUM_ACCOUNT_BADGE_TYPES; i++)
		if(this->m_badges[i] != other.m_badges[i]) return FALSE;
	return TRUE;
}

//----------------------------------------------------------------------------
// STATIC BADGE COLLECTION GENERATION
//----------------------------------------------------------------------------
#undef  _ACCOUNT_BADGE_PERMISSIONS_
#undef   BADGE_PERMISSION
#define  BADGE_PERMISSION(name,...)	const BADGE_COLLECTION name(#name,__VA_ARGS__,BADGE_TYPE_INVALID);
#include "accountbadges.h"

//----------------------------------------------------------------------------
// TRANSLATION FUNCTIONS
// Note: we can friend these if m_badges becomes private once more.
//----------------------------------------------------------------------------
BOOL BadgeCollectionWriteToBuf(BIT_BUF &buf, const BADGE_COLLECTION &tBadges)
{
	for(int i = 0; i < NUM_ACCOUNT_BADGE_TYPES; i++)
	{
		if(!buf.PushBuf((BYTE*)&tBadges.m_badges[i], 64  BBD_FILE_LINE)) return FALSE;
	}
	return TRUE;
}


BOOL BadgeCollectionReadFromBuf(BIT_BUF &buf, BADGE_COLLECTION &tBadgesOutput)
{
	for(int i = 0; i < NUM_ACCOUNT_BADGE_TYPES; i++)
	{
		if(!buf.PopBuf((BYTE*)&tBadgesOutput.m_badges[i], 64  BBD_FILE_LINE)) return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
// Since we have so many badges associated with subscriber-dom...
//----------------------------------------------------------------------------
BOOL BadgeCollectionHasSubscriberBadge(
	const BADGE_COLLECTION &badges)
{
	return badges.HasBadge(ACCT_TITLE_SUBSCRIBER) ||
		badges.HasBadge(ACCT_MODIFIER_TRIAL_SUBSCRIPTION) ||
		badges.HasBadge(ACCT_MODIFIER_STANDARD_SUBSCRIPTION) ||
		badges.HasBadge(ACCT_MODIFIER_LIFETIME_SUBSCRIPTION) ||
		(badges.HasBadge(ACCT_TITLE_TEST_CENTER_SUBSCRIBER) && AppTestFlag(AF_TEST_CENTER_SUBSCRIBER));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL BadgeCollectionIsFlagshipEmployee(
	const BADGE_COLLECTION &badges)
{
	return badges.HasBadge(ACCT_TITLE_ADMINISTRATOR) ||
		   badges.HasBadge(ACCT_TITLE_DEVELOPER) || 
		   badges.HasBadge(ACCT_TITLE_FSSPING0_EMPLOYEE) ||
		   badges.HasBadge(ACCT_TITLE_CUSTOMER_SERVICE_REPRESENTATIVE);
}