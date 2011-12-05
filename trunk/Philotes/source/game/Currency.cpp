//----------------------------------------------------------------------------
// FILE:
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "Currency.h"
#include "excel.h"
#include "items.h"
#include "unit_priv.h"
#include "Economy.h"


cCurrency::cCurrency( void )
{	
	FillInCurrencyTypeValues( NULL );
}

cCurrency::cCurrency( UNIT *pUnit )
{
	FillInCurrencyTypeValues( pUnit );
}
cCurrency::cCurrency( int valueInGame, 
					  int valueInWorld, /* = 0 */
					  int valuePVPPoints ) /* = 0 */
{	

	m_CurrencyTypeValues[KCURRENCY_TYPE_INGAME] = valueInGame;
	m_CurrencyTypeValues[KCURRENCY_TYPE_REALWORLD] = valueInWorld;
	m_CurrencyTypeValues[KCURRENCY_TYPE_PVPPOINTS] = valuePVPPoints;
}


cCurrency::~cCurrency()
{

}

inline void cCurrency::FillInCurrencyTypeValues( UNIT *pUnit )
{
	
	if( pUnit != NULL )
	{
		switch( UnitGetGenus( pUnit ) )
		{
		case GENUS_PLAYER:
		case GENUS_MONSTER:		
			ASSERTX_RETURN( pUnit, "No unit passed in" );
			for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
				m_CurrencyTypeValues[t] = UnitGetStat( pUnit, sCurrencyStats[ t ] );
			break;
		case GENUS_ITEM:
			ASSERTX_RETURN( pUnit, "No unit passed in" );
			if (UnitIsA(pUnit, UNITTYPE_MONEY))
			{
				for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
					m_CurrencyTypeValues[t] = UnitGetStat( pUnit, sCurrencyStats[ t ] );
			}
			else
			{
				(*this) = EconomyItemBuyPrice( pUnit, NULL ); //we set our selves to the buy price in this case
			}
			break;
		default:
			for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
				m_CurrencyTypeValues[t] = 0;
			break;		
		};	
	}
	else
	{
		for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
			m_CurrencyTypeValues[t] = 0;
	}
}

int cCurrency::GetValue( ECURRENCY_VALUES value )
{	
	int returnValue( 0 );		
	switch( value )
	{
	case KCURRENCY_VALUE_INGAME:		//returns STATS_GOLD
		returnValue = m_CurrencyTypeValues[KCURRENCY_TYPE_INGAME];
		break;
	case KCURRENCY_VALUE_INGAME_RANK1:	//returns copper
		returnValue = m_CurrencyTypeValues[KCURRENCY_TYPE_INGAME] % 100;
		break;
	case KCURRENCY_VALUE_INGAME_RANK2:	//returns silver
		returnValue = (int)( (float)m_CurrencyTypeValues[KCURRENCY_TYPE_INGAME] * .01f ) % 100;
		break;
	case KCURRENCY_VALUE_INGAME_RANK3:	//returns gold
		returnValue = (int)( (float)m_CurrencyTypeValues[KCURRENCY_TYPE_INGAME] * .0001f );		
		break;
	case KCURRENCY_VALUE_REALWORLD:		//returns STATS_MONEY_REALWORLD
		returnValue = m_CurrencyTypeValues[KCURRENCY_TYPE_REALWORLD];
		break;
	case KCURRENCY_VALUE_REALWORLD_RANK1://return cents
		returnValue = m_CurrencyTypeValues[KCURRENCY_TYPE_REALWORLD] % 100;
		break;
	case KCURRENCY_VALUE_REALWORLD_RANK2://return dollars
		returnValue = (int)( (float)m_CurrencyTypeValues[KCURRENCY_TYPE_REALWORLD] * .01f);
		break;
	case KCURRENCY_VALUE_PVPPOINTS: //returns PVP Points
		returnValue = m_CurrencyTypeValues[KCURRENCY_TYPE_PVPPOINTS];
		break;
	};
	return returnValue;
}


void cCurrency::AddCurrency( cCurrency &currency )
{
	for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
		m_CurrencyTypeValues[t] += currency.m_CurrencyTypeValues[t];
	

}

void cCurrency::RemoveCurrency( cCurrency &currency )
{
	for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
		m_CurrencyTypeValues[t] -= currency.m_CurrencyTypeValues[t];

}

void cCurrency::operator/= ( int value )
{
	for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
		m_CurrencyTypeValues[t] /= value;
}

void cCurrency::operator*= ( int value )
{
	for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
		m_CurrencyTypeValues[t] *= value;
}

void cCurrency::operator/= ( float value )
{
	for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
		m_CurrencyTypeValues[t] = (int)((float)m_CurrencyTypeValues[t]/value);
}

void cCurrency::operator*= ( float value )
{
	for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
		m_CurrencyTypeValues[t] = (int)((float)m_CurrencyTypeValues[t] * value);

}


void cCurrency::operator+= ( cCurrency currency )
{
	AddCurrency( currency );
}

void cCurrency::operator-= ( cCurrency currency )
{
	RemoveCurrency( currency );
}

BOOL cCurrency::operator== ( cCurrency currency )
{
	for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
	{
		if( m_CurrencyTypeValues[t] != currency.m_CurrencyTypeValues[t] )
			return FALSE;
	}
	return TRUE;
}

BOOL cCurrency::operator!= ( cCurrency currency )
{
	BOOL bNotEqual( FALSE );
	for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
	{
		if( m_CurrencyTypeValues[t] != currency.GetValue( sCurrencyWholeValues[ t ] ) )
		{
			bNotEqual = TRUE; 
		}
	}
	return bNotEqual;

}

BOOL cCurrency::operator>= ( cCurrency currency )
{
	for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
	{
		if( m_CurrencyTypeValues[t] < currency.m_CurrencyTypeValues[t] )
			return FALSE;
	}
	return TRUE;
}

BOOL cCurrency::operator<= ( cCurrency currency )
{
	for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
	{
		if( m_CurrencyTypeValues[t] > currency.m_CurrencyTypeValues[t] )
			return FALSE;
	}
	return TRUE;

}

BOOL cCurrency::operator> ( cCurrency currency )
{
	for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
	{
		if( m_CurrencyTypeValues[t] <= currency.m_CurrencyTypeValues[t] )
			return FALSE;
	}
	return TRUE;
}

BOOL cCurrency::operator< ( cCurrency currency )
{
	for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
	{
		if( m_CurrencyTypeValues[t] < currency.m_CurrencyTypeValues[t] )
			return TRUE;
	}
	return FALSE;

}

cCurrency cCurrency::operator- ( cCurrency currency )
{
	cCurrency nReturnCurrency;
	for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
		nReturnCurrency.m_CurrencyTypeValues[t] = m_CurrencyTypeValues[t] - currency.GetValue( sCurrencyWholeValues[ t ] );
	return nReturnCurrency;
}

cCurrency cCurrency::operator+ ( cCurrency currency )
{
	cCurrency nReturnCurrency;
	for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
		nReturnCurrency.m_CurrencyTypeValues[t] = m_CurrencyTypeValues[t] + currency.GetValue( sCurrencyWholeValues[ t ] );
	return nReturnCurrency;
}


BOOL cCurrency::CanCarry( GAME *pGame, cCurrency currency )
{
	for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
	{
		int nValue = m_CurrencyTypeValues[t] + currency.GetValue( sCurrencyWholeValues[ t ] );
		const STATS_DATA * stat = StatsGetData( pGame, sCurrencyStats[ t ] );
		if( nValue > stat->m_nMaxSet )
			return FALSE;
	}
	return TRUE;
}


int cCurrency::HighestValue()
{
	int nMax( m_CurrencyTypeValues[0] );
	for( int t = 1; t < KCURRENCY_TYPE_COUNT; t++ )
		nMax = MAX( nMax, m_CurrencyTypeValues[t] );
	return nMax;	
}

int cCurrency::LowestValue()
{
	int nMin( m_CurrencyTypeValues[0] );
	for( int t = 1; t < KCURRENCY_TYPE_COUNT; t++ )
		nMin = MIN( nMin, m_CurrencyTypeValues[t] );
	return nMin;	
}

BOOL cCurrency::IsZero()
{ 
	for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
	{
		if( m_CurrencyTypeValues[ t ]  > 0 )
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL cCurrency::IsNegative()
{ 
	BOOL isNeg( FALSE );
	for( int t = 0; t < KCURRENCY_TYPE_COUNT; t++ )
	{
		if( m_CurrencyTypeValues[ t ]  < 0 )
		{
			isNeg = TRUE;
		}
	}
	return isNeg;	
}
