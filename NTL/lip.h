
#ifndef NTL_g_lip__H
#define NTL_g_lip__H

#define __MULTI_THREADED

#ifdef __MULTI_THREADED
#define _STATIC_ST	
#define _CLEAR_Z(x)	_ntl_zfree(&x)
#else
#define _STATIC_ST	static
#define _CLEAR_Z(x)	
#endif


#include <NTL/config.h>
#include <NTL/mach_desc.h>

#ifdef NTL_GMP_LIP

#include <NTL/gmp_aux.h>

#include <NTL/g_lip.h>

#else

#include <NTL/c_lip.h>

#endif

#endif
