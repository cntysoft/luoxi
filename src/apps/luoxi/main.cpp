#include <QTimer>
#include <csignal>

#include "corelib/io/terminal.h"
#include "corelib/kernel/errorinfo.h"
#include <QCoreApplication>

#include "application.h"
#include "command_runner.h"
#include <QDebug>

using LuoXiApplication = luoxi::Application;
using CommandRunner = luoxi::CommandRunner;
using ErrorInfo = sn::corelib::ErrorInfo;
using Terminal = sn::corelib::Terminal;
using TerminalColor = sn::corelib::TerminalColor;

//全局更新函数
namespace luoxi{
void global_initializer();
void global_cleanup();
}//luoxi

int main(int argc, char *argv[])
{
   try{
      LuoXiApplication app(argc, argv);
      qAddPreRoutine(luoxi::global_initializer);
      qAddPostRoutine(luoxi::global_cleanup);
      app.ensureImportantDir();
      app.watchUnixSignal(SIGINT, true);
      app.watchUnixSignal(SIGTERM, true);
      app.watchUnixSignal(SIGABRT, true);
      CommandRunner cmdrunner(app);
      QTimer::singleShot(0, Qt::PreciseTimer, [&cmdrunner]{
         cmdrunner.run();
      });
      return app.exec();
   }catch(const ErrorInfo& errorInfo){
      QString str(errorInfo.toString());
      if(str.size() > 0){
         str += '\n';
         Terminal::writeText(str.toLocal8Bit(), TerminalColor::Red);
      }
      return EXIT_FAILURE;
   }
}
