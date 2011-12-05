//----------------------------------------------------------------------------
#ifndef _UNITFILECOMMON_H_
#define _UNITFILECOMMON_H_

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct DATABASE_UNIT;

//----------------------------------------------------------------------------
// Structs
//----------------------------------------------------------------------------
struct INVENTORY_POSITION
{
	INVENTORY_LOCATION	tInvLoc;
	DWORD				dwInvLocationCode;
	UNITID				idContainer;
	TIME				tiEquipTime;
	struct UNIT *		pContainer;
};

//----------------------------------------------------------------------------
struct WORLD_POSITION
{
	ROOMID				idRoom;
	VECTOR				vPosition;
	VECTOR				vDirection;
	VECTOR				vUp;
	unsigned int		nAngle;
	float				flScale;
	float				flVelocityBase;	
	WORLD_POSITION()
	:idRoom(INVALID_ID),
	vPosition(INVALID_POSITION), 
	vDirection(INVALID_DIRECTION), vUp(INVALID_DIRECTION),
	nAngle(0), flScale(1.0f), flVelocityBase(0.0f)
	{}
};

//----------------------------------------------------------------------------
enum POSITION_TYPE
{
	POSITION_TYPE_INVALID = -1,

	POSITION_TYPE_WORLD,
	POSITION_TYPE_INVENTORY,

	NUM_POSITION_TYPES

};

//----------------------------------------------------------------------------
struct UNIT_POSITION
{
	POSITION_TYPE eType;				// describes which of the following are valid
	INVENTORY_POSITION tInvPos;		// position in an inventory information
	WORLD_POSITION tWorldPos;		// position in world information
	UNIT_POSITION()
	:eType(POSITION_TYPE_INVALID), tWorldPos()
	{}
};

//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------
template <XFER_MODE mode>
BOOL UnitFileXferHeader(
	XFER<mode> & xfer,
	UNIT_FILE_HEADER & header);

template BOOL UnitFileXferHeader<XFER_SAVE>(XFER<XFER_SAVE> & xfer, UNIT_FILE_HEADER & header);
template BOOL UnitFileXferHeader<XFER_LOAD>(XFER<XFER_LOAD> & xfer, UNIT_FILE_HEADER & header);


template <XFER_MODE mode>
BOOL UnitPositionXfer(
	GAME * game,
	UNIT * unit,
	XFER<mode> & xfer,
	UNIT_FILE_HEADER & header,
	UNIT_FILE_XFER_SPEC<mode> &tSpec,
	UNIT_POSITION & position,
	DATABASE_UNIT *pDBUnit);

template BOOL UnitPositionXfer<XFER_SAVE>(GAME * game, UNIT * unit, XFER<XFER_SAVE> & xfer, UNIT_FILE_HEADER & header, UNIT_FILE_XFER_SPEC<XFER_SAVE> &tSpec, UNIT_POSITION & position, DATABASE_UNIT *pDBUnit);
template BOOL UnitPositionXfer<XFER_LOAD>(GAME * game, UNIT * unit, XFER<XFER_LOAD> & xfer, UNIT_FILE_HEADER & header, UNIT_FILE_XFER_SPEC<XFER_LOAD> &tSpec, UNIT_POSITION & position, DATABASE_UNIT *pDBUnit);


BOOL UnitFileIsForDisk(
	const UNIT_FILE_HEADER &tHeader);

BOOL UnitFileHasPosition(
	UNIT_FILE_HEADER &tHeader);

template <XFER_MODE mode>
BOOL UnitFileGUIDXfer( 
	XFER<mode> &tXfer, 
	GAME *pGame,
	UNIT_FILE_HEADER &tHeader, 
	UNIT_FILE_XFER_SPEC<mode> &tSpec,
	PGUID *pGuid,
	UNIT *pUnit);

template BOOL UnitFileGUIDXfer<XFER_SAVE>( XFER<XFER_SAVE> &tXfer, GAME *pGame, UNIT_FILE_HEADER &tHeader, UNIT_FILE_XFER_SPEC<XFER_SAVE> &tSpec, PGUID *pGuid, UNIT *pUnit);
template BOOL UnitFileGUIDXfer<XFER_LOAD>( XFER<XFER_LOAD> &tXfer, GAME *pGame, UNIT_FILE_HEADER &tHeader, UNIT_FILE_XFER_SPEC<XFER_LOAD> &tSpec, PGUID *pGuid, UNIT *pUnit);

#endif //_UNITFILECOMMON_H_