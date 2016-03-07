#include "global_command.h"

namespace luoxi{
namespace command{

using sn::corelib::TerminalColor;

GlobalHelpCommand::GlobalHelpCommand(AbstractCommandRunner& runner, const CommandMeta& invokeMeta)
   : AbstractCommand(runner, invokeMeta)
{
}

void GlobalHelpCommand::exec()
{
   m_cmdRunner.printUsage();
   exit(EXIT_SUCCESS);
}

GlobalHelpCommand::~GlobalHelpCommand()
{}


GlobalVersionCommand::GlobalVersionCommand(AbstractCommandRunner& runner, const CommandMeta& invokeMeta)
   : AbstractCommand(runner, invokeMeta)
{
}

void GlobalVersionCommand::exec()
{
   printConsoleMsg("luoxi version ");
   printConsoleMsg(QString("%1\n").arg(LUOXI_VERSION), TerminalColor::Cyan);
   exit(EXIT_SUCCESS);
}

GlobalVersionCommand::~GlobalVersionCommand()
{}

}//command
}//luoxi