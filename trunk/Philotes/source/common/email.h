//----------------------------------------------------------------------------
//	email.h
//  (C) Copyright 2003, Flagship Studios, all rights reserved
//----------------------------------------------------------------------------

#ifndef __EMAIL_H_
#define __EMAIL_H_

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// NOTE: This enum must match the CSRPortal C# enumeration CSR_EMAIL_RECIPIENT_TYPE
//----------------------------------------------------------------------------
enum CSR_EMAIL_RECIPIENT_TYPE
{
	CERT_INVALID = -1,
	
	CERT_TO_PLAYER		= 0,	// do not change, referenced in CSR portal and database stored procedures
	CERT_TO_ACCOUNT		= 1,	// do not change, referenced in CSR portal and database stored procedures
	CERT_TO_GUILD		= 2,	// do not change, referenced in CSR portal and database stored procedures
	CERT_TO_SHARD		= 3,	// do not change, referenced in CSR portal and database stored procedures
	
	CERT_NUM_TYPES
	
};

//----------------------------------------------------------------------------
enum EMAIL_SPEC_SOURCE
{
	ESS_INVALID = -1,

	ESS_PLAYER,				// player mail
	ESS_CSR,				// CSR mail
	ESS_PLAYER_ITEMSALE,	// a fake email from the game server for itemsale purposes
	ESS_PLAYER_ITEMBUY,		// a fake email from the game server for buying an item
	ESS_AUCTION,			// An auction email
	ESS_CONSIGNMENT,		// A consignment house email
	
	ESS_NUM_TYPES		// keep this last p0lease
};

//----------------------------------------------------------------------------
enum EMAIL_SPEC_SUB_TYPE
{
	ESST_INVALID = -1,

	ESST_CSR_ITEM_RESTORE,	// an item restore from a CSR rep
		
	ESST_NUM_TYPES			// keep this last please
};

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

CSR_EMAIL_RECIPIENT_TYPE EmailParseCSREmailRecipientType(
	const WCHAR *puszType);

#endif
