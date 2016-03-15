#include "global_command.h"
#include "corelib/kernel/application.h"
#include "corelib/io/terminal.h"

#include <iostream>

namespace luoxi{
namespace command{

using sn::corelib::TerminalColor;
using sn::corelib::Application;

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


GlobalPidFilenameCommand::GlobalPidFilenameCommand(AbstractCommandRunner& runner, const CommandMeta& invokeMeta)
   : AbstractCommand(runner, invokeMeta)
{
}

void GlobalPidFilenameCommand::exec()
{
   std::cout << Application::instance()->getPidFilename().toStdString();
   exit(EXIT_SUCCESS);
}

GlobalPidFilenameCommand::~GlobalPidFilenameCommand()
{}

}//command
}//luoxi