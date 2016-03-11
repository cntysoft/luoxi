#include "stddir.h"

#include "lxlib/global/const.h"
#include "corelib/kernel/settings.h"
#include "corelib/kernel/application.h"

namespace lxlib{
namespace kernel{

using sn::corelib::Settings;
using sn::corelib::Application;

QString StdDir::getBaseDataDir()
{
   static QString dir;
   if(dir.isEmpty()){
#ifdef AUTOTEST_BUILD
      dir = "/cntysoft/luoxi";
#else
      Settings& settings = Application::instance()->getSettings();
      dir = settings.getValue("baseDataDir", LUOXI_CFG_GROUP_GLOBAL, "/cntysoft/luoxi").toString();
#endif
   }
   return dir;
}

QString StdDir::getSoftwareRepoDir()
{
   return StdDir::getBaseDataDir()+"/softwarerepo";
}

QString StdDir::getMetaDir()
{
   return StdDir::getBaseDataDir()+"/meta";
}

}//network
}//lxlib