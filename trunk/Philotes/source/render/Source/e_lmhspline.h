
#pragma once

/*
	Hermite-Curve(Points & Tangents given)
	Bezier-Curve(Points & Tangents given)
		the same with other factors for each point
	
	Cardinal-Splines(Points given)
		subset of Hermite-Curve, Tangents are generated
		
		Catmull-Rom-Splines
			subset of Cardinal-Splines with a=0.5
			
	TCB-Splines(Points given)
		subset of Hermite-Curve with Tangents generated with special parameters

	SpeedConst-Idea taken from edeltorus(Nils Pipenbrink)
*/

#include "e_lmvector.h"
#include "e_lmarray.h"

#include "e_lmmin.h"
#include "e_lm_eval.h"

namespace LMSPLINE
{;

template <class T> class HSpline {
public:
	HSpline();
	HSpline(int _numberPoints, T *_points, T *_tangents);
	~HSpline();

	void addPointAtEnd(T &pointToAdd, T &tangenteToAdd);

   T &getPoint(int index) {
      return mPoints[index];
   }
   T &getTangent(int index) {
      return mTangents[index];
   }
   void setPoint(int index, T &pointToSet) {
      mPoints[index] = pointToSet;
   }
   void setTangent(int index, T &tangentToSet) {
      mTangents[index] = tangentToSet;
   }

	void setupCardinal(float a);
	void setupTCB(float tension, float continuity, float bias);
	void setupTimeConstant(int numberSamples = 1000);

	T calcPoint(float t);
	T calcTCBPoint(float t);
	T calcPointTimeConstant(float t);

	int getNumberPoints() {
		return mPoints.getSize();
	}

	bool isTimeConstantAvailable() { return mTimeConstantAvailable; }
	float getLength() { return mSplineLength; }
	
protected:
	bool mTimeConstantAvailable;

	typedef struct {
		float segmentLength;
		int endIndex;
		float p1, p2;
	}TimeConstantValue;

	array<T> mPoints;
	array<T> mTangents;
	array<T> mIncomingTCB;		//incoming tangents with TCB splines

	float mSplineLength;

	array<TimeConstantValue> mTimeConstantValues;
};

//----------------------------------------------------------------------------------
// HSpline
//----------------------------------------------------------------------------------
// 
// 01.11.2003
template <class T> HSpline<T>::HSpline() 
: mTimeConstantAvailable(false)
{
}


//----------------------------------------------------------------------------------
// HSpline
//----------------------------------------------------------------------------------
// 
// 01.11.2003
template <class T> HSpline<T>::HSpline(int _numberPoints,T *_points, T *_tangents) 
: mPoints( _numberPoints, 10)
, mTangents( _numberPoints, 10)
, mIncomingTCB( _numberPoints, 10)
{
	int i;
	for(i=0; i<_numberPoints; i++) {
		mPoints[i] = _points[i];
		mTangents[i] = _tangents[i];
	}
}


//----------------------------------------------------------------------------------
// HSpline
//----------------------------------------------------------------------------------
// 
// 01.11.2003
template <class T> HSpline<T>::~HSpline() {
}


//----------------------------------------------------------------------------------
// addPoint
//----------------------------------------------------------------------------------
// 
// 01.11.2003
template <class T> void HSpline<T>::addPointAtEnd(T &pointToAdd, T &tangenteToAdd) {
	mPoints.add(pointToAdd);
	mTangents.add(tangenteToAdd);
}


//----------------------------------------------------------------------------------
// setupCardinal
//----------------------------------------------------------------------------------
// 
// 01.11.2003
template <class T> void HSpline<T>::setupCardinal(float a) {
	// Tangenten für Cardinalspline berechnen (erste und letzte Tangente muss schon stimmen!)
	int i;
	for(i=1;i<mPoints.getSize()-1;i++) {
		mPoints[i] = (mPoints[i+1] - mPoints[i-1]) * a;
	}
}


//----------------------------------------------------------------------------------
// setupTCB
//----------------------------------------------------------------------------------
// 
// 01.11.2003
template <class T> void HSpline<T>::setupTCB(float tension,float continuity,float bias) {
	int i;
	for(i=1;i<mPoints.getSize()-1;i++) {
		//eingehende Tangente berechnen:
		float ein1,ein2;
		ein1=((1-tension)*(1-continuity)*(1+bias))/2;
		ein2=((1-tension)*(1+continuity)*(1-bias))/2;

		mIncomingTCB[i-1] = (mPoints[i] - mPoints[i-1])*ein1 + (mPoints[i+1] - mPoints[i])*ein2;

		float aus1,aus2;
		aus1=((1-tension)*(1+continuity)*(1+bias))/2;
		aus2=((1-tension)*(1-continuity)*(1-bias))/2;

		mTangents[i] = (mPoints[i] - mPoints[i-1])*aus1 + (mPoints[i+1] - mPoints[i])*aus2;
	}
}

//----------------------------------------------------------------------------------
// calcPoint
//----------------------------------------------------------------------------------
// 
// 01.11.2003
template <class T> T HSpline<T>::calcPoint(float t) {
	if(t > mPoints.getSize()-1)
		return mPoints[mPoints.getSize()-1];

	float floorT = (float)floor(t);			//nach unten hin abrunden
	int p1 = (int)floorT;
	t = t - floorT;

	if(t == 0)
		return mPoints[p1];				//gleich richtigen Punkt zurückgeben

	float t2, t3;
	t2 = t * t;
	t3 = t2 * t;
	float h1 =   2 * t3 - 3 * t2 + 1;
	float h2 = - 2 * t3 + 3 * t2;
	float h3 =       t3 - 2 * t2 + t;
	float h4 =       t3 -     t2;

	T result;
	T vp1 = mPoints[p1];
	T vp2 = mPoints[p1+1];
	T vt1 = mTangents[p1];
	T vt2 = mTangents[p1+1];
	result = vp1*h1 + vp2*h2 + vt1*h3 + vt2*h4;
	return result;
}

//----------------------------------------------------------------------------------
// setupTimeConstant
//----------------------------------------------------------------------------------
// 
static double bezierFit(double t, double *p) {			//function for optimising to bezier
	//p0 = 0, p3 = 1
	return 0*(1-t)*(1-t)*(1-t) + p[0]*(1-t)*(1-t)*t + p[1]*(1-t)*t*t + 1*t*t*t;
}
template<class T> void HSpline<T>::setupTimeConstant(int numberSamplesPerSegment) {
	const int numberSamplesInverse = 200;

	int mNumberSamples = numberSamplesPerSegment * (mPoints.getSize()-1);
	mTimeConstantValues.empty();

	//First I sample the curve in 1000 steps to get the entire length and keep the sampled data.
	float *lengthSamples = new float[mNumberSamples+20];
	float length = 0;
	mSplineLength = 0;
	float step = 1.0f / (float)numberSamplesPerSegment;
	lengthSamples[0] = 0;
	T lastPoint = mPoints[0];
	int index = 1;
	float lastT = 0;
	for(float t = step; t<mPoints.getSize()-1; t +=step) {
		if(floor(lastT) < floor(t)) {	//end of one segment reached?
			TimeConstantValue value;
			value.segmentLength = length;
			value.endIndex = index;
			mTimeConstantValues.add(value);
			
			mSplineLength += length;
			length = 0;
		}

		T nextPoint = calcPoint(t);
		
		length += vecLen(nextPoint - lastPoint);	//calculate length of segment
		lastPoint = nextPoint;

		lengthSamples[index] = length;
		++index;
		lastT = t;
	}
	TimeConstantValue value;
	value.segmentLength = length;
	value.endIndex = index;
	mTimeConstantValues.add(value);
	mSplineLength += length;

	/*Then I normalize the data by dividing by the segment length. The data gets inverted in a way 
	that I have a lookup table that maps length to t. 
	Then I fit a bezier to the data. I do least sqare error fitting (pretty easy) and I can throw 
	away two of the 4 control points since the curve starts always at 0 and ends at 1. So in the 
	end I have to store 3 additional floats per bezier: The segment length and the two control points 
	of the length-to-t speed compensation bezier.*/
	int baseIndex = 0;
	int i;
	double *inverseFunction = new double[numberSamplesInverse+10];
	double *tArray = new double[numberSamplesInverse+10];
	float time = 0.0f;
	step = 1.0f / numberSamplesInverse;
	for(int j=0; j<numberSamplesInverse; j++) {
		tArray[j] = time;
		time += step;
	}

	for(i=0; i<mTimeConstantValues.getSize(); i++) {
		//now linearise the function
		float segLength = mTimeConstantValues[i].segmentLength;

		int actIndex = baseIndex;
		int writeIndex = 1;
		float stepSize = segLength / numberSamplesInverse;
		inverseFunction[0] = 0.0f;
		for(float t=stepSize; t<segLength-stepSize; t += stepSize) {
			while(lengthSamples[actIndex] < t)
				actIndex++;
			//now find exact value with linear interpolation
			float value = (actIndex-1) + (t-lengthSamples[actIndex-1])/(lengthSamples[actIndex]-lengthSamples[actIndex-1]) * (actIndex-(actIndex-1));
			inverseFunction[writeIndex++] = (value-baseIndex) / (float)(mTimeConstantValues[i].endIndex - baseIndex);
		}
		inverseFunction[numberSamplesInverse-1] = 1.0f;		//notwendig?

		//now fit bezier trough data
		lm_control_type lmControl;
		lm_data_type lmData;
		lm_initialize_control(&lmControl);
		lmData.user_func = bezierFit;
		lmData.user_t = tArray;
		lmData.user_y = inverseFunction;
		
		int nParameter = 2;
		double parameter[2] = {1., 1.};		//not 0!
		lm_minimize(numberSamplesInverse, nParameter, parameter, lm_evaluate_default, lm_print_default, &lmData, &lmControl);
		
		//in parameter[] stehen jetzt die beiden werte...
		mTimeConstantValues[i].p1 = (float)parameter[0];
		mTimeConstantValues[i].p2 = (float)parameter[1];

		baseIndex = mTimeConstantValues[i].endIndex;
	}
	delete[] tArray;
	delete[] inverseFunction;
	delete[] lengthSamples;

	mTimeConstantAvailable = true;
}

//----------------------------------------------------------------------------------
// calcPointTimeConstant
//----------------------------------------------------------------------------------
// 
template<class T> T HSpline<T>::calcPointTimeConstant(float pos) {
	/*For a lookup I just have to normalize the "position" along the segment, do a bezier interpolation 
	to get the interpolant and then pass this value to the 3d bezier.*/

	int segIndex = 0;
	float lengthSoFar = mTimeConstantValues[0].segmentLength;
	float segStart = 0;
	while(pos > lengthSoFar) {
		segStart += mTimeConstantValues[segIndex].segmentLength;
		segIndex++;
		lengthSoFar += mTimeConstantValues[segIndex].segmentLength;
	}

	float	segPos = (pos-segStart) / mTimeConstantValues[segIndex].segmentLength;

	float p1 = mTimeConstantValues[segIndex].p1;
	float p2 = mTimeConstantValues[segIndex].p2;

	//float value = p0*(1-t)*(1-t)*(1-t) + p1*(1-t)*(1-t)*t + p2*(1-t)*t*t + p3*t*t*t;      p0=0; p3=1
	float value = p1*(1-segPos)*(1-segPos)*segPos + p2*(1-segPos)*segPos*segPos + segPos*segPos*segPos;

	return calcPoint(segIndex + value);
}

//----------------------------------------------------------------------------------
// calcTCBPoint
//----------------------------------------------------------------------------------
// 
// 01.11.2003
template <class T> T HSpline<T>::calcTCBPoint(float t) {
	float t2,t3,floorT;
	int p1;

	floorT=(float)floor(t);			//nach unten hin abrunden
	p1=(int)floorT;					//an richtigen Punkt springen
	t=t-floorT;

	if(t==0)
		return mPoints[p1];			//gleich richtigen Punkt zurückgeben

	t2=t*t;
	t3=t2*t;
	float h1 =  2*t3 - 3*t2 + 1;
	float h2 = -2*t3 + 3*t2;
	float h3 =    t3 - 2*t2 + t;
	float h4 =    t3 -   t2;

	T result;
	result = mPoints[p1]*h1 + mPoints[p1+1]*h2 + mTangents[p1]*h3 + mIncomingTCB[p1+1]*h4;
	return result;
}

}; // LMSPLINE