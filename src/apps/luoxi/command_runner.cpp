#include "corelib/kernel/errorinfo.h"
#include "corelib/io/terminal.h"
#include "corelib/command/abstract_command.h"
#include "corelib/command/command_meta.h"

#include "command_runner.h"
#include "application.h"
#include "command/command_repo.h"

namespace luoxi {

using sn::corelib::ErrorInfo;
using sn::corelib::TerminalColor;
using sn::corelib::CommandMeta;
using sn::corelib::AbstractCommand;
using sn::corelib::AbstractCommandRunner;

using luoxi::command::GlobalVersionCommand;
using luoxi::command::GlobalHelpCommand;
using luoxi::command::StartServerCommand;
using luoxi::command::GlobalPidFilenameCommand;

CommandRunner::CommandRunner(Application &app)
   : AbstractCommandRunner(app)
{
   addUsageText("welcome to use sheneninfo luoxi system\n", TerminalColor::Green);
   addUsageText("usage: \n", TerminalColor::LightBlue);
   addUsageText("--version  print luoxi system version number\n");
   addUsageText("--help     print help document\n");
   addUsageText("start [--daemon] [--port] start luoxi server\n");
   addUsageText("pidfilename get application pid filename\n\n");
   initCommandPool();
   initRouteItems();
}

void CommandRunner::initCommandPool()
{
   m_cmdRegisterPool.insert("Global_Version", [](AbstractCommandRunner& runner, const CommandMeta& meta)->AbstractCommand*{
      GlobalVersionCommand *cmd = new GlobalVersionCommand(dynamic_cast<CommandRunner&>(runner), meta);
      return cmd;
   });
   m_cmdRegisterPool.insert("Global_Help", [](AbstractCommandRunner& runner, const CommandMeta& meta)->AbstractCommand*{
      GlobalHelpCommand *cmd = new GlobalHelpCommand(dynamic_cast<CommandRunner&>(runner), meta);
      return cmd;
   });
   m_cmdRegisterPool.insert("Global_StartServer", [](AbstractCommandRunner& runner, const CommandMeta& meta)->AbstractCommand*{
      StartServerCommand *cmd = new StartServerCommand(dynamic_cast<CommandRunner&>(runner), meta);
      return cmd;
   });
   m_cmdRegisterPool.insert("Global_PidFilename", [](AbstractCommandRunner& runner, const CommandMeta& meta)->AbstractCommand*{
      GlobalPidFilenameCommand* cmd = new GlobalPidFilenameCommand(dynamic_cast<CommandRunner&>(runner), meta);
      return cmd;
   });
}

void CommandRunner::initRouteItems()
{
   addCmdRoute("version", "--version", 1, {
                  {"category", "Global"},
                  {"name", "Version"}
               });
   addCmdRoute("help", "--help", 1, {
                  {"category", "Global"},
                  {"name", "Help"}
               });
   addCmdRoute("startserver", "start [--daemon] [--port]", 1, {
                  {"category", "Global"},
                  {"name", "StartServer"}
               });
   addCmdRoute("pidfilename", "pidfilename", 1, {
                  {"category", "Global"},
                  {"name", "PidFilename"}
               });
}

}//luoxi
