///PHILOTES Source Code.  (C)2012 PhiloLabs

// ------- Philotes Debug Macros --------- by Li
// NOTE: unchecked, if __PHILO_NO_ASSERT__ is set
// ph_assert()  - the vanilla ph_assert() Macro
// ph_assert2() - an ph_assert() plus a message from the programmer

#pragma once

#include "core/config.h"

void ph_printf(const char *, ...) __attribute__((format(printf,1,2)));
void ph_error(const char*, ...) __attribute__((format(printf,1,2)));
void ph_dbgout(const char*, ...) __attribute__((format(printf,1,2)));
void ph_warning(const char*, ...) __attribute__((format(printf,1,2)));
void ph_confirm(const char*, ...) __attribute__((format(printf,1,2)));
void ph_sleep(double);
void ph_barf(const char *, const char *, int);
void ph_barf2(const char*, const char*, const char*, int);

#if __PHILO_NO_ASSERT__
#	define ph_assert(exp) if(!(exp)){}
#	define ph_assert2(exp, msg) if(!(exp)){}
#else
#	define ph_assert(exp) { if (!(exp)) ph_barf(#exp,__FILE__,__LINE__); }
#	define ph_assert2(exp, msg) { if (!(exp)) ph_barf2(#exp,msg,__FILE__,__LINE__); }
#endif

