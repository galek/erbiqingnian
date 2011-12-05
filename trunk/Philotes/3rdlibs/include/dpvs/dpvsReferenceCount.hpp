#ifndef __DPVSREFERENCECOUNT_HPP
#define __DPVSREFERENCECOUNT_HPP
/*****************************************************************************
 *
 * dPVS
 * -----------------------------------------
 *
 * (C) 1999-2005 Hybrid Graphics Ltd.
 * (C) 2006-     Umbra Software Ltd.
 * All Rights Reserved.
 *	
 * This file consists of unpublished, proprietary source code of
 * Umbra Software, and is considered Confidential Information for
 * purposes of non-disclosure agreement. Disclosure outside the terms
 * outlined in signed agreement may result in irreparable harm to
 * Umbra Software and legal action against the party in breach.
 *
 * Description:     Reference counting mechanism used by most DPVS public classes
 *
 * $Id: //umbra/products/dpvs/interface/dpvsReferenceCount.hpp#1 $
 * $Date: 2006/11/07 $
 * $Author: otso $
 * 
 ******************************************************************************/

#if !defined (__DPVSDEFS_HPP)
#   include "dpvsDefs.hpp"
#endif

namespace DPVS
{

/******************************************************************************
 *
 * Class:           DPVS::ReferenceCount
 *
 * Description:     Reference counting base class
 *
 * Notes:           Classes in the library that use reference counting are
 *                  all derived from this class. Instances of reference
 *                  counted classes cannot be destructed by calling
 *                  'delete', instead the member function release() should
 *                  be used.
 *
 *                  Note that only DPVS classes can inherit the
 *                  ReferenceCount class.
 *
 *                  Note that the user cannot inherit from DPVS classes.
 *
 * See Also:        DPVS::ReferenceCount::release()
 *
 *****************************************************************************/

class ReferenceCount
{
public:
    DPVSDEC void			addReference        (void);         
    DPVSDEC void			autoRelease         (void);         

    DPVSDEC const char*		getName             (void) const;
    DPVSDEC int				getReferenceCount   (void) const;   
    DPVSDEC void*			getUserPointer      (void) const;
    DPVSDEC bool			release             (void) const;   
    DPVSDEC void			setName             (const char*);
    DPVSDEC void			setUserPointer      (void*);

	static DPVSDEC bool		debugIsValidPointer	(const ReferenceCount*);
protected:
							ReferenceCount      (class ImpReferenceCount*);         
    virtual                 ~ReferenceCount     (void);   
    virtual void            destruct            (void) const;
	UINT32					m_reserved0;					// reserved for internal use
private:
    mutable INT32           m_referenceCount    :31;        // reference count of the object
    mutable UINT32          m_autoReleased      :1;         // has the object been auto-released?
    void*                   m_userPointer;                  // user data pointer
};


} // DPVS

//------------------------------------------------------------------------
#endif //__DPVSREFERENCECOUNT_HPP
