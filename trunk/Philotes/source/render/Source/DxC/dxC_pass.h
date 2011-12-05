//----------------------------------------------------------------------------
// dxC_pass.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_PASS__
#define __DXC_PASS__

#include "dxC_meshlist.h"

namespace FSSE
{

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define PASS_FLAG_ALPHABLEND				MAKE_MASK(0)
#define PASS_FLAG_FOG_ENABLED				MAKE_MASK(1)
#define PASS_FLAG_PARTICLES					MAKE_MASK(2)
#define PASS_FLAG_SKYBOX					MAKE_MASK(3)
#define PASS_FLAG_BLOB_SHADOWS				MAKE_MASK(4)
#define PASS_FLAG_SET_VIEW_PARAMS			MAKE_MASK(5)
#define PASS_FLAG_DEPTH						MAKE_MASK(6)
#define PASS_FLAG_BEHIND					MAKE_MASK(7)


#define MAX_PASS_NAME_LEN					32

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum SORT_CRITERIA
{
	SORT_INVALID = -1,
	SORT_NONE = 0,
	SORT_FRONT_TO_BACK,
	SORT_BACK_TO_FRONT,
	SORT_STATE,
	//
	NUM_SORT_CRITERIA
};

//----------------------------------------------------------------------------
// Include pass definition enum
#define INCLUDE_RPD_ENUM
enum RENDER_PASS_TYPE
{
	RPTYPE_INVALID = -1,
#	include "dxC_pass_def.h"
	NUM_RENDER_PASS_TYPES
};
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct RenderPassDefinition
{
	DWORD				dwPassFlags;

	DWORD				dwModelFlags_MustHaveAll [ DWORD_FLAG_SIZE(NUM_MODEL_FLAGBITS) ];
	DWORD				dwModelFlags_MustHaveSome[ DWORD_FLAG_SIZE(NUM_MODEL_FLAGBITS) ];
	DWORD				dwModelFlags_MustNotHave [ DWORD_FLAG_SIZE(NUM_MODEL_FLAGBITS) ];

	DWORD				dwMeshFlags_MustHaveAll;
	DWORD				dwMeshFlags_MustHaveSome;
	DWORD				dwMeshFlags_MustNotHave;

	SORT_CRITERIA		eMeshSort;
	DRAW_LAYER			eDrawLayer;

	int					nMeshListID;
	int					nSortIndex;

	void Reset()
	{
		structclear( (*this) );
		eMeshSort   = SORT_INVALID;
		eDrawLayer  = DRAW_LAYER_DEFAULT;
		nMeshListID = INVALID_ID;
		nSortIndex	= INVALID_ID;
	}
};

struct RenderPassDefinitionSortable
{
	union
	{
		int						nSort_1;
		int						nSection;
	};
	union
	{
		int						nSort_2;
		int						nPass;
	};
	RenderPassDefinition *		pRPD;
	int							nViewParams;

	static int Compare( const void * pE1, const void * pE2 )
	{
		const RenderPassDefinitionSortable * pS1 = static_cast<const RenderPassDefinitionSortable *>( pE1 );
		const RenderPassDefinitionSortable * pS2 = static_cast<const RenderPassDefinitionSortable *>( pE2 );

		if ( pS1->nSort_1 < pS2->nSort_1 )
			return -1;
		if ( pS1->nSort_1 > pS2->nSort_1 )
			return 1;
		if ( pS1->nSort_2 < pS2->nSort_2 )
			return -1;
		if ( pS1->nSort_2 > pS2->nSort_2 )
			return 1;
		return 0;
	}
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline BOOL dxC_PassTypeGetAlpha( RENDER_PASS_TYPE ePass )
{
	extern BOOL gbRPDAlpha[ NUM_RENDER_PASS_TYPES ];
	return gbRPDAlpha[ ePass ];
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------


PRESULT dxC_PassDefinitionGet(
	RENDER_PASS_TYPE ePass,
	RenderPassDefinition & tRPD );

PRESULT dxC_PassEnumGetString(
	RENDER_PASS_TYPE ePass,
	WCHAR * pwszString,
	int nBufLen );

}; // namespace FSSE

#endif // __DXC_PASS__
