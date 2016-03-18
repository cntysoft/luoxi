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
   QString runtimeDir = QDir::tempPath() + QDir::separator() + "luoxi";
   settings.setValue("runtimeDir", runtimeDir, LUOXI_CFG_GROUP_GLOBAL);
   settings.setValue("baseDataDir", "/cntysoft/luoxi", LUOXI_CFG_GROUP_GLOBAL);
   settings.setValue("deployBaseDir", "/srv/www", LUOXI_CFG_GROUP_GLOBAL);
   settings.setValue("nginxConfDir", "/etc/nginx/vhost", LUOXI_CFG_GROUP_GLOBAL);
   settings.setValue("webDeployUserId", 30, LUOXI_CFG_GROUP_GLOBAL);
   settings.setValue("webDeployGroupId", 8, LUOXI_CFG_GROUP_GLOBAL);
   settings.setValue("dbHost", "localhost", LUOXI_CFG_GROUP_GLOBAL);
   settings.setValue("dbUser", "root", LUOXI_CFG_GROUP_GLOBAL);
   settings.setValue("dbPassword", "cntysoft", LUOXI_CFG_GROUP_GLOBAL);
   settings.setValue("metaserverHost", "127.0.0.1", LUOXI_CFG_GROUP_GLOBAL);
   settings.setValue("metaserverPort", MS_LISTEN_PORT, LUOXI_CFG_GROUP_GLOBAL);
   settings.setValue("aliAccessKeyId", "twJjKngFf6uySA0P", LUOXI_CFG_GROUP_GLOBAL);
   settings.setValue("aliAccessKeySecret", "3edbGFcrayDU4kaDrS004sp5J5Auod", LUOXI_CFG_GROUP_GLOBAL);
   settings.setValue("deployServerAddress", "127.0.0.1", LUOXI_CFG_GROUP_GLOBAL);
}


}//luoxi