#ifndef LUOXI_LIB_GLOBAL_GLOBAL_H
#define LUOXI_LIB_GLOBAL_GLOBAL_H

#include "corelib/global/global.h"

#ifdef LUOXI_STATIC_LIB
   #define LUOXI_LIB_EXPORT 
#else
   #ifdef LUOXI_LIBRARY
      #define LUOXI_LIB_EXPORT Q_DECL_EXPORT
   #else
      #define LUOXI_LIB_EXPORT Q_DECL_IMPORT
   #endif
#endif

#endif // LUOXI_LIB_GLOBAL_GLOBAL_H
