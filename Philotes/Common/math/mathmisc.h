#pragma once


#include "mathprerequisites.h"
#include "scalar.h"
//#include "util/array.h"
//#include "util/list.h"
#include <vector>
#include <iostream>
#include <list>
#include <limits>

namespace Math
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

    /** Wrapper class which indicates a given angle value is in Degrees.
    @remarks
        Degree values are interchangeable with Radian values, and conversions
        will be done automatically between them.
    */
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

    /** Wrapper class which identifies a value as the currently default angle 
        type, as defined by Math::setAngleUnit.
    @remarks
        Angle values will be automatically converted between radians and degrees,
        as appropriate.
    */
	class Angle
	{
		scalar mAngle;
	public:
		explicit Angle ( scalar angle ) : mAngle(angle) {}
		operator Radian() const;
		operator Degree() const;
	};

	// these functions could not be defined within the class definition of class
	// Radian because they required class Degree to be defined
	inline Radian::Radian ( const Degree& d ) : mRad(d.valueRadians()) {
	}
	inline Radian& Radian::operator = ( const Degree& d ) {
		mRad = d.valueRadians(); return *this;
	}
	inline Radian Radian::operator + ( const Degree& d ) const {
		return Radian ( mRad + d.valueRadians() );
	}
	inline Radian& Radian::operator += ( const Degree& d ) {
		mRad += d.valueRadians();
		return *this;
	}
	inline Radian Radian::operator - ( const Degree& d ) const {
		return Radian ( mRad - d.valueRadians() );
	}
	inline Radian& Radian::operator -= ( const Degree& d ) {
		mRad -= d.valueRadians();
		return *this;
	}

	class _PhiloCommonExport MathMisc
	{
	public:
       enum AngleUnit
       {
           AU_DEGREE,
           AU_RADIAN
       };

    protected:
       // angle units used by the api
       static AngleUnit msAngleUnit;

        /// Size of the trig tables as determined by constructor.
        static int mTrigTableSize;

        /// Radian -> index factor value ( mTrigTableSize / 2 * PI )
        static scalar mTrigTableFactor;
        static scalar* mSinTable;
        static scalar* mTanTable;

        /** Private function to build trig tables.
        */
        void buildTrigTables();

		static scalar SinTable (scalar fValue);
		static scalar TanTable (scalar fValue);
    public:
        /** Default constructor.
            @param
                trigTableSize Optional parameter to set the size of the
                tables used to implement Sin, Cos, Tan
        */
        MathMisc(unsigned int trigTableSize = 4096);

        /** Default destructor.
        */
        ~MathMisc();

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

        /** Cosine function.
            @param
                fValue Angle in radians
            @param
                useTables If true, uses lookup tables rather than
                calculation - faster but less accurate.
        */
        static inline scalar Cos (const Radian& fValue, bool useTables = false) {
			return (!useTables) ? scalar(cos(fValue.valueRadians())) : SinTable(fValue.valueRadians() + N_PI_HALF);
		}
        /** Cosine function.
            @param
                fValue Angle in radians
            @param
                useTables If true, uses lookup tables rather than
                calculation - faster but less accurate.
        */
        static inline scalar Cos (scalar fValue, bool useTables = false) {
			return (!useTables) ? scalar(cos(fValue)) : SinTable(fValue + N_PI_HALF);
		}

		static inline scalar Exp (scalar fValue) { return scalar(exp(fValue)); }

		static inline scalar Floor (scalar fValue) { return scalar(floor(fValue)); }

		static inline scalar Log (scalar fValue) { return scalar(log(fValue)); }

		/// Stored value of log(2) for frequent use
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

        /** Sine function.
            @param
                fValue Angle in radians
            @param
                useTables If true, uses lookup tables rather than
                calculation - faster but less accurate.
        */
        static inline scalar Sin (const Radian& fValue, bool useTables = false) {
			return (!useTables) ? scalar(sin(fValue.valueRadians())) : SinTable(fValue.valueRadians());
		}
        /** Sine function.
            @param
                fValue Angle in radians
            @param
                useTables If true, uses lookup tables rather than
                calculation - faster but less accurate.
        */
        static inline scalar Sin (scalar fValue, bool useTables = false) {
			return (!useTables) ? scalar(sin(fValue)) : SinTable(fValue);
		}

		static inline scalar Sqr (scalar fValue) { return fValue*fValue; }

		static inline scalar Sqrt (scalar fValue) { return scalar(sqrt(fValue)); }

        static inline Radian Sqrt (const Radian& fValue) { return Radian(sqrt(fValue.valueRadians())); }

        static inline Degree Sqrt (const Degree& fValue) { return Degree(sqrt(fValue.valueDegrees())); }

        /** Inverse square root i.e. 1 / Sqrt(x), good for vector
            normalisation.
        */
		static scalar InvSqrt(scalar fValue);

        static scalar UnitRandom ();  // in [0,1]

        static scalar RangeRandom (scalar fLow, scalar fHigh);  // in [fLow,fHigh]

        static scalar SymmetricRandom ();  // in [-1,1]

        /** Tangent function.
            @param
                fValue Angle in radians
            @param
                useTables If true, uses lookup tables rather than
                calculation - faster but less accurate.
        */
		static inline scalar Tan (const Radian& fValue, bool useTables = false) {
			return (!useTables) ? scalar(tan(fValue.valueRadians())) : TanTable(fValue.valueRadians());
		}
        /** Tangent function.
            @param
                fValue Angle in radians
            @param
                useTables If true, uses lookup tables rather than
                calculation - faster but less accurate.
        */
		static inline scalar Tan (scalar fValue, bool useTables = false) {
			return (!useTables) ? scalar(tan(fValue)) : TanTable(fValue);
		}

		static inline scalar DegreesToRadians(scalar degrees) { return degrees * fDeg2Rad; }
        static inline scalar RadiansToDegrees(scalar radians) { return radians * fRad2Deg; }

       /** These functions used to set the assumed angle units (radians or degrees) 
            expected when using the Angle type.
       @par
            You can set this directly after creating a new Root, and also before/after resource creation,
            depending on whether you want the change to affect resource files.
       */
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

       /** Checks whether a given point is inside a triangle, in a
            2-dimensional (Cartesian) space.
            @remarks
                The vertices of the triangle must be given in either
                trigonometrical (anticlockwise) or inverse trigonometrical
                (clockwise) order.
            @param
                p The point.
            @param
                a The triangle's first vertex.
            @param
                b The triangle's second vertex.
            @param
                c The triangle's third vertex.
            @returns
                If the point resides in the triangle, <b>true</b> is
                returned.
            @par
                If the point is outside the triangle, <b>false</b> is
                returned.
        */
        static bool pointInTri2D(const Vector2& p, const Vector2& a, 
			const Vector2& b, const Vector2& c);

       /** Checks whether a given 3D point is inside a triangle.
       @remarks
            The vertices of the triangle must be given in either
            trigonometrical (anticlockwise) or inverse trigonometrical
            (clockwise) order, and the point must be guaranteed to be in the
			same plane as the triangle
        @param
            p The point.
        @param
            a The triangle's first vertex.
        @param
            b The triangle's second vertex.
        @param
            c The triangle's third vertex.
		@param 
			normal The triangle plane's normal (passed in rather than calculated
				on demand since the caller may already have it)
        @returns
            If the point resides in the triangle, <b>true</b> is
            returned.
        @par
            If the point is outside the triangle, <b>false</b> is
            returned.
        */
        static bool pointInTri3D(const Vector3& p, const Vector3& a, 
			const Vector3& b, const Vector3& c, const Vector3& normal);
        /** Ray / plane intersection, returns boolean result and distance. */
        static std::pair<bool, scalar> intersects(const Ray& ray, const Plane& plane);

        /** Ray / sphere intersection, returns boolean result and distance. */
        static std::pair<bool, scalar> intersects(const Ray& ray, const Sphere& sphere, 
            bool discardInside = true);
        
        /** Ray / box intersection, returns boolean result and distance. */
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

        /** Calculates the tangent space vector for a given set of positions / texture coords. */
        static Vector3 calculateTangentSpaceVector(
            const Vector3& position1, const Vector3& position2, const Vector3& position3,
            scalar u1, scalar v1, scalar u2, scalar v2, scalar u3, scalar v3);

        /** Build a reflection matrix for the passed in plane. */
        static Matrix4 buildReflectionMatrix(const Plane& p);
        /** Calculate a face normal, including the w component which is the offset from the origin. */
        static Vector4 calculateFaceNormal(const Vector3& v1, const Vector3& v2, const Vector3& v3);
        /** Calculate a face normal, no w-information. */
        static Vector3 calculateBasicFaceNormal(const Vector3& v1, const Vector3& v2, const Vector3& v3);
        /** Calculate a face normal without normalize, including the w component which is the offset from the origin. */
        static Vector4 calculateFaceNormalWithoutNormalize(const Vector3& v1, const Vector3& v2, const Vector3& v3);
        /** Calculate a face normal without normalize, no w-information. */
        static Vector3 calculateBasicFaceNormalWithoutNormalize(const Vector3& v1, const Vector3& v2, const Vector3& v3);

		/** Generates a value based on the Gaussian (normal) distribution function
			with the given offset and scale parameters.
		*/
		static scalar gaussianDistribution(scalar x, scalar offset = 0.0f, scalar scale = 1.0f);

		/** Clamp a value within an inclusive range. */
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


		static const scalar POS_INFINITY;
		static const scalar NEG_INFINITY;
		//static const scalar PI;
		static const scalar TWO_PI;
		static const scalar HALF_PI;
		static const scalar fDeg2Rad;
		static const scalar fRad2Deg;

    };

	// these functions must be defined down here, because they rely on the
	// angle unit conversion functions in class Math:

	inline scalar Radian::valueDegrees() const
	{
		return MathMisc::RadiansToDegrees ( mRad );
	}

	inline scalar Radian::valueAngleUnits() const
	{
		return MathMisc::RadiansToAngleUnits ( mRad );
	}

	inline scalar Degree::valueRadians() const
	{
		return MathMisc::DegreesToRadians ( mDeg );
	}

	inline scalar Degree::valueAngleUnits() const
	{
		return MathMisc::DegreesToAngleUnits ( mDeg );
	}

	inline Angle::operator Radian() const
	{
		return Radian(MathMisc::AngleUnitsToRadians(mAngle));
	}

	inline Angle::operator Degree() const
	{
		return Degree(MathMisc::AngleUnitsToDegrees(mAngle));
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