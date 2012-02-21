
/********************************************************************
	����:		2012��2��20��
	�ļ���: 	MathUtil.h
	������:		������
	����:		�༭�����õ���ѧ����	
*********************************************************************/

#pragma once

#define gf_PI  float(3.14159265358979323846264338327950288419716939937510)		// pi
#define gf_PI2 float(3.14159265358979323846264338327950288419716939937510*2.0)	// 2*pi
#define DEG2RAD( a ) ( (a) * (gf_PI/180.0f) )
#define RAD2DEG( a ) ( (a) * (180.0f/gf_PI) )
#define MAX(a,b)    (((a) > (b)) ? (a) : (b))
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))

typedef D3DXVECTOR2 	Float2;
typedef D3DXVECTOR3 	Float3;
typedef D3DXVECTOR3 	Angle;
typedef D3DXVECTOR4 	Float4;
typedef D3DXQUATERNION	Quaternion;

namespace MathUtil
{
	INT		IntRound(double f);

	double	Round( double fVal, double fStep );

	bool	IsFloat3Zero(const Float3& vec);

	template<class T> inline T ClampTpl( T X, T Min, T Max ) 
	{	
		return X<Min ? Min : X<Max ? X : Max;
	}


}
