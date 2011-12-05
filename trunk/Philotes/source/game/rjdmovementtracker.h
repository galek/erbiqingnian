#ifndef _RJDMOVEMENTTRACKER_H_
#define _RJDMOVEMENTTRACKER_H_

//----------------------------------------------------------------------------
// ENUM
//Will log the movement and tickrate of any flagged hackers.
//Of course, there could be some unknown glitch causing someone to teleport clientside.
//So take this with a grain of salt.
//----------------------------------------------------------------------------
enum MOVEMENT_HACK_LEVEL
{
	MOVEMENT_HACK_NONE,
	MOVEMENT_HACK_LAGGING,
//	MOVEMENT_HACK_POSSIBLE,
	MOVEMENT_HACK_PROBABLE,
//	MOVEMENT_HACK_DEFINITE
};


//----------------------------------------------------------------------------
// DEFINE
//----------------------------------------------------------------------------

#define EXPECTED_MESSAGES_PER_TICK 1
#define TICK_PROPORTIONAL_TOLERANCE 1.01f
#define TICK_CONSTANT_TOLERANCE 50
#define VELOCITY_CONSTANT_TOLERANCE 100.0f
#define VELOCITY_PROPORTIONAL_TOLERANCE 1.3f
#define DECAY_TRACKERS 2
#define LAG_BURST_TICK_THRESHHOLD 10

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class rjdMovementTracker
{
public:
	int nFlagCount;  //We probably don't want to log every single time we
	//flag, since we may flag 20 times a second.

	rjdMovementTracker() {memclear(this, sizeof(*this));}
	void Reset()	{memclear(this, sizeof(*this));}

	MOVEMENT_HACK_LEVEL AddMovement(
		DWORD dwTick, 
		float fMoveDistance, 
	//We multiply fMoveDistance by GAME_TICKS_PER_SECOND to get the momentary velocity
		float fVelocityExpected); 
#if ISVERSION(SERVER_VERSION) || ISVERSION(DEVELOPMENT)
protected:
	MOVEMENT_HACK_LEVEL rjdMovementTracker::AddTick(DWORD dwTick);
	MOVEMENT_HACK_LEVEL rjdMovementTracker::AddVelocity(
		float fMoveDistance,
		float fVelocityExpected);


	DWORD dwStart;
	DWORD dwLast;

	DWORD nTotalMessages;
	DWORD nMessagesAtCurrentTick;

	DWORD nMessagesAtLastLagBurst;
	DWORD dwLastLagBurst;

	//We window by exponentially decaying with time.
	//Multiple  trackers because we need both short and long windows
	//to really figure out who is lagging and who is cheating.
	float fMessagesDecayed[DECAY_TRACKERS]; 
	float fTicksDecayed[DECAY_TRACKERS];

	double fTotalVelocityActual;
	double fTotalVelocityExpected;

	float fVelocityActualDecayed[DECAY_TRACKERS];
	float fVelocityExpectedDecayed[DECAY_TRACKERS];
#endif //ISVERSION(SERVER_VERSION) || ISVERSION(DEVELOPMENT)
};

#endif //_RJDMOVEMENTTRACKER_H_
