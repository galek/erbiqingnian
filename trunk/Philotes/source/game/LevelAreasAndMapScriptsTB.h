#include "importdefines.h"


IMPORT_FUNC( VMCreateMapFromMapSpawner, RETVAL_INT, createMap )
  IMPORT_CONTEXT
IMPORT_END

IMPORT_FUNC( VMRandomizeMapSpawner, RETVAL_INT, randomizeMapSpawner )
  IMPORT_CONTEXT
IMPORT_END


IMPORT_FUNC( VMRandomizeMapSpawnerEpic, RETVAL_INT, randomizeMapSpawnerEpic )
  IMPORT_CONTEXT
IMPORT_END


IMPORT_FUNC(VMCreateRandomDungeonSeed, RETVAL_INT, createRandomDungeonSeedRuneStone )
	IMPORT_CONTEXT,    
	IMPORT_INDEX(DATATABLE_LEVEL_ZONES, nZone ),
	IMPORT_INDEX(DATATABLE_LEVEL, nLevelType ),
	IMPORT_INT( nMin ),
	IMPORT_INT( nMax ),
	IMPORT_INT( nEpic )
IMPORT_END

IMPORT_FUNC( VMSetMapSpawnerByLevelAreaID, RETVAL_INT, setMapSpawnerByLevelAreaID )
  IMPORT_CONTEXT,
  IMPORT_INT( nLevelAreaID )
IMPORT_END

IMPORT_FUNC(VMSetMapSpawnerByLevelAreaID, RETVAL_INT, setMapSpawnerByLevelArea )
  IMPORT_CONTEXT,    
  IMPORT_INDEX(DATATABLE_LEVEL_AREAS, nLevelAreaID )  
IMPORT_END
