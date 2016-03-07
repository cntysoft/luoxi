#include <QDir>

#include "lxlib/global/const.h"
#include "kernel/settings.h"

namespace luoxi{

using sn::corelib::Settings;

static void init_global_cfg(Settings &settings);

void init_defualt_cfg(Settings& settings)
{
   init_global_cfg(settings);
}

static void init_global_cfg(Settings& settings)
{
   QString runtimeDir = QDir::tempPath()+QDir::separator()+"upgrademgrslave";
   settings.setValue("runtimeDir", runtimeDir, LUOXI_CFG_GROUP_GLOBAL);
   settings.setValue("baseDataDir", "/cntysoft/upgrademgrslave", LUOXI_CFG_GROUP_GLOBAL);
}


}//luoxi