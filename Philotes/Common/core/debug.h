///PHILOTES Source Code.  (C)2012 PhiloLabs

// ------- Philotes Debug Macros --------- by Li
// NOTE: unchecked, if __PHILO_NO_ASSERT__ is set
// ph_assert()  - the vanilla ph_assert() Macro
// ph_assert2() - an ph_assert() plus a message from the programmer

#pragma once

#include "core/config.h"

void n_printf(const char *, ...) __attribute__((format(printf,1,2)));
void n_error(const char*, ...) __attribute__((format(printf,1,2)));
void n_dbgout(const char*, ...) __attribute__((format(printf,1,2)));
void n_warning(const char*, ...) __attribute__((format(printf,1,2)));
void n_confirm(const char*, ...) __attribute__((format(printf,1,2)));
void n_sleep(double);
void n_barf(const char *, const char *, int);
void n_barf2(const char*, const char*, const char*, int);

#if __PHILO_NO_ASSERT__
#	define ph_assert(exp) if(!(exp)){}
#	define ph_assert2(exp, msg) if(!(exp)){}
#else
#	define ph_assert(exp) { if (!(exp)) n_barf(#exp,__FILE__,__LINE__); }
#	define ph_assert2(exp, msg) { if (!(exp)) n_barf2(#exp,msg,__FILE__,__LINE__); }
#endif

