#include <QString>
#include <QTcpServer>
#include <QProcess>
#include <QDir>
#include <QVariant>
#include <QDebug>

#include "start_server_command.h"
#include "utils/common_funcs.h"
#include "corelib/command/command_meta.h"
#include "corelib/kernel/errorinfo.h"
#include "corelib/kernel/settings.h"
#include "corelib/network/web_service_server.h"
#include "lxlib/global/const.h"
#include "lxlib/network/multi_thread_server.h"
#include "start_server_command.h"

namespace luoxi{
namespace command{

using sn::corelib::CommandMeta;
using sn::corelib::ErrorInfo;
using sn::corelib::Settings;

using lxlib::network::MultiThreadServer;
using lxlib::network::set_global_server;
using sn::corelib::network::WebServiceServer;

using luoxi::Application;
using luoxi::get_app_ref;

StartServerCommand::StartServerCommand(AbstractCommandRunner& runner, const CommandMeta& invokeMeta)
   : AbstractCommand(runner, invokeMeta)
{
}

void StartServerCommand::exec()
{
   bool daemon = m_invokeMeta.getCmdArgs().value("daemon") == "true" ? true : false;
   Application& app = *get_app_ref();
   if(daemon){
      QStringList args = app.arguments();
      args.takeFirst();
      args.removeAll("--daemon");
      if(QProcess::startDetached(Application::applicationFilePath(), args, QDir::currentPath())){
         app.exit(EXIT_SUCCESS);
      }else{
         throw ErrorInfo("start daemon failure, Failed to create process");
      }
   }
   MultiThreadServer *server = new MultiThreadServer(app);
   WebServiceServer *webServiceServer = new WebServiceServer(app, "websocketserver");
   lxlib::network::set_global_server(server);
   sn::corelib::network::set_global_web_service_server(webServiceServer);
   server->setHost(QHostAddress::Any);
   server->setPort(LUOXI_LISTEN_PORT);
   webServiceServer->setHost(QHostAddress::Any);
   webServiceServer->setPort(LUOXI_LISTEN_PORT + 1);
   app.createPidFile();
   bool status = server->run();
   bool webServiceStatus = webServiceServer->run();
   //防止内存泄漏,这里利用闭包复制指针
   QObject::connect(&app, &Application::aboutToQuit, [&app](){
      MultiThreadServer *&server = lxlib::network::get_global_server();
      if(nullptr != server){
         delete server;
      }
      WebServiceServer *&webServiceServer = sn::corelib::network::get_global_web_service_server();
      if(nullptr != webServiceServer){
         delete webServiceServer;
      }
      app.deletePidFile();
   });
   if(!status){
      throw ErrorInfo(server->errorString());
   }
   if(!webServiceStatus){
      throw ErrorInfo(webServiceServer->errorString());
   }
}


}//command
}//luoxi
