//----------------------------------------------------------------------------
// jobs.h
//
// (C)Copyright 2004, Flagship Studios. All rights reserved.
//
// jobs creates a pool of worker threads to execute functions outside of the
// main thread.
//
// currently two queues of jobs are available, which allows small jobs to
// be processed even while the system is tied up executing a larger job.
//
// when a job is submitted, 3 functions are provided:
// execute (required):	callback run by worker thread
// complete (optional):	callback run by main thread after execute completes
// cleanup (optional):	callback from main thread for unexecuted (cancelled) jobs
// note that one of complete or cleanup will be called but not both
//
// ideally, one shouldn't provide a complete function because doing so
// adds additional latency
//----------------------------------------------------------------------------
#ifndef _JOBS_H_
#define _JOBS_H_


//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum JOB_TYPE
{
	JOB_TYPE_LITTLE,			// small jobs take a short amount of time
	JOB_TYPE_BIG,				// large jobs take longer

	NUM_JOB_TYPES,
};


//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct JOB_DATA
{
	volatile long		cancel;
	DWORD_PTR			data1;
	DWORD_PTR			data2;
	DWORD_PTR			data3;
	DWORD_PTR			data4;

	JOB_DATA(
		void) : data1(NULL), data2(NULL), data3(NULL), data4(NULL), cancel(FALSE) {};

	JOB_DATA(
		DWORD_PTR data1_) : data1(data1_), data2(NULL), data3(NULL), data4(NULL), cancel(FALSE) {};

	JOB_DATA(
		DWORD_PTR data1_,
		DWORD_PTR data2_) : data1(data1_), data2(data2_), data3(NULL), data4(NULL), cancel(FALSE) {};

	JOB_DATA(
		DWORD_PTR data1_,
		DWORD_PTR data2_,
		DWORD_PTR data3_) : data1(data1_), data2(data2_), data3(data3_), data4(NULL), cancel(FALSE) {};

	JOB_DATA(
		DWORD_PTR data1_,
		DWORD_PTR data2_,
		DWORD_PTR data3_,
		DWORD_PTR data4_) : data1(data1_), data2(data2_), data3(data3_), data4(data4_), cancel(FALSE) {};
};


//----------------------------------------------------------------------------
// TYPEDEF
//----------------------------------------------------------------------------
typedef void (* PFN_JOB_EXECUTE)(JOB_DATA & data);
typedef void (* PFN_JOB_COMPLETE)(JOB_DATA & data);
typedef void (* PFN_JOB_CLEANUP)(JOB_DATA & data);


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
void c_JobsInit(
	void);

void c_JobsFree(
	void);

void c_JobsUpdate(
	void);

HJOB c_JobSubmit(
	JOB_TYPE eJobType,
	PFN_JOB_EXECUTE	fpExecute,				// occurs in worker thread
	const JOB_DATA & job,
	PFN_JOB_COMPLETE fpComplete = NULL,		// occurs in submit thread
	PFN_JOB_CLEANUP	fpCleanup = NULL);		// occurs in submit thread

// note that cancel doesn't always truly cancel if the job is already being executed
void c_JobCancel(
	HJOB job);

#if ISVERSION(DEVELOPMENT)
void c_JobTest(
	void);
#endif


#endif // _JOBS_H_