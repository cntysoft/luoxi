#ifndef LUOXI_UTILS_COMMON_FUNCS_H
#define LUOXI_UTILS_COMMON_FUNCS_H

#include <QLatin1String>

#include "application.h"

namespace luoxi{

using LuoxiApplication = luoxi::Application;

LuoxiApplication* get_app_ref();

QLatin1String get_luoxi_version();

QString get_application_filepath();

}//luoxi

#endif // LUOXI_UTILS_COMMON_FUNCS_H
