
#include "interpolationpath.h"

#include "perfhier.h"

template class CInterpolationPath <float>;
template class CInterpolationPath <CFloatPair>;
template class CInterpolationPath <CFloatTriple>;


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <class CDataType>
int CInterpolationPath<CDataType>::AddPoint ( const CDataType & tData, float fTime )
{

	// if we already have this time point, then just change the data
	for ( int nIndex = 0; nIndex < (int) m_tPoints.Count(); nIndex++ )
	{
		CInterpolationPoint * pPoint = GetPoint( nIndex );
		if ( pPoint->m_fTime == fTime )
		{
			pPoint->m_tData = tData;
			return nIndex;
		}

		if ( pPoint->m_fTime > fTime )
			break;
	}

	CInterpolationPoint tPoint;
	tPoint.m_tData  = tData;
	tPoint.m_fTime = fTime;

	ArrayAddItem(m_tPoints, tPoint);
	Sort();

	for ( int nIndex = 0; nIndex < (int) m_tPoints.Count(); nIndex++ )
	{
		if ( GetPoint (nIndex)->m_fTime == fTime )
			return nIndex;
	}
	return INVALID_ID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

template <class CDataType>
void CInterpolationPath<CDataType>::DeletePoint ( int nIndex )
{
	if ((unsigned int)nIndex < (unsigned int)m_tPoints.Count())
	{
		ArrayRemove(m_tPoints, nIndex);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void CInterpolationPath<float>		::SetDefault ( float fValue ) { m_tDefault = fValue; }
void CInterpolationPath<CFloatPair>	::SetDefault ( float fValue ) { m_tDefault = CFloatPair(fValue, fValue); }
void CInterpolationPath<CFloatTriple>::SetDefault( float fValue ) { m_tDefault = CFloatTriple(fValue, fValue, fValue); }

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <class CDataType>
CDataType	CInterpolationPath<CDataType>	::GetAverage ( float fTime, int nIndex ) const
{	
	const CInterpolationPoint * ptFirst  = GetPoint ( nIndex - 1 );
	const CInterpolationPoint * ptSecond = GetPoint ( nIndex );
	CDataType tAverage = ( fTime - ptFirst->m_fTime  ) * ptSecond->m_tData 
					  +  ( ptSecond->m_fTime - fTime ) * ptFirst ->m_tData;
	return tAverage / ( ptSecond->m_fTime - ptFirst->m_fTime);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <class CDataType>
void CInterpolationPath<CDataType>::Copy ( const CInterpolationPath<CDataType> * pPath )
{
	ArrayClear(m_tPoints);
	for ( int nPoint = 0; nPoint < pPath->GetPointCount(); nPoint++ )
	{
		const CInterpolationPoint * pPoint = pPath->GetPoint( nPoint );
		ArrayAddItem(m_tPoints, *pPoint);
	}
	m_tDefault = pPath->m_tDefault;
	Sort();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <class CDataType>
CDataType CInterpolationPath<CDataType>::GetValue ( float fTime ) const
{
#if !ISVERSION(SERVER_VERSION)
	HITCOUNT( OTHER_GETVALUE )
#endif //!SERVER_VERSION
	if ( 0 == m_tPoints.Count() )
		return m_tDefault;

	// If we are less than the first one, then return the first one
	const CInterpolationPoint * ptFirst = GetPoint ( 0 );
	if ( fTime < ptFirst->m_fTime )
	{
		return ptFirst->m_tData;
	}

	int nPoints = m_tPoints.Count();
	for (int i = 1; i < nPoints; i++)
	{
		ptFirst++;
		if (fTime < ptFirst->m_fTime )
		{
			return GetAverage ( fTime, i );
		}
	}
	return ptFirst->m_tData;
}

////-------------------------------------------------------------------------------------------------
////-------------------------------------------------------------------------------------------------
template <class CDataType>
static int sCompare(const void * ptFirst, const void * ptSecond)
{
	ASSERT_RETX( ptFirst && ptSecond ,-1);

	CInterpolationPath<CDataType>::CInterpolationPoint * ptPointFirst = 
		(CInterpolationPath<CDataType>::CInterpolationPoint * ) ptFirst;
	CInterpolationPath<CDataType>::CInterpolationPoint * ptPointSecond = 
		(CInterpolationPath<CDataType>::CInterpolationPoint * ) ptSecond;

	if ( ptPointFirst->m_fTime == ptPointSecond->m_fTime )
		return 0;
	else if ( ptPointFirst->m_fTime > ptPointSecond->m_fTime )
		return 1;
	else
		return -1;
}
void CInterpolationPath<float>		 ::Sort() {	m_tPoints.Sort( sCompare<float> );			}
void CInterpolationPath<CFloatPair>  ::Sort() {	m_tPoints.Sort( sCompare<CFloatPair> );		}
void CInterpolationPath<CFloatTriple>::Sort() {	m_tPoints.Sort( sCompare<CFloatTriple> );	}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void TestInterpolationPath ()
{
	CInterpolationPath <float> tFloatPath;

	float fFirst  = 10.0f;
	float fSecond = 60.0f;
	float fThird  =100.0f;
	tFloatPath.AddPoint ( fSecond, 0.5f );
	ASSERT (tFloatPath.GetValue ( 0.0f ) == fSecond);
	ASSERT (tFloatPath.GetValue ( 0.5f ) == fSecond);
	ASSERT (tFloatPath.GetValue ( 1.0f ) == fSecond);

	tFloatPath.AddPoint ( fFirst, 0.0f );
	ASSERT (tFloatPath.GetValue ( 0.0f ) == fFirst);
	ASSERT (tFloatPath.GetValue ( 0.5f ) == fSecond);
	ASSERT (tFloatPath.GetValue ( 1.0f ) == fSecond);
	ASSERT (tFloatPath.GetValue ( 0.25f ) == (fSecond + fFirst) / 2.0f );

	tFloatPath.AddPoint ( fThird, 1.0f );
	ASSERT (tFloatPath.GetValue ( 0.0f ) == fFirst);
	ASSERT (tFloatPath.GetValue ( 0.5f ) == fSecond);
	ASSERT (tFloatPath.GetValue ( 1.0f ) == fThird);
	ASSERT (tFloatPath.GetValue ( 0.25f ) == (fSecond + fFirst) / 2.0f );
	ASSERT (tFloatPath.GetValue ( 0.75f ) == (fSecond + fThird) / 2.0f );

}