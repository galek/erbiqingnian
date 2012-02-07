

#include "colourValue.h"
#include "math/mathMisc.h"
#include "math/scalar.h"

namespace Util {

    const ColourValue ColourValue::ZERO = ColourValue(0.0,0.0,0.0,0.0);
    const ColourValue ColourValue::Black = ColourValue(0.0,0.0,0.0);
    const ColourValue ColourValue::White = ColourValue(1.0,1.0,1.0);
    const ColourValue ColourValue::Red = ColourValue(1.0,0.0,0.0);
    const ColourValue ColourValue::Green = ColourValue(0.0,1.0,0.0);
    const ColourValue ColourValue::Blue = ColourValue(0.0,0.0,1.0);

    //---------------------------------------------------------------------
    RGBA ColourValue::getAsRGBA(void) const
    {
        uint8 val8;
        uint32 val32 = 0;

        // Convert to 32bit pattern
        // (RGBA = 8888)

        // Red
        val8 = static_cast<uint8>(r * 255);
        val32 = val8 << 24;

        // Green
        val8 = static_cast<uint8>(g * 255);
        val32 += val8 << 16;

        // Blue
        val8 = static_cast<uint8>(b * 255);
        val32 += val8 << 8;

        // Alpha
        val8 = static_cast<uint8>(a * 255);
        val32 += val8;

        return val32;
    }
    //---------------------------------------------------------------------
    ARGB ColourValue::getAsARGB(void) const
    {
        uint8 val8;
        uint32 val32 = 0;

        // Convert to 32bit pattern
        // (ARGB = 8888)

        // Alpha
        val8 = static_cast<uint8>(a * 255);
        val32 = val8 << 24;

        // Red
        val8 = static_cast<uint8>(r * 255);
        val32 += val8 << 16;

        // Green
        val8 = static_cast<uint8>(g * 255);
        val32 += val8 << 8;

        // Blue
        val8 = static_cast<uint8>(b * 255);
        val32 += val8;


        return val32;
    }
	//---------------------------------------------------------------------

	BGRA ColourValue::getAsBGRA(void) const
	{
		uint8 val8;
		uint32 val32 = 0;

		// Convert to 32bit pattern
		// (ARGB = 8888)

		// Blue
		val8 = static_cast<uint8>(b * 255);
		val32 = val8 << 24;

		// Green
		val8 = static_cast<uint8>(g * 255);
		val32 += val8 << 16;

		// Red
		val8 = static_cast<uint8>(r * 255);
		val32 += val8 << 8;

		// Alpha
		val8 = static_cast<uint8>(a * 255);
		val32 += val8;


		return val32;
	}
    //---------------------------------------------------------------------
    ABGR ColourValue::getAsABGR(void) const
    {
        uint8 val8;
        uint32 val32 = 0;

        // Convert to 32bit pattern
        // (ABRG = 8888)

        // Alpha
        val8 = static_cast<uint8>(a * 255);
        val32 = val8 << 24;

        // Blue
        val8 = static_cast<uint8>(b * 255);
        val32 += val8 << 16;

        // Green
        val8 = static_cast<uint8>(g * 255);
        val32 += val8 << 8;

        // Red
        val8 = static_cast<uint8>(r * 255);
        val32 += val8;


        return val32;
    }
    //---------------------------------------------------------------------
    void ColourValue::setAsRGBA(const RGBA val)
    {
        uint32 val32 = val;

        // Convert from 32bit pattern
        // (RGBA = 8888)

        // Red
        r = ((val32 >> 24) & 0xFF) / 255.0f;

        // Green
        g = ((val32 >> 16) & 0xFF) / 255.0f;

        // Blue
        b = ((val32 >> 8) & 0xFF) / 255.0f;

        // Alpha
        a = (val32 & 0xFF) / 255.0f;
    }
    //---------------------------------------------------------------------
    void ColourValue::setAsARGB(const ARGB val)

    {
        uint32 val32 = val;

        // Convert from 32bit pattern
        // (ARGB = 8888)

        // Alpha
        a = ((val32 >> 24) & 0xFF) / 255.0f;

        // Red
        r = ((val32 >> 16) & 0xFF) / 255.0f;

        // Green
        g = ((val32 >> 8) & 0xFF) / 255.0f;

        // Blue
        b = (val32 & 0xFF) / 255.0f;
    }
	//---------------------------------------------------------------------
	void ColourValue::setAsBGRA(const BGRA val)
	{
		uint32 val32 = val;

		// Convert from 32bit pattern
		// (ARGB = 8888)

		// Blue
		b = ((val32 >> 24) & 0xFF) / 255.0f;

		// Green
		g = ((val32 >> 16) & 0xFF) / 255.0f;

		// Red
		r = ((val32 >> 8) & 0xFF) / 255.0f;

		// Alpha
		a = (val32 & 0xFF) / 255.0f;
	}
    //---------------------------------------------------------------------
    void ColourValue::setAsABGR(const ABGR val)
    {
        uint32 val32 = val;

        // Convert from 32bit pattern
        // (ABGR = 8888)

        // Alpha
        a = ((val32 >> 24) & 0xFF) / 255.0f;

        // Blue
        b = ((val32 >> 16) & 0xFF) / 255.0f;

        // Green
        g = ((val32 >> 8) & 0xFF) / 255.0f;

        // Red
        r = (val32 & 0xFF) / 255.0f;
    }
    //---------------------------------------------------------------------
    bool ColourValue::operator==(const ColourValue& rhs) const
    {
        return (r == rhs.r &&
            g == rhs.g &&
            b == rhs.b &&
            a == rhs.a);
    }
    //---------------------------------------------------------------------
    bool ColourValue::operator!=(const ColourValue& rhs) const
    {
        return !(*this == rhs);
    }
	//---------------------------------------------------------------------
	void ColourValue::setHSB(float hue, float saturation, float brightness)
	{
		using namespace Math;
		// wrap hue
		if (hue > 1.0f)
		{
			hue -= (int)hue;
		}
		else if (hue < 0.0f)
		{
			hue += (int)hue + 1;
		}
		// clamp saturation / brightness
		saturation = MathMisc::Min(saturation, (float)1.0);
		saturation = MathMisc::Max(saturation, (float)0.0);
		brightness = MathMisc::Min(brightness, (float)1.0);
		brightness = MathMisc::Max(brightness, (float)0.0);

		if (brightness == 0.0f)
		{   
			// early exit, this has to be black
			r = g = b = 0.0f;
			return;
		}

		if (saturation == 0.0f)
		{   
			// early exit, this has to be grey

			r = g = b = brightness;
			return;
		}


		float hueDomain  = hue * 6.0f;
		if (hueDomain >= 6.0f)
		{
			// wrap around, and allow mathematical errors
			hueDomain = 0.0f;
		}
		unsigned short domain = (unsigned short)hueDomain;
		float f1 = brightness * (1 - saturation);
		float f2 = brightness * (1 - saturation * (hueDomain - domain));
		float f3 = brightness * (1 - saturation * (1 - (hueDomain - domain)));

		switch (domain)
		{
		case 0:
			// red domain; green ascends
			r = brightness;
			g = f3;
			b = f1;
			break;
		case 1:
			// yellow domain; red descends
			r = f2;
			g = brightness;
			b = f1;
			break;
		case 2:
			// green domain; blue ascends
			r = f1;
			g = brightness;
			b = f3;
			break;
		case 3:
			// cyan domain; green descends
			r = f1;
			g = f2;
			b = brightness;
			break;
		case 4:
			// blue domain; red ascends
			r = f3;
			g = f1;
			b = brightness;
			break;
		case 5:
			// magenta domain; blue descends
			r = brightness;
			g = f1;
			b = f2;
			break;
		}


	}
	//---------------------------------------------------------------------
	void ColourValue::getHSB(float* hue, float* saturation, float* brightness) const
	{
		using namespace Math;

		float vMin = MathMisc::Min(r, MathMisc::Min(g, b));
		float vMax = MathMisc::Max(r, MathMisc::Max(g, b));
		float delta = vMax - vMin;

		*brightness = vMax;

		if (MathMisc::RealEqual(delta, 0.0f, (scalar)1e-6))
		{
			// grey
			*hue = 0;
			*saturation = 0;
		}
		else                                    
		{
			// a colour
			*saturation = delta / vMax;

			float deltaR = (((vMax - r) / 6.0f) + (delta / 2.0f)) / delta;
			float deltaG = (((vMax - g) / 6.0f) + (delta / 2.0f)) / delta;
			float deltaB = (((vMax - b) / 6.0f) + (delta / 2.0f)) / delta;

			if (MathMisc::RealEqual(r, vMax))
				*hue = deltaB - deltaG;
			else if (MathMisc::RealEqual(g, vMax))
				*hue = 0.3333333f + deltaR - deltaB;
			else if (MathMisc::RealEqual(b, vMax)) 
				*hue = 0.6666667f + deltaG - deltaR;

			if (*hue < 0.0f) 
				*hue += 1.0f;
			if (*hue > 1.0f)
				*hue -= 1.0f;
		}

		
	}

}

