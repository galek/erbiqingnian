#pragma once




typedef enum 
{
	DURATION_CONSTANT = 0,
	DURATION_ONE_TIME,
	DURATION_LOOPS_SKIP_TO_END,
	DURATION_LOOPS_RUN_TO_END,
}DURATION_TYPE;

//typedef D3DXVECTOR2 CFloatPair;
//typedef D3DXVECTOR3 CFloatTriple;
typedef VECTOR2 CFloatPair;
typedef VECTOR  CFloatTriple;


#if DEBUG_MEMORY_ALLOCATIONS
#define CInterpolationPathInit(path)			(path).Init(__FILE__, __LINE__)
#else
#define CInterpolationPathInit(path)			(path).Init()
#endif


template <class TDataType>
class CInterpolationPath
{
public:
	class CInterpolationPoint
	{
	public:
		float		m_fTime;
		TDataType	m_tData; 
	};

	CInterpolationPath(
		void)
	{
		ArrayInit(m_tPoints, NULL, 1);
	}

	~CInterpolationPath(
		void) 
	{
		Destroy();	
	};

	 // you need to call this if the structure has had ZeroMemory called on it
	void Init(
#if DEBUG_MEMORY_ALLOCATIONS
		const char * file,
		unsigned int line
#endif
		) 
	{ 
		ArrayInitFL(m_tPoints, NULL, 1, file, line);
	}

	int AddPoint(
		const TDataType & tData, 
		float fTime);

	TDataType GetValue(
		float fTime) const;

	CInterpolationPoint * GetPoint (
		int nIndex) 
	{	
		// having it in two lines helps with debugging
		CInterpolationPoint * pPoint = (CInterpolationPoint *) m_tPoints.GetPointer(nIndex);
		return pPoint;
	}

	const CInterpolationPoint * GetPoint (
		int nIndex) const
	{ 
		return (CInterpolationPoint *) m_tPoints.GetPointer(nIndex);
	}

	void DeletePoint(
		int nIndex);

	int GetPointCount(
		void) const 
	{ 
		return m_tPoints.Count(); 
	}

	void Copy(
		const CInterpolationPath<TDataType> * pPath);

	void Clear(void) 
	{ 
		ArrayClear(m_tPoints); 
	}

	void Destroy(
		void) 
	{ 
		m_tPoints.Destroy(); 
	}

	BOOL IsEmpty(
		void) 
	{ 
		return m_tPoints.IsEmpty(); 
	}

	void SetDefault(
		float fDefault);

protected:
	SIMPLE_DYNAMIC_ARRAY<CInterpolationPoint> m_tPoints;
	TDataType m_tDefault;

	TDataType GetAverage(
		float fTime,
		int nIndex) const;

	void Sort(
		void);
};

void TestInterpolationPath ();

