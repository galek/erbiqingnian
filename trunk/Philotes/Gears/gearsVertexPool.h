
#pragma once

#include <set>

_NAMESPACE_BEGIN

template <class Type> class VertexLess
{
public:
	typedef Array< Type > VertexVector;

	bool operator()(int32 v1,int32 v2) const;

	static void SetSearch(const Type& match,VertexVector *list)
	{
		mFind = match;
		mList = list;
	};

private:

	const Type& Get(int32 index) const
	{
		if ( index == -1 ) return mFind;
		VertexVector &vlist = *mList;
		return vlist[index];
	}

	static Type mFind;

	static VertexVector  *mList;
};

//////////////////////////////////////////////////////////////////////////

template <class Type> class VertexPool
{
public:
	typedef std::set<int32, VertexLess<Type> > VertexSet;
	typedef Array< Type > VertexVector;

	int32 GetVertex(const Type& vtx)
	{
		VertexLess<Type>::SetSearch(vtx,&mVtxs);
		typename VertexSet::iterator found;
		found = mVertSet.find( -1 );
		if ( found != mVertSet.end() )
		{
			return *found;
		}
		int32 idx = (int32)mVtxs.Size();
		mVtxs.Append( vtx );
		mVertSet.insert( idx );
		return idx;
	};

	void GetPos(int32 idx,Vector3& pos) const
	{
		pos = mVtxs[idx].mPos;
	}

	const Type& Get(int32 idx) const
	{
		return mVtxs[idx];
	};

	int32 GetSize(void) const
	{
		return (int32)mVtxs.Size();
	};

	void Clear(int32 reservesize)
	{
		mVertSet.clear();
		mVtxs.Reset();
		mVtxs.Reserve(reservesize);
	};

	const VertexVector& GetVertexList(void) const { return mVtxs; };

	void Set(const Type& vtx)
	{
		mVtxs.Append(vtx);
	}

	int32 GetVertexCount(void) const
	{
		return (int32)mVtxs.size();
	};

	bool GetVertex(int32 i,Vector3& vect) const
	{
		vect = mVtxs[i].mPos;
		return true;
	};


	Type * GetBuffer(void)
	{
		return &mVtxs[0];
	};

private:

	VertexSet      mVertSet; 

	VertexVector   mVtxs;	 
};

_NAMESPACE_END