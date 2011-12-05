//----------------------------------------------------------------------------
// statsfunc.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"

/*
//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum STATSFUNC_OP
{
	SFOP_OPERAND_STAT,
	SFOP_OPERAND_CONST,
	SFOP_ADD,
	SFOP_MUL,
	SFOP_PCT,
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------
typedef int (*STATSFUNC_OPFUNC)(struct STATSFUNC_CALC & calc, struct STATSFUNC_NODE * expr);


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct STATSFUNC_NODE
{
	STATSFUNC_OP		m_eOperator;			// operator to be performed on children
	STATSFUNC_OPFUNC	m_fpOp;
	STATSFUNC_NODE		m_nChildCount;			// number of children
	STATSFUNC_NODE *	m_pOperands;			// pointers to children
	int					m_Target;				// target stat
	int					m_nConstantVal;			// operand constant value
	int					m_nStat;				// operand stat
};


// pass this around to reduce argument passing during recursion
struct STATSFUNC_CALC
{
	GAME *				game;
	UNIT *				unit;
	WORD				wStat;
	PARAM				dwParam;
	int					oldvalue;
	int					newvalue;
	BOOL				bSend;
};


//----------------------------------------------------------------------------
// STATIC FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// get rid of this intermediate step soon
//----------------------------------------------------------------------------
int sStatsFuncChangeStat(
	STATSFUNC_CALC & calc,
	STATSFUNC_NODE * expr)
{
	ASSERT_RETZERO(expr->m_fpOp);
	return expr->m_fpOp(calc, expr);
}


//----------------------------------------------------------------------------
// operand: constant
//----------------------------------------------------------------------------
static inline int sStatsFuncChangeStat_Constant(
	STATSFUNC_CALC & calc,
	STATSFUNC_NODE * expr)
{
	return expr->m_nConstantVal;
}


//----------------------------------------------------------------------------
// operand: constant
//----------------------------------------------------------------------------
static inline int sStatsFuncChangeStat_Stat(
	STATSFUNC_CALC & calc,
	STATSFUNC_NODE * expr)
{
	return 
}


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsFuncParse(
	char * expression)
{

}


//----------------------------------------------------------------------------
// call this when a stat changes
//----------------------------------------------------------------------------
void StatsFuncChangeStat(
	GAME * game,
	UNIT * unit,
	WORD wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data,
	int newvalue,
	int oldvalue,
	BOOL bSend)
{
	int delta = 0;

	for (unsigned int ii = 0; ii < stats_data->nAffectedStatsList; ++ii)
	{
		STATSFUNC_NODE * node = stats_data->pnAffectedStatsList[ii];
		ASSERT_CONTINUE(node && node->m_eOperator == SFOP_OPERAND_STAT);
		STATSFUNC_NODE * op = node->m_pParent;
		ASSERT_CONTINUE(op);

		switch (op->m_eOperator)
		{
		case SFOP_OPERAND:
		case SFOP_OPERAND_CONST:
			ASSERT_BREAK(0);
		case SFOP_ADD:		// add it
			{
				delta -= oldvalue;
				delta += newvalue;
			}
			break;
		case SFOP_MUL:
			break;
		case SFOP_PCT:
			break;
		}
	}
}
*/