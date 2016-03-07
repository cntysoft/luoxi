#ifndef LUOXI_COMMAND_RUNNER_H
#define LUOXI_COMMAND_RUNNER_H

#include "corelib/command/abstract_command_runner.h"

namespace luoxi{

using sn::corelib::AbstractCommandRunner;
class Application;

class CommandRunner : public AbstractCommandRunner
{
public:
   CommandRunner(Application &app);
private:
   void initCommandPool();
   void initRouteItems();
};

}//luoxi

#endif // LUOXI_COMMAND_RUNNER_H