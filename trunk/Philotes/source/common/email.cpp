//----------------------------------------------------------------------------
// email.cpp
// (C) Copyright 2003, Flagship Studios, all rights reserved
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------

#include "email.h"

//----------------------------------------------------------------------------
// DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct CSR_EMAIL_RECIPIENT_TYPE_LOOKUP
{
	CSR_EMAIL_RECIPIENT_TYPE eRecipientType;
	const WCHAR *puszName;
};
static const CSR_EMAIL_RECIPIENT_TYPE_LOOKUP sgtCSREmailTypeLookup[] = 
{
	{ CERT_TO_PLAYER,		L"CERT_TO_PLAYER" },
	{ CERT_TO_ACCOUNT,		L"CERT_TO_ACCOUNT" },
	{ CERT_TO_GUILD,		L"CERT_TO_GUILD" },
	{ CERT_TO_SHARD,		L"CERT_TO_SHARD" }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CSR_EMAIL_RECIPIENT_TYPE EmailParseCSREmailRecipientType(
	const WCHAR *puszType)
{
	ASSERTX_RETVAL( puszType, CERT_INVALID, "Expected type string for CSR email type" );

	for (int i = 0; i < arrsize( sgtCSREmailTypeLookup ); ++i)
	{
		const CSR_EMAIL_RECIPIENT_TYPE_LOOKUP *pLookup = &sgtCSREmailTypeLookup[ i ];
		if (PStrICmp( pLookup->puszName, puszType ) == 0)
		{
			return pLookup->eRecipientType;
		}
	}
	
	return CERT_INVALID;		
	
}
