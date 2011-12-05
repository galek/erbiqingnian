#ifndef __DPVS_HPP
#define __DPVS_HPP
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
 * Description:     Public interface main header file that includes
 *                  all other DPVS public header files. Include
 *                  this file in your application if you don't know
 *                  which specific DPVS headers are needed.
 *
 * $Id: //umbra/products/dpvs/interface/dpvs.hpp#1 $
 * $Date: 2006/11/07 $
 * $Author: otso $
 * 
 ******************************************************************************/

#if !defined(__DPVSCAMERA_HPP)
#   include "dpvsCamera.hpp"
#endif
#if !defined(__DPVSCELL_HPP)
#   include "dpvsCell.hpp"
#endif
#if !defined(__DPVSCOMMANDER_HPP)
#   include "dpvsCommander.hpp"
#endif
#if !defined (__DPVSLIBRARY_HPP)       
#   include "dpvsLibrary.hpp"
#endif
#if !defined(__DPVSMODEL_HPP)
#   include "dpvsModel.hpp"
#endif
#if !defined(__DPVSOBJECT_HPP)
#   include "dpvsObject.hpp"
#endif
#if !defined(__DPVSUTILITY_HPP)
#   include "dpvsUtility.hpp"
#endif

//------------------------------------------------------------------------
#endif // __DPVS_HPP
