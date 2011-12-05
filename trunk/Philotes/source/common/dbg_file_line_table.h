//-------------------------------------------------------------------------------------------------
// dbg_file_line_table.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//-------------------------------------------------------------------------------------------------
#ifndef _DBG_FILE_LINE_TABLE_H_
#define _DBG_FILE_LINE_TABLE_H_



//-------------------------------------------------------------------------------------------------
// CLASSES
//-------------------------------------------------------------------------------------------------
#define DBG_FILE_LINE_HASH	256

class DbgFileLineNode
{
public:
	const char*			file;
	int					line;
	DbgFileLineNode*	next;
	int					count;
};

typedef BOOL FP_DBG_FILE_LINE_ITERATOR(DbgFileLineNode* node, DWORD_PTR data1, DWORD_PTR data2);

class DbgFileLineTable
{
	DbgFileLineNode*	hash[DBG_FILE_LINE_HASH];

public:
	DbgFileLineTable(
		void)
	{
		memclear(hash, DBG_FILE_LINE_HASH * sizeof(DbgFileLineNode*));
	}

	void CleanUp(
		void)
	{
		for (int ii = 0; ii < DBG_FILE_LINE_HASH; ii++)
		{
			DbgFileLineNode* cur = hash[ii];
			while (cur)
			{
				DbgFileLineNode* next = cur->next;
				FREE(NULL, cur);
				cur = next;
			}
		}
		memclear(hash, DBG_FILE_LINE_HASH * sizeof(DbgFileLineNode*));
	}

	~DbgFileLineTable(
		void)
	{
		CleanUp();
	}

	void Add(
		const char* file,
		int line)
	{
		unsigned int key = (unsigned int)((CAST_PTR_TO_INT(file) + line) * 46061) % DBG_FILE_LINE_HASH;
		DbgFileLineNode* cur = hash[key];
		while (cur)
		{
			if (cur->file == file && cur->line == line)
			{
				break;
			}
			cur = cur->next;
		}
		if (!cur)
		{
			cur = (DbgFileLineNode*)MALLOCZ(NULL, sizeof(DbgFileLineNode));
			cur->file = file;
			cur->line = line;
			cur->next = hash[key];
			hash[key] = cur;
		}
		cur->count++;
	}

	void Sub(
		const char* file,
		int line)
	{
		unsigned int key = (unsigned int)((CAST_PTR_TO_INT(file) + line)* 46061) % DBG_FILE_LINE_HASH;
		DbgFileLineNode* cur = hash[key];
		while (cur)
		{
			if (cur->file == file && cur->line == line)
			{
				cur->count--;
				break;
			}
			cur = cur->next;
		}
	}

	void Iterate(
		FP_DBG_FILE_LINE_ITERATOR* iterator,
		DWORD data1,
		DWORD data2)
	{
		for (int ii = 0; ii < DBG_FILE_LINE_HASH; ii++)
		{
			DbgFileLineNode* cur = hash[ii];
			while (cur)
			{
				if (!iterator(cur, data1, data2))
				{
					return;
				}
				cur = cur->next;
			}
		}
	}
};


//-------------------------------------------------------------------------------------------------
// STRUCTS
//-------------------------------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
struct EVENT_SYSTEM_DEBUG
{
	int						num_events;
	int						num_immediate;
	class DbgFileLineTable*	fltbl;
};
#endif


#endif // _DBG_FILE_LINE_TABLE_H_