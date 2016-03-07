#ifndef LUOXI_COMMAND_GLOBAL_COMMAND_H
#define LUOXI_COMMAND_GLOBAL_COMMAND_H

#include "corelib/command/abstract_command.h"

namespace luoxi{
namespace command{

using sn::corelib::AbstractCommand;
using sn::corelib::AbstractCommandRunner;
using sn::corelib::CommandMeta;

class GlobalHelpCommand : public AbstractCommand 
{
public:
   GlobalHelpCommand(AbstractCommandRunner &runner, const CommandMeta &invokeMeta);
public:
   virtual void exec();
   virtual ~GlobalHelpCommand();
};


class GlobalVersionCommand : public AbstractCommand 
{
public:
   GlobalVersionCommand(AbstractCommandRunner& runner, const CommandMeta& invokeMeta);
public:
   virtual void exec();
   virtual ~GlobalVersionCommand();
};

}//command
}//luoxi

#endif // LUOXI_COMMAND_GLOBAL_COMMAND_H
