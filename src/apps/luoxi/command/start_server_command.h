#ifndef LUOXI_COMMAND_START_SERVER_COMMAND_H
#define LUOXI_COMMAND_START_SERVER_COMMAND_H

#include "corelib/command/abstract_command.h"

namespace luoxi{
namespace command{

using sn::corelib::AbstractCommand;
using sn::corelib::AbstractCommandRunner;
using sn::corelib::CommandMeta;

class StartServerCommand : public AbstractCommand
{
public:
   StartServerCommand(AbstractCommandRunner& runner, const CommandMeta& invokeMeta);
public:
   virtual void exec();
};

}//command
}//luoxi

#endif // LUOXI_COMMAND_START_SERVER_COMMAND_H
