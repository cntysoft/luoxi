#ifndef LUOXI_SERVICE_GLOBAL_DEFS_H
#define LUOXI_SERVICE_GLOBAL_DEFS_H

#include <qglobal.h>

#ifdef LUOXI_SERVICE_STATIC_LIB
   #define LUOXI_SERVICE_EXPORT 
#else
   #ifdef LUOXI_SERVICE_LIBRARY
      #define LUOXI_SERVICE_EXPORT Q_DECL_EXPORT
   #else
      #define LUOXI_SERVICE_EXPORT Q_DECL_IMPORT
   #endif
#endif

#include "macros.h"

#endif // LUOXI_SERVICE_GLOBAL_DEFS_H
