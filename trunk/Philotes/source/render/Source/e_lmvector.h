
#pragma once

//#include <math.h>

namespace LMSPLINE
{;

class vector4d {
public:
	inline vector4d() {};
	inline vector4d(float x,float y,float z,float w);
	inline vector4d& operator+=(const vector4d& a);
	inline vector4d operator+(const vector4d& a);
	inline vector4d& operator-=(const vector4d& a);
	inline vector4d operator-(const vector4d& a);
	inline vector4d& operator-();
	inline vector4d& operator*=(const float a);
	inline vector4d operator*(const float a);
	inline void set(float xx,float yy,float zz,float ww);
	float x,y,z,w;
};

vector4d::vector4d(float xx,float yy,float zz,float ww) {
	x=xx;
	y=yy;
	z=zz;
	w=ww;
}
void vector4d::set(float xx,float yy,float zz,float ww) {
	x=xx;
	y=yy;
	z=zz;
	w=ww;
}
vector4d& vector4d::operator+=(const vector4d& a) {
	x+=a.x;
	y+=a.y;
	z+=a.z;
	return *this;
}
vector4d vector4d::operator+(const vector4d& a) {
	vector4d res;
	res.x=x+a.x;
	res.y=y+a.y;
	res.z=z+a.z;
	res.w=1;
	return res;
}
vector4d& vector4d::operator-=(const vector4d& a) {
	x-=a.x;
	y-=a.y;
	z-=a.z;
	return *this;
}
vector4d vector4d::operator-(const vector4d& a) {
	vector4d res;
	res.x=x-a.x;
	res.y=y-a.y;
	res.z=z-a.z;
	res.w=1;
	return res;
}
vector4d& vector4d::operator-() {
	x = -x;
	y = -y;
	z = -z;
	return *this;
}
vector4d& vector4d::operator*=(const float a) {
	x*=a;
	y*=a;
	z*=a;
	return *this;
}
vector4d vector4d::operator*(const float a) {
	vector4d res;
	res.x=x*a;
	res.y=y*a;
	res.z=z*a;
	res.w=w;
	return res;
}

//----------------------------------------------------------------

class vector3d {
public:
	inline vector3d() {};
	inline vector3d(float x,float y,float z);
	inline vector3d& operator+=(const vector3d& a);
	inline vector3d operator+(const vector3d& a);
	inline vector3d& operator-=(const vector3d& a);
	inline vector3d operator-(const vector3d& a);
	inline vector3d& operator-();
	inline vector3d& operator*=(const float a);
	inline vector3d operator*(const float a);
	inline void set(float xx,float yy,float zz);
	float x,y,z;
};
vector3d::vector3d(float xx,float yy,float zz) {
	x=xx;
	y=yy;
	z=zz;
}
void vector3d::set(float xx,float yy,float zz) {
	x=xx;
	y=yy;
	z=zz;
}
vector3d& vector3d::operator+=(const vector3d& a) {
	x+=a.x;
	y+=a.y;
	z+=a.z;
	return *this;
}
vector3d vector3d::operator+(const vector3d& a) {
	vector3d res;
	res.x=x+a.x;
	res.y=y+a.y;
	res.z=z+a.z;
	return res;
}
vector3d& vector3d::operator-=(const vector3d& a) {
	x-=a.x;
	y-=a.y;
	z-=a.z;
	return *this;
}
vector3d vector3d::operator-(const vector3d& a) {
	vector3d res;
	res.x=x-a.x;
	res.y=y-a.y;
	res.z=z-a.z;
	return res;
}
vector3d& vector3d::operator-() {
	x = -x;
	y = -y;
	z = -z;
	return *this;
}
vector3d& vector3d::operator*=(const float a) {
	x*=a;
	y*=a;
	z*=a;
	return *this;
}
vector3d vector3d::operator*(const float a) {
	vector3d res;
	res.x=x*a;
	res.y=y*a;
	res.z=z*a;
	return res;
}

//----------------------------------------------------------------

class vector2d {
public:
	inline vector2d() {};
	inline vector2d(float x,float y);
	inline vector2d& operator+=(const vector2d& a);
	inline vector2d operator+(const vector2d& a);
	inline vector2d& operator-=(const vector2d& a);
	inline vector2d operator-(const vector2d& a);
	inline vector2d& operator-();
	inline vector2d& operator*=(const float a);
	inline vector2d operator*(const float a);
	inline void set(float xx,float yy);
	float x,y;
};
inline float vecLen(const vector2d& a);
inline vector2d normalize(const vector2d& a);

vector2d::vector2d(float xx,float yy) {
	x=xx;
	y=yy;
}
void vector2d::set(float xx,float yy) {
	x=xx;
	y=yy;
}
vector2d& vector2d::operator+=(const vector2d& a) {
	x+=a.x;
	y+=a.y;
	return *this;
}
vector2d vector2d::operator+(const vector2d& a) {
	vector2d res;
	res.x=x+a.x;
	res.y=y+a.y;
	return res;
}
vector2d vector2d::operator-(const vector2d& a) {
	vector2d res;
	res.x=x-a.x;
	res.y=y-a.y;
	return res;
}
vector2d& vector2d::operator-() {
	x = -x;
	y = -y;
	return *this;
}
vector2d vector2d::operator*(const float a) {
	vector2d res;
	res.x=x*a;
	res.y=y*a;
	return res;
}
vector2d& vector2d::operator*=(const float a) {
	x*=a;
	y*=a;
	return *this;
}
vector2d& vector2d::operator-=(const vector2d& a) {
	x-=a.x;
	y-=a.y;
	return *this;
}

float vecLen(const vector2d& a) {
	return (float)sqrt(a.x*a.x + a.y*a.y);
}

//----------------------------------------------------------------
//----------------------------------------------------------Ebenen
//----------------------------------------------------------------
class plane3d {
public:
	void set(vector4d &a,vector4d &b, vector4d &c);
	void set(vector3d &a,vector3d &b, vector3d &c);

	float calcDistance(vector3d &v);
	float calcDistance(vector4d &v);

	vector3d normal;
	float distance;
};

//----------------------------------------------------------------
//--------------------------------------------------------Matrizen
//----------------------------------------------------------------
class matrix33 {
public:
	matrix33& operator+=(const matrix33& a);	
	matrix33 operator+(const matrix33& a);
	matrix33& operator-=(const matrix33& a);
	matrix33 operator-(const matrix33& a);
	matrix33& operator*=(const float a);	
	matrix33 operator*(const float a);
	matrix33 operator*=(const matrix33& a);
	matrix33 operator*(const matrix33& a);
	vector3d    operator*=(const vector3d& a);
	vector3d    operator*(const vector3d& a);

	float _11,_12,_13;
    float _21,_22,_23;
    float _31,_32,_33;
};
inline float skalProd(const vector3d& a,const vector3d& b);
inline float vecLen(const vector3d& a);
inline float angle(const vector3d& a,const vector3d& b);
inline vector3d normalize(const vector3d& a);
inline vector3d negatize(const vector3d& a);
inline vector3d crossProd(const vector3d& a,const vector3d& b);
matrix33 buildRotMatrix33(float winkel1, float winkel2, float winkel3,float x=0,float y=0,float z=0);
matrix33 calcCameraMatrix(vector3d& startpkt,vector3d& endpkt,float roll);
matrix33 transpose(const matrix33& a);
void setIdent(matrix33 &matrix);

//----------------------------------------------------------------

class matrix44 {
public:
	matrix44& operator+=(const matrix44& a);	
	matrix44 operator+(const matrix44& a);
	matrix44& operator-=(const matrix44& a);
	matrix44 operator-(const matrix44& a);
	matrix44& operator*=(const float a);	
	matrix44 operator*(const float a);
	matrix44 operator*=(const matrix44& a);
	matrix44 operator*(const matrix44& a);
	vector4d   operator*=(const vector4d& a);
	vector4d   operator*(const vector4d& a);

	float _11,_12,_13,_14;
    float _21,_22,_23,_24;
    float _31,_32,_33,_34;
	float _41,_42,_43,_44;
};

inline float skalProd(const vector4d& a,const vector4d& b);
inline float vecLen(const vector4d& a);
inline float angle(const vector4d& a,const vector4d& b);
inline vector4d normalize(const vector4d& a);
inline vector4d negatize(const vector4d& a);
inline vector4d crossProd(const vector4d& a,const vector4d& b);
matrix44 transpose(const matrix44& a);
matrix44 buildRotMatrix44(float winkel1, float winkel2, float winkel3,float x=0,float y=0,float z=0);
void setIdent(matrix44 &matrix);





float skalProd(const vector3d& a,const vector3d& b) { 
	return a.x*b.x+a.y*b.y+a.z*b.z;
}
float skalProd(const vector4d& a,const vector4d& b) { 
    return a.x*b.x+a.y*b.y+a.z*b.z;
}
float vecLen(const vector3d& a) { 
	return (float)sqrt(a.x*a.x+a.y*a.y+a.z*a.z);
}
float vecLen(const vector4d& a) {
	return (float)sqrt(a.x*a.x+a.y*a.y+a.z*a.z);
}
float angle(const vector3d& a,const vector3d& b) {
	return (float)acos((a.x*b.x+a.y*b.y+a.z*b.z)/(sqrt(a.x*a.x+a.y*a.y+a.z*a.z)*
                                           sqrt(b.x*b.x+b.y*b.y+b.z*b.z)));
}

vector3d normalize(const vector3d& a) { 
    float b;
	vector3d res;
    b=1/(float)sqrt(a.x*a.x+a.y*a.y+a.z*a.z);
    res.x=a.x*b;
    res.y=a.y*b;
    res.z=a.z*b;
    return res;
}
vector3d negatize(const vector3d& a) {
	vector3d res;
	res.x=-a.x;
	res.y=-a.y;
	res.z=-a.z;
	return res;
}
vector3d crossProd(const vector3d& a,const vector3d& b) { 
    vector3d res;
    res.x=a.y*b.z-a.z*b.y;
    res.y=a.z*b.x-a.x*b.z;
    res.z=a.x*b.y-a.y*b.x;
    return res;
}


vector2d normalize(const vector2d& a) {
	float b;
	vector2d res;
	b=1/(float)sqrt(a.x*a.x+a.y*a.y);
	res.x = a.x*b;
	res.y = a.y*b;
	return res;
}

vector4d normalize(const vector4d& a) { 
    float b;
	vector4d res;
    b=1/(float)sqrt(a.x*a.x+a.y*a.y+a.z*a.z);
    res.x=a.x*b;
    res.y=a.y*b;
    res.z=a.z*b;
	res.w=1;
    return res;
}
vector4d negatize(const vector4d& a) {
	vector4d res;
	res.x=-a.x;
	res.y=-a.y;
	res.z=-a.z;
	return res;
}
vector4d crossProd(const vector4d& a,const vector4d& b) { 
	vector4d res;
    res.x=a.y*b.z-a.z*b.y;
    res.y=a.z*b.x-a.x*b.z;
    res.z=a.x*b.y-a.y*b.x;
	res.w=1;
    return res;
}

}; // LMSPLINE
