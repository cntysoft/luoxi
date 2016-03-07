#include "common_funcs.h"

namespace luoxi{

LuoxiApplication* get_app_ref()
{
   return static_cast<LuoxiApplication*>(LuoxiApplication::instance());
}

QLatin1String get_luoxi_version()
{
   return QLatin1String(LUOXI_VERSION);
}

QString get_application_filepath()
{
#ifdef LUOXI_DEBUG_BUILD
   return Application::applicationFilePath();
#else
   return QString("/usr/local/bin/luoxi");
#endif
}

}//luoxi