#pragma once


#include "mathprerequisites.h"
#include "scalar.h"
#include <vector>
#include <iostream>
#include <list>
#include <limits>

namespace Philo
{
	class Radian
	{
		scalar mRad;

	public:
		explicit Radian ( scalar r=0 ) : mRad(r) {}
		Radian ( const Degree& d );
		Radian& operator = ( const scalar& f ) { mRad = f; return *this; }
		Radian& operator = ( const Radian& r ) { mRad = r.mRad; return *this; }
		Radian& operator = ( const Degree& d );

		scalar valueDegrees() const; // see bottom of this file
		scalar valueRadians() const { return mRad; }
		scalar valueAngleUnits() const;

		const Radian& operator + () const { return *this; }
		Radian operator + ( const Radian& r ) const { return Radian ( mRad + r.mRad ); }
		Radian operator + ( const Degree& d ) const;
		Radian& operator += ( const Radian& r ) { mRad += r.mRad; return *this; }
		Radian& operator += ( const Degree& d );
		Radian operator - () const { return Radian(-mRad); }
		Radian operator - ( const Radian& r ) const { return Radian ( mRad - r.mRad ); }
		Radian operator - ( const Degree& d ) const;
		Radian& operator -= ( const Radian& r ) { mRad -= r.mRad; return *this; }
		Radian& operator -= ( const Degree& d );
		Radian operator * ( scalar f ) const { return Radian ( mRad * f ); }
		Radian operator * ( const Radian& f ) const { return Radian ( mRad * f.mRad ); }
		Radian& operator *= ( scalar f ) { mRad *= f; return *this; }
		Radian operator / ( scalar f ) const { return Radian ( mRad / f ); }
		Radian& operator /= ( scalar f ) { mRad /= f; return *this; }

		bool operator <  ( const Radian& r ) const { return mRad <  r.mRad; }
		bool operator <= ( const Radian& r ) const { return mRad <= r.mRad; }
		bool operator == ( const Radian& r ) const { return mRad == r.mRad; }
		bool operator != ( const Radian& r ) const { return mRad != r.mRad; }
		bool operator >= ( const Radian& r ) const { return mRad >= r.mRad; }
		bool operator >  ( const Radian& r ) const { return mRad >  r.mRad; }

		inline _PhiloCommonExport friend std::ostream& operator <<
			( std::ostream& o, const Radian& v )
		{
			o << "Radian(" << v.valueRadians() << ")";
			return o;
		}
	};

	class Degree
	{
		scalar mDeg; // if you get an error here - make sure to define/typedef 'scalar' first

	public:
		explicit Degree ( scalar d=0 ) : mDeg(d) {}
		Degree ( const Radian& r ) : mDeg(r.valueDegrees()) {}
		Degree& operator = ( const scalar& f ) { mDeg = f; return *this; }
		Degree& operator = ( const Degree& d ) { mDeg = d.mDeg; return *this; }
		Degree& operator = ( const Radian& r ) { mDeg = r.valueDegrees(); return *this; }

		scalar valueDegrees() const { return mDeg; }
		scalar valueRadians() const; // see bottom of this file
		scalar valueAngleUnits() const;

		const Degree& operator + () const { return *this; }
		Degree operator + ( const Degree& d ) const { return Degree ( mDeg + d.mDeg ); }
		Degree operator + ( const Radian& r ) const { return Degree ( mDeg + r.valueDegrees() ); }
		Degree& operator += ( const Degree& d ) { mDeg += d.mDeg; return *this; }
		Degree& operator += ( const Radian& r ) { mDeg += r.valueDegrees(); return *this; }
		Degree operator - () const { return Degree(-mDeg); }
		Degree operator - ( const Degree& d ) const { return Degree ( mDeg - d.mDeg ); }
		Degree operator - ( const Radian& r ) const { return Degree ( mDeg - r.valueDegrees() ); }
		Degree& operator -= ( const Degree& d ) { mDeg -= d.mDeg; return *this; }
		Degree& operator -= ( const Radian& r ) { mDeg -= r.valueDegrees(); return *this; }
		Degree operator * ( scalar f ) const { return Degree ( mDeg * f ); }
		Degree operator * ( const Degree& f ) const { return Degree ( mDeg * f.mDeg ); }
		Degree& operator *= ( scalar f ) { mDeg *= f; return *this; }
		Degree operator / ( scalar f ) const { return Degree ( mDeg / f ); }
		Degree& operator /= ( scalar f ) { mDeg /= f; return *this; }

		bool operator <  ( const Degree& d ) const { return mDeg <  d.mDeg; }
		bool operator <= ( const Degree& d ) const { return mDeg <= d.mDeg; }
		bool operator == ( const Degree& d ) const { return mDeg == d.mDeg; }
		bool operator != ( const Degree& d ) const { return mDeg != d.mDeg; }
		bool operator >= ( const Degree& d ) const { return mDeg >= d.mDeg; }
		bool operator >  ( const Degree& d ) const { return mDeg >  d.mDeg; }

		inline _PhiloCommonExport friend std::ostream& operator <<
			( std::ostream& o, const Degree& v )
		{
			o << "Degree(" << v.valueDegrees() << ")";
			return o;
		}
	};

	class Angle
	{
		scalar mAngle;
	public:
		explicit Angle ( scalar angle ) : mAngle(angle) {}
		operator Radian() const;
		operator Degree() const;
	};

	inline Radian::Radian ( const Degree& d ) : mRad(d.valueRadians()) 
	{
	}
	inline Radian& Radian::operator = ( const Degree& d ) 
	{
		mRad = d.valueRadians(); return *this;
	}
	inline Radian Radian::operator + ( const Degree& d ) const 
	{
		return Radian ( mRad + d.valueRadians() );
	}
	inline Radian& Radian::operator += ( const Degree& d )
	{
		mRad += d.valueRadians();
		return *this;
	}
	inline Radian Radian::operator - ( const Degree& d ) const
	{
		return Radian ( mRad - d.valueRadians() );
	}
	inline Radian& Radian::operator -= ( const Degree& d )
	{
		mRad -= d.valueRadians();
		return *this;
	}

	//////////////////////////////////////////////////////////////////////////

	template< typename T > struct TRect
	{
		T left, top, right, bottom;
		TRect() : left(0), top(0), right(0), bottom(0) {}
		TRect( T const & l, T const & t, T const & r, T const & b )
			: left( l ), top( t ), right( r ), bottom( b )
		{
		}
		TRect( TRect const & o )
			: left( o.left ), top( o.top ), right( o.right ), bottom( o.bottom )
		{
		}
		TRect & operator=( TRect const & o )
		{
			left = o.left;
			top = o.top;
			right = o.right;
			bottom = o.bottom;
			return *this;
		}
		T width() const
		{
			return right - left;
		}
		T height() const
		{
			return bottom - top;
		}
		bool isNull() const
		{
			return width() == 0 || height() == 0;
		}
		void setNull()
		{
			left = right = top = bottom = 0;
		}
		TRect & merge(const TRect& rhs)
		{
			if (isNull())
			{
				*this = rhs;
			}
			else if (!rhs.isNull())
			{
				left = std::min(left, rhs.left);
				right = std::max(right, rhs.right);
				top = std::min(top, rhs.top);
				bottom = std::max(bottom, rhs.bottom);
			}

			return *this;

		}
		TRect intersect(const TRect& rhs) const
		{
			TRect ret;
			if (isNull() || rhs.isNull())
			{
				return ret;
			}
			else
			{
				ret.left = std::max(left, rhs.left);
				ret.right = std::min(right, rhs.right);
				ret.top = std::max(top, rhs.top);
				ret.bottom = std::min(bottom, rhs.bottom);
			}

			if (ret.left > ret.right || ret.top > ret.bottom)
			{
				ret.left = ret.top = ret.right = ret.bottom = 0;
			}

			return ret;

		}

	};
	template<typename T>
	std::ostream& operator<<(std::ostream& o, const TRect<T>& r)
	{
		o << "TRect<>(l:" << r.left << ", t:" << r.top << ", r:" << r.right << ", b:" << r.bottom << ")";
		return o;
	}

	typedef TRect<float> FloatRect;

	typedef TRect<scalar> RealRect;

	typedef TRect< long > Rect;

	//////////////////////////////////////////////////////////////////////////

	struct Box
	{
		size_t left, top, right, bottom, front, back;

		Box()
			: left(0), top(0), right(1), bottom(1), front(0), back(1)
		{
		}
	
		Box( size_t l, size_t t, size_t r, size_t b ):
		left(l),
			top(t),   
			right(r),
			bottom(b),
			front(0),
			back(1)
		{
			ph_assert(right >= left && bottom >= top && back >= front);
		}
		
		Box( size_t l, size_t t, size_t ff, size_t r, size_t b, size_t bb ):
		left(l),
			top(t),   
			right(r),
			bottom(b),
			front(ff),
			back(bb)
		{
			ph_assert(right >= left && bottom >= top && back >= front);
		}

		bool contains(const Box &def) const
		{
			return (def.left >= left && def.top >= top && def.front >= front &&
				def.right <= right && def.bottom <= bottom && def.back <= back);
		}

		size_t getWidth() const { return right-left; }
		size_t getHeight() const { return bottom-top; }
		size_t getDepth() const { return back-front; }
	};

	//////////////////////////////////////////////////////////////////////////

	class _PhiloCommonExport Math
	{
	public:
		enum AngleUnit
		{
			AU_DEGREE,
			AU_RADIAN
		};

	protected:

		static AngleUnit msAngleUnit;

		static int mTrigTableSize;

		static scalar mTrigTableFactor;
		static scalar* mSinTable;
		static scalar* mTanTable;

		void buildTrigTables();

		static scalar SinTable (scalar fValue);
		static scalar TanTable (scalar fValue);
	public:

		Math(uint trigTableSize = 4096);

		~Math();

		static inline int IAbs (int iValue) { return ( iValue >= 0 ? iValue : -iValue ); }
		static inline int ICeil (float fValue) { return int(ceil(fValue)); }
		static inline int IFloor (float fValue) { return int(floor(fValue)); }
		static int ISign (int iValue);

		static inline scalar Abs (scalar fValue) { return scalar(fabs(fValue)); }
		static inline Degree Abs (const Degree& dValue) { return Degree(fabs(dValue.valueDegrees())); }
		static inline Radian Abs (const Radian& rValue) { return Radian(fabs(rValue.valueRadians())); }
		static Radian ACos (scalar fValue);
		static Radian ASin (scalar fValue);
		static inline Radian ATan (scalar fValue) { return Radian(atan(fValue)); }
		static inline Radian ATan2 (scalar fY, scalar fX) { return Radian(atan2(fY,fX)); }
		static inline scalar Ceil (scalar fValue) { return scalar(ceil(fValue)); }
		static inline bool isNaN(scalar f)
		{
			// std::isnan() is C99, not supported by all compilers
			// However NaN always fails this next test, no other number does.
			return f != f;
		}
		static inline bool isFinite(scalar f)
		{
			return (0 == ((_FPCLASS_SNAN | _FPCLASS_QNAN | _FPCLASS_NINF | _FPCLASS_PINF) & _fpclass(f) ));
		}

		static inline scalar Max(scalar a, scalar b)
		{
			return (a > b) ? a : b;
		}

		static inline double Max(double a, double b)
		{
			return (a > b) ? a : b;
		}

		static inline int Max(int a, int b)
		{
			return (a > b) ? a : b;
		}

		static inline scalar Min(scalar a, scalar b)
		{
			return (a < b) ? a : b;
		}

		static inline double Min(double a, double b)
		{
			return (a < b) ? a : b;
		}

		static inline int Min(int a, int b)
		{
			return (a < b) ? a : b;
		}

		static inline int Min(uint32 a, uint32 b)
		{
			return (a < b) ? a : b;
		}

		static inline scalar Cos (const Radian& fValue, bool useTables = false) 
		{
			return (!useTables) ? scalar(cos(fValue.valueRadians())) : SinTable(fValue.valueRadians() + HALF_PI);
		}

		static inline scalar Cos (scalar fValue, bool useTables = false) 
		{
			return (!useTables) ? scalar(cos(fValue)) : SinTable(fValue + HALF_PI);
		}

		static inline scalar Exp (scalar fValue) { return scalar(exp(fValue)); }

		static inline scalar Floor (scalar fValue) { return scalar(floor(fValue)); }

		static inline scalar Log (scalar fValue) { return scalar(log(fValue)); }

		static const scalar LOG2;

		static inline scalar Log2 (scalar fValue) { return scalar(log(fValue)/LOG2); }

		static inline scalar LogN (scalar base, scalar fValue) { return scalar(log(fValue)/log(base)); }

		static inline scalar Pow (scalar fBase, scalar fExponent) { return scalar(pow(fBase,fExponent)); }

		static scalar Sign (scalar fValue);
		static inline Radian Sign ( const Radian& rValue )
		{
			return Radian(Sign(rValue.valueRadians()));
		}
		static inline Degree Sign ( const Degree& dValue )
		{
			return Degree(Sign(dValue.valueDegrees()));
		}

		static inline scalar Sin (const Radian& fValue, bool useTables = false) {
			return (!useTables) ? scalar(sin(fValue.valueRadians())) : SinTable(fValue.valueRadians());
		}

		static inline scalar Sin (scalar fValue, bool useTables = false) {
			return (!useTables) ? scalar(sin(fValue)) : SinTable(fValue);
		}

		static inline scalar Sqr (scalar fValue) { return fValue*fValue; }

		static inline scalar Sqrt (scalar fValue) { return scalar(sqrt(fValue)); }

		static inline Radian Sqrt (const Radian& fValue) { return Radian(sqrt(fValue.valueRadians())); }

		static inline Degree Sqrt (const Degree& fValue) { return Degree(sqrt(fValue.valueDegrees())); }

		static scalar InvSqrt(scalar fValue);

		static scalar UnitRandom ();  // in [0,1]

		static scalar RangeRandom (scalar fLow, scalar fHigh);  // in [fLow,fHigh]

		static scalar SymmetricRandom ();  // in [-1,1]

		static inline scalar Tan (const Radian& fValue, bool useTables = false) {
			return (!useTables) ? scalar(tan(fValue.valueRadians())) : TanTable(fValue.valueRadians());
		}

		static inline scalar Tan (scalar fValue, bool useTables = false) {
			return (!useTables) ? scalar(tan(fValue)) : TanTable(fValue);
		}

		static inline scalar DegreesToRadians(scalar degrees) { return degrees * fDeg2Rad; }
		static inline scalar RadiansToDegrees(scalar radians) { return radians * fRad2Deg; }

		static void setAngleUnit(AngleUnit unit);
		/** Get the unit being used for angles. */
		static AngleUnit getAngleUnit(void);

		/** Convert from the current AngleUnit to radians. */
		static scalar AngleUnitsToRadians(scalar units);
		/** Convert from radians to the current AngleUnit . */
		static scalar RadiansToAngleUnits(scalar radians);
		/** Convert from the current AngleUnit to degrees. */
		static scalar AngleUnitsToDegrees(scalar units);
		/** Convert from degrees to the current AngleUnit. */
		static scalar DegreesToAngleUnits(scalar degrees);


		static bool pointInTri2D(const Vector2& p, const Vector2& a, 
			const Vector2& b, const Vector2& c);


		static bool pointInTri3D(const Vector3& p, const Vector3& a, 
			const Vector3& b, const Vector3& c, const Vector3& normal);
		static std::pair<bool, scalar> intersects(const Ray& ray, const Plane& plane);

		static std::pair<bool, scalar> intersects(const Ray& ray, const Sphere& sphere, 
			bool discardInside = true);

		static std::pair<bool, scalar> intersects(const Ray& ray, const AxisAlignedBox& box);


		static bool intersects(const Ray& ray, const AxisAlignedBox& box,
			scalar* d1, scalar* d2);


		static std::pair<bool, scalar> intersects(const Ray& ray, const Vector3& a,
			const Vector3& b, const Vector3& c, const Vector3& normal,
			bool positiveSide = true, bool negativeSide = true);

		static std::pair<bool, scalar> intersects(const Ray& ray, const Vector3& a,
			const Vector3& b, const Vector3& c,
			bool positiveSide = true, bool negativeSide = true);

		static bool intersects(const Sphere& sphere, const AxisAlignedBox& box);

		static bool intersects(const Plane& plane, const AxisAlignedBox& box);

		static std::pair<bool, scalar> intersects(
			const Ray& ray, const std::vector<Plane>& planeList, 
			bool normalIsOutside);

		static std::pair<bool, scalar> intersects(
			const Ray& ray, const std::list<Plane>& planeList, 
			bool normalIsOutside);

		static bool intersects(const Sphere& sphere, const Plane& plane);


		static bool RealEqual(scalar a, scalar b,
			scalar tolerance = std::numeric_limits<scalar>::epsilon());

		static Vector3 calculateTangentSpaceVector(
			const Vector3& position1, const Vector3& position2, const Vector3& position3,
			scalar u1, scalar v1, scalar u2, scalar v2, scalar u3, scalar v3);

		static Matrix4 buildReflectionMatrix(const Plane& p);

		static Vector4 calculateFaceNormal(const Vector3& v1, const Vector3& v2, const Vector3& v3);

		static Vector3 calculateBasicFaceNormal(const Vector3& v1, const Vector3& v2, const Vector3& v3);

		static Vector4 calculateFaceNormalWithoutNormalize(const Vector3& v1, const Vector3& v2, const Vector3& v3);

		static Vector3 calculateBasicFaceNormalWithoutNormalize(const Vector3& v1, const Vector3& v2, const Vector3& v3);

		static scalar gaussianDistribution(scalar x, scalar offset = 0.0f, scalar scale = 1.0f);

		template <typename T>
		static T Clamp(T val, T minval, T maxval)
		{
			ph_assert (minval < maxval && "Invalid clamp range");
			return std::max<T>(std::min<T>(val, maxval), minval);
		}

		static Matrix4 makeViewMatrix(const Vector3& position, const Quaternion& orientation, 
			const Matrix4* reflectMatrix = 0);

		/** Get a bounding radius value from a bounding box. */
		static scalar boundingRadiusFromAABB(const AxisAlignedBox& aabb);

		static void makeProjectionMatrix(const Radian& fovy, scalar aspectRatio, 
			scalar nearPlane, scalar farPlane, Matrix4& outMatrix);

		static void makeProjectionMatrix(scalar left, scalar right, scalar bottom, 
			scalar top, scalar near, scalar far, Matrix4& outMatrix);

		static void	buildProjectMatrix(float *dst, const Matrix4 &proj, const Matrix4 &view);

		static void	buildUnprojectMatrix(float *dst, const Matrix4 &proj, const Matrix4 &view);


		static const scalar POS_INFINITY;
		static const scalar NEG_INFINITY;
		static const scalar PI;
		static const scalar TWO_PI;
		static const scalar HALF_PI;
		static const scalar fDeg2Rad;
		static const scalar fRad2Deg;
		static const scalar INFINITE_FAR_PLANE_ADJUST;
	};


	inline scalar Radian::valueDegrees() const
	{
		return Math::RadiansToDegrees ( mRad );
	}

	inline scalar Radian::valueAngleUnits() const
	{
		return Math::RadiansToAngleUnits ( mRad );
	}

	inline scalar Degree::valueRadians() const
	{
		return Math::DegreesToRadians ( mDeg );
	}

	inline scalar Degree::valueAngleUnits() const
	{
		return Math::DegreesToAngleUnits ( mDeg );
	}

	inline Angle::operator Radian() const
	{
		return Radian(Math::AngleUnitsToRadians(mAngle));
	}

	inline Angle::operator Degree() const
	{
		return Degree(Math::AngleUnitsToDegrees(mAngle));
	}

	inline Radian operator * ( scalar a, const Radian& b )
	{
		return Radian ( a * b.valueRadians() );
	}

	inline Radian operator / ( scalar a, const Radian& b )
	{
		return Radian ( a / b.valueRadians() );
	}

	inline Degree operator * ( scalar a, const Degree& b )
	{
		return Degree ( a * b.valueDegrees() );
	}

	inline Degree operator / ( scalar a, const Degree& b )
	{
		return Degree ( a / b.valueDegrees() );
	}

}