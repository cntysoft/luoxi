#ifndef LUOXI_APPLICATION_H
#define LUOXI_APPLICATION_H

#include "corelib/kernel/application.h"
#include "corelib/kernel/settings.h"

namespace luoxi{

using BaseApplication = sn::corelib::Application;
using sn::corelib::Settings;

class Application : public BaseApplication
{
public:
   Application(int &argc, char **argv);
public:
   virtual Settings::CfgInitializerFnType getDefaultCfgInitializerFn();
   virtual ~Application();
};

}//luoxi

#endif // LUOXI_APPLICATION_H
