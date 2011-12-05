//----------------------------------------------------------------------------
// datatables.h
//----------------------------------------------------------------------------
#ifdef	_DATATABLES_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _DATATABLES_H_


enum EXCELTABLE 
{
	DATATABLE_NONE = -1,

#define DATATABLE_ENUM(e)			e,
	#include "../data_common/excel/exceltables_hdr.h"
#undef DATATABLE_ENUM

	NUM_DATATABLES,
};


#endif // _DATATABLES_H_

