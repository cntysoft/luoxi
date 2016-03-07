#include "application.h"

namespace luoxi {

extern void init_defualt_cfg(Settings &settings);

Application::Application(int &argc, char **argv)
   : BaseApplication(argc, argv)
{
   setApplicationName("luoxi");
}

Settings::CfgInitializerFnType Application::getDefaultCfgInitializerFn()
{
   return init_defualt_cfg;
}

Application::~Application()
{}

}//luoxi
