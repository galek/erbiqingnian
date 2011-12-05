//-----------------------------------------------------------------------------
// jobs.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// INCLUDE
//-----------------------------------------------------------------------------

#include "jobs.h"
#include "list.h"


//-----------------------------------------------------------------------------
// MACROS
//-----------------------------------------------------------------------------
// move to common or whatever when done
#define INT_TO_VOID_PTR(n)			(void *)((char *)0 + (n))
#define VOID_PTR_TO_INT(p)			(int)((char *)(p) - (char *)0)


//-----------------------------------------------------------------------------
// CONSTANTS
//-----------------------------------------------------------------------------
#define WORKER_PRIORITY				THREAD_PRIORITY_NORMAL			// worker thread priority

enum
{
	QUIT_EVENT	=					0,
	JOB_LITTLE_EVENT =				1,
	JOB_BIG_EVENT =					2,
	NUM_JOB_EVENTS
};


//-----------------------------------------------------------------------------
// STRUCT
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// STRUCT
//-----------------------------------------------------------------------------
struct JOB : LIST_DL_NODE
{
	JOB_TYPE			eJobType;
	PFN_JOB_EXECUTE		fpExecute;
	PFN_JOB_COMPLETE	fpComplete;
	PFN_JOB_CLEANUP		fpCleanup;
	JOB_DATA			data;
};


DWORD WINAPI JobWorkerThread(
	LPVOID param);


struct JOB_SYSTEM
{
	LIST_CIRC_MT		todo_queue[NUM_JOB_TYPES];
	LIST_CIRC_MT		done_queue;			// may need to expand this to one per calling thread
	LIST_SING_MT		garbage;
	HANDLE				events[NUM_JOB_EVENTS];
	volatile long		quit;
	volatile long		threads_requested;
	volatile long		threads_active;

	JOB * GetNew(
		void)
	{
		JOB * job = (JOB *)garbage.Pop();
		if (!job)
		{
			job = (JOB *)MALLOCZ(NULL, sizeof(JOB));
		}
		return job;
	}

	void Recycle(
		JOB * job)
	{
		ASSERT_RETURN(job);
		memclear(job, sizeof(JOB));
		garbage.Push(job);
	}

	HJOB Submit(
		JOB_TYPE eJobType,
		PFN_JOB_EXECUTE	fpExecute,
		const JOB_DATA & data,
		PFN_JOB_COMPLETE fpComplete,
		PFN_JOB_CLEANUP	fpCleanup)
	{
		ASSERT_RETNULL(eJobType >= 0 && eJobType < NUM_JOB_TYPES);
		ASSERT_RETNULL(fpExecute);

		JOB * job = GetNew();
		ASSERT_RETNULL(job);

		job->data = data;
		job->eJobType = eJobType;
		job->fpExecute = fpExecute;
		job->fpComplete = fpComplete;
		job->fpCleanup = fpCleanup;

		todo_queue[eJobType].InsertTail(job);

		SetEvent(events[1 + eJobType]);

		return (HJOB)(&job->data);		// note that execute could be called by this point, but complete can't, and thus job is still valid
	}

	BOOL Execute(
		JOB_TYPE eJobType)
	{
		JOB * job = (JOB *)todo_queue[eJobType].Pop();
		if (!job)
		{
			return FALSE;
		}

		if (!job->data.cancel && job->fpExecute)
		{
			job->fpExecute(job->data);
		}

		if (job->fpComplete)
		{
			done_queue.InsertTail(job);
		}
		else
		{
			Recycle(job);
		}

		return TRUE;
	}

	void Update(
		void)
	{
		LIST_DL_NODE * head = NULL;
		LIST_DL_NODE * tail = NULL;
		done_queue.PopList(head, tail);

		JOB * curr = NULL;
		JOB * next = (JOB *)head;
		while ((curr = next) != NULL)
		{
			next = (JOB *)curr->m_List_Next;

			if (curr->fpComplete)
			{
				curr->fpComplete(curr->data);
			}
			Recycle(curr);
		}
	}

	void CleanupQueue(
		LIST_CIRC_MT & queue)
	{
		LIST_DL_NODE * head = NULL;
		LIST_DL_NODE * tail = NULL;
		queue.PopList(head, tail);

		JOB * curr = NULL;
		JOB * next = (JOB *)head;
		while ((curr = next) != NULL)
		{
			next = (JOB *)curr->m_List_Next;

			if (curr->fpCleanup)
			{
				curr->fpCleanup(curr->data);
			}
			FREE(NULL, curr);
		}
	}

	void FreeGarbage(
		void)
	{
		LIST_SL_NODE * head = NULL;
		garbage.PopList(head);

		JOB * curr = NULL;
		JOB * next = (JOB *)head;
		while ((curr = next) != NULL)
		{
			next = (JOB *)curr->m_List_Next;
			FREE(NULL, curr);
		}
	}

	void FreeEvents(
		void)
	{
		for (unsigned int ii = 0; ii < NUM_JOB_EVENTS; ++ii)
		{
			if (events[ii])
			{
				CloseHandle(events[ii]);
				events[ii] = NULL;
			}
		}
	}

	void KillWorkerThreads(
		void)
	{
		// kill worker threads
		if (!events[QUIT_EVENT])
		{
			return;
		}
		quit = TRUE;
		SetEvent(events[QUIT_EVENT]);

		unsigned int iter = 0;
		while (threads_active || threads_requested)
		{
			Sleep(50);
			if (++iter > 60)
			{
				ASSERT(0);
				return;
			}
		}

		Update();		// this may need to get called in each thread
	}

	void Free(
		void)
	{
		KillWorkerThreads();

		for (unsigned int ii = 0; ii < NUM_JOB_TYPES; ++ii)
		{
			CleanupQueue(todo_queue[ii]);
		}

		FreeGarbage();

		FreeEvents();
	}

	BOOL InitEvents(
		void)
	{
		if ((events[QUIT_EVENT] = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)	// manual reset
		{
			ASSERT(0);
			return FALSE;
		}
		for (unsigned int ii = 1; ii < NUM_JOB_EVENTS; ++ii)				// auto reset
		{
			if ((events[ii] = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
			{
				ASSERT(0);
				return FALSE;
			}
		}
		return TRUE;
	}

	BOOL InitWorkerThreads(
		void)
	{
		quit = FALSE;
		threads_requested = 0;
		threads_active = 0;

		CPU_INFO info;
		ASSERT_RETFALSE(CPUInfo(info));
		ASSERT_RETFALSE(info.m_AvailCore > 0);
		unsigned int num_job_threads = MAX((unsigned int)2, (unsigned int)info.m_AvailCore);

		for (unsigned int ii = 0; ii < num_job_threads; ++ii)
		{
			InterlockedIncrement(&threads_requested);

			DWORD dwThreadId;
			HANDLE hWorker = (HANDLE)CreateThread(NULL,	0, JobWorkerThread, INT_TO_VOID_PTR(ii), 0, &dwThreadId);
			if (hWorker == NULL)
			{
				ASSERT(0);
				return FALSE;
			}
			SetThreadPriority(hWorker, WORKER_PRIORITY);
			CloseHandle(hWorker);
		}
		return TRUE;
	}

	// only call init from a single thread, don't call any other job functions from other threads until init is complete
	BOOL Init(
		void)
	{
		for (unsigned int ii = 0; ii < NUM_JOB_TYPES; ++ii)
		{
			todo_queue[ii].Init();
		}
		done_queue.Init();
		garbage.Init();

		// create worker threads and triggers
		if (!InitEvents())
		{
			Free();
			return FALSE;
		}

		if (!InitWorkerThreads())
		{
			Free();
			return FALSE;
		}

		return TRUE;
	}
};


//-----------------------------------------------------------------------------
// GLOBALS
//-----------------------------------------------------------------------------
JOB_SYSTEM gJobs;


// ---------------------------------------------------------------------------
// worker thread
// ---------------------------------------------------------------------------
DWORD WINAPI JobWorkerThread(
	LPVOID param)    
{
	unsigned int thread_id = (unsigned int)VOID_PTR_TO_INT(param);
	InterlockedIncrement(&gJobs.threads_active);
	InterlockedDecrement(&gJobs.threads_requested);

#if ISVERSION(DEBUG_VERSION)
	char thread_name[256];
	PStrPrintf(thread_name, 256, "job_%02d", thread_id);
	SetThreadName(thread_name);
	trace("begin job system worker thread [%s]\n", thread_name);
#endif

	JOB_TYPE eBaseJobType = (JOB_TYPE)(thread_id);		// the job type this thread is primarily responsible for
	unsigned int num_events_to_wait = eBaseJobType + 2;

	while (TRUE)
	{
		DWORD ret = WaitForMultipleObjectsEx(num_events_to_wait, gJobs.events, FALSE, INFINITE, FALSE);
		if (ret <= WAIT_OBJECT_0 || ret >= WAIT_OBJECT_0 + num_events_to_wait)
		{
			break;		// WAIT_OBJECT_0 is quit signal
		}

		// execute from our job types until no more jobs to execute
		while (!gJobs.quit)
		{
			BOOL bDidJob = FALSE;

			for (JOB_TYPE eJobType = eBaseJobType; eJobType >= JOB_TYPE_LITTLE; eJobType = (JOB_TYPE)(eJobType - 1))
			{
				if (gJobs.Execute(eJobType))
				{
					bDidJob = TRUE;
				}
			}

			if (!bDidJob)
			{
				break;
			}
		}
	}

#if ISVERSION(DEBUG_VERSION)
	trace("exit job system worker thread [%s]\n", thread_name);
#endif

	InterlockedDecrement(&gJobs.threads_active);

	return 0;
}


//-----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void c_JobsInit(
	void)
{
	gJobs.Init();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void c_JobsFree(
	void)
{
	gJobs.Free();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void c_JobsUpdate(
	void)
{
	gJobs.Update();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
HJOB c_JobSubmit(
	JOB_TYPE eJobType,
	PFN_JOB_EXECUTE	fpExecute,
	const JOB_DATA & job,
	PFN_JOB_COMPLETE fpComplete,	// default = NULL
	PFN_JOB_CLEANUP	fpCleanup)		// default = NULL
{
	return gJobs.Submit(eJobType, fpExecute, job, fpComplete, fpCleanup);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void c_JobCancel(
	HJOB job)
{
	ASSERT_RETURN(job);

	JOB_DATA * data = (JOB_DATA *)job;
	data->cancel = 1;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
void c_JobTestExecute1(
	JOB_DATA & data)
{
	trace("c_JobTestExecute1(): %d  thread: %d\n", (int)data.data1, GetCurrentThreadId());
	Sleep((DWORD)data.data2);
}


void c_JobTestExecute2(
	JOB_DATA & data)
{
	trace("c_JobTestExecute2(): %d  thread: %d\n", (int)data.data1, GetCurrentThreadId());
	Sleep((DWORD)data.data2);
	Sleep(30);
}


void c_JobTestComplete1(
	JOB_DATA & data)
{
	trace("c_JobTestComplete1(): %d\n", (int)data.data1);
}


void c_JobTestCleanup1(
	JOB_DATA & data)
{
	trace("c_JobTestCleanup1(): %d\n", (int)data.data1);
}


void c_JobTest(
	void)
{
	trace("begin c_JobTest()\n");

	c_JobsInit();

	RAND rand;
	RandInit(rand, 1);

	unsigned int job = 0;
	// submit a bunch of jobs and call update
	for (unsigned int ii = 0; ii < 1000; ++ii)
	{
		unsigned int job_count = RandGetNum(rand, 3);

		for (unsigned int jj = 0; jj < job_count; ++jj)
		{
			JOB_TYPE eJobType = (RandGetNum(rand, 10) == 0 ? JOB_TYPE_BIG : JOB_TYPE_LITTLE);

			trace("submit %d   job type: %d\n", job, eJobType);
			c_JobSubmit(eJobType, (eJobType == JOB_TYPE_LITTLE ? c_JobTestExecute1 : c_JobTestExecute2), 
				JOB_DATA(job, RandGetNum(rand, 0, 10)), c_JobTestComplete1, c_JobTestCleanup1);
			++job;
		}

		c_JobsUpdate();
		
		if (RandGetNum(rand, 2) == 0)
		{
			Sleep(RandGetNum(rand, 5));
		}
	}

	c_JobsFree();

	trace("end c_JobTest()\n");
}
#endif