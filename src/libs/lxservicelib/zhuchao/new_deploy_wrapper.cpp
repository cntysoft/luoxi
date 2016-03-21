#include "new_deploy.h"

#include <QProcess>
#include <QSqlQuery>
#include <QProcessEnvironment>
#include <QVariant>
#include <QByteArray>
#include <cstdlib>

#include "corelib/kernel/settings.h"
#include "corelib/kernel/application.h"
#include "corelib/global/common_funcs.h"
#include "corelib/kernel/errorinfo.h"
#include "corelib/io/filesystem.h"
#include "lxlib/global/const.h"
#include "lxlib/kernel/stddir.h"
#include "corelib/kernel/stddir.h"

namespace lxservice{
namespace zhuchao{

using sn::corelib::Settings;
using sn::corelib::Application;
using sn::corelib::throw_exception;
using sn::corelib::ErrorInfo;
using sn::corelib::Filesystem;
using lxlib::kernel::StdDir;

const QString NewDeployWrapper::ZHUCHAO_PKG_NAME_TPL = "zhuchaoweb_%1.tar.gz";
const QString NewDeployWrapper::DEPLOY_DIR_TPL = "zhuchao";
const QString NewDeployWrapper::DB_NAME_TPL = "zhuchao";
const QString NewDeployWrapper::DB_SQL_FILENAME_TPL = "zhuchaoweb_%1.sql";

NewDeployWrapper::NewDeployWrapper(ServiceProvider& provider)
   :AbstractService(provider)
{
   Settings& settings = Application::instance()->getSettings();
   m_deployBaseDir = settings.getValue("deployBaseDir", LUOXI_CFG_GROUP_GLOBAL, "/srv/www").toString();
   m_userId = settings.getValue("webDeployUserId", LUOXI_CFG_GROUP_GLOBAL, 30).toInt();
   m_groupId = settings.getValue("webDeployGroupId", LUOXI_CFG_GROUP_GLOBAL, 8).toInt();
   m_dbHost = settings.getValue("dbHost", LUOXI_CFG_GROUP_GLOBAL).toString();
   m_dbUser = settings.getValue("dbUser", LUOXI_CFG_GROUP_GLOBAL).toString();
   m_dbPassword = settings.getValue("dbPassword", LUOXI_CFG_GROUP_GLOBAL).toString();
   m_sqlEngine.reset(new SqlEngine(SqlEngine::QMYSQL, {
                                      {"host", m_dbHost},
                                      {"username", m_dbUser},
                                      {"password", m_dbPassword}
                                   }));
}

ServiceInvokeResponse NewDeployWrapper::deploy(const ServiceInvokeRequest &request)
{
   if(m_step != STEP_PREPARE){
      throw_exception(ErrorInfo("状态错误"), getErrorContext());
   }
   m_step = STEP_INIT_CONTEXT;
   m_context.reset(new NewDeployContext);
   QMap<QString, QVariant> args = request.getArgs();
   checkRequireFields(args, {"targetVersion", "withoutDb"});
   QString softwareRepoDir = StdDir::getSoftwareRepoDir();
   m_context->deployStatus = true;
   m_context->targetVersion = args.value("targetVersion").toString();
   m_context->withoutDb = args.value("withoutDb").toBool();
   QString baseFilename = QString(ZHUCHAO_PKG_NAME_TPL).arg(m_context->targetVersion);
   QString upgradePkgFilename = softwareRepoDir + '/' + baseFilename;
   m_context->pkgFilename = upgradePkgFilename;
   m_context->request = request;
   ServiceInvokeResponse response("ZhuChao/NewDeploy/deploy", true);
   response.setSerial(request.getSerial());
   m_context->response = response;
   //检查站点目录是否存在
   QString targetDeployDir = m_deployBaseDir+'/' + DEPLOY_DIR_TPL;
   m_context->deployDir = targetDeployDir;
   //下载升级文件到本地, 每次都强行下载
   downloadZhuChaoPkg(baseFilename);
   if(!m_context->deployStatus){
      goto process_error;
   }
   unzipPkg(m_context->pkgFilename);
   if(!m_context->deployStatus){
      goto process_error;
   }
   copyFilesToDeployDir();
   if(!m_context->deployStatus){
      goto process_error;
   }
   if(!m_context->withoutDb){
      createDatabase();
      if(!m_context->deployStatus){
         goto process_error;
      }
   }
   m_step = STEP_FINISH;
   response.setStatus(true);
   response.setDataItem("msg", "筑巢部署完成");
   response.setDataItem("step", STEP_FINISH);
   return response;
process_error:
   response.setDataItem("msg", m_context->deployErrorString);
   writeInterResponse(request, response);
   response.setStatus(false);
   response.setDataItem("step", STEP_ERROR);
   response.setError({-1, "筑巢部署失败"});
   clearState();
   return response;
}

void NewDeployWrapper::downloadZhuChaoPkg(const QString &filename)
{
   //获取相关的配置信息
   Settings& settings = Application::instance()->getSettings();
   QSharedPointer<DownloadClientWrapper> downloader = getDownloadClient(settings.getValue("metaserverHost").toString(), 
                                                                 settings.getValue("metaserverPort").toInt());
   downloader->download(filename);
   m_eventLoop.exec();
}

void NewDeployWrapper::unzipPkg(const QString &pkgFilename)
{
   m_context->response.setDataItem("step", STEP_EXTRA_PKG);
   m_context->response.setDataItem("msg", "正在解压升级压缩包");
   m_context->response.setStatus(true);
   writeInterResponse(m_context->request, m_context->response);
   QString targetExtraDir(getDeployTmpDir());
   if(!Filesystem::dirExist(targetExtraDir)){
      Filesystem::createPath(targetExtraDir);
   }
   QProcess process;
   QStringList args;
   process.setWorkingDirectory(targetExtraDir);
   args << "-zxvf" << pkgFilename;
   process.start("tar", args);
   bool status = process.waitForFinished(-1);
   if(!status || process.exitCode() != 0){
      m_context->deployStatus = false;
      m_context->deployErrorString = process.errorString();
   }
}

void NewDeployWrapper::copyFilesToDeployDir()
{
   m_step = STEP_COPY_FILES;
   m_context->response.setDataItem("step", STEP_COPY_FILES);
   m_context->response.setDataItem("msg", "正在复制项目文件");
   writeInterResponse(m_context->request, m_context->response);
   Filesystem::createPath(m_context->deployDir);
   QString sourceDir = ZHUCHAO_PKG_NAME_TPL.mid(0, ZHUCHAO_PKG_NAME_TPL.size() - 7);
   sourceDir = getDeployTmpDir() + '/'+sourceDir.arg(m_context->targetVersion);
   if(Filesystem::dirExist(m_context->deployDir)){
      Filesystem::deleteDirRecusive(m_context->deployDir);
   }
   if(!Filesystem::copyDir(sourceDir, m_context->deployDir, true)){
      m_context->deployErrorString = Filesystem::getErrorString();
      m_context->deployStatus = false;
   }
}

void NewDeployWrapper::createDatabase()
{
   m_step = STEP_CREATE_DB;
   m_context->response.setDataItem("step", STEP_CREATE_DB);
   m_context->response.setDataItem("msg", "正在创建站点数据库");
   writeInterResponse(m_context->request, m_context->response);
   try{
      m_sqlEngine->query(QString("DROP DATABASE IF EXISTS `%1`").arg(DB_NAME_TPL));
      m_sqlEngine->query(QString("CREATE DATABASE `%1`").arg(DB_NAME_TPL));
   }catch(ErrorInfo error){
      m_context->deployStatus = false;
      m_context->deployErrorString = error.toString();
   }
   QProcessEnvironment env(QProcessEnvironment::systemEnvironment());
   QString ldLibraryPath(env.value("LD_LIBRARY_PATH"));
   ldLibraryPath = "/usr/lib64:"+ldLibraryPath;
   ::setenv("LD_LIBRARY_PATH", ldLibraryPath.toLocal8Bit(), 1);
   QString cmd = QString("mysql -u%1 -p%2 %3 < %4").arg(m_dbUser, m_dbPassword, DB_NAME_TPL, m_context->deployDir+'/'+QString(DB_SQL_FILENAME_TPL).arg(m_context->targetVersion));
   int exitCode = std::system(cmd.toLocal8Bit());
   if(0 != exitCode){
      m_context->deployStatus = false;
      m_context->deployErrorString = QString("创建数据库 %1 失败").arg(DB_NAME_TPL);
   }
}


QString NewDeployWrapper::getDeployTmpDir()
{
   return StdDir::getBaseDataDir()+"/deploy/tmp";
}

QSharedPointer<DownloadClientWrapper> NewDeployWrapper::getDownloadClient(const QString &host, quint16 port)
{
   if(m_downloadClient.isNull()){
      m_downloadClient.reset(new DownloadClientWrapper(getServiceInvoker(host, port)));
      connect(m_downloadClient.data(), &DownloadClientWrapper::beginDownload, this, [&](){
         m_context->response.setDataItem("step", STEP_DOWNLOAD_PKG);
         m_context->response.setDataItem("msg", "开始下载软件包");
         writeInterResponse(m_context->request, m_context->response);
      });
      QObject::connect(m_downloadClient.data(), &DownloadClientWrapper::downloadError, this, [&](int, const QString &errorMsg){
         m_eventLoop.exit();
         m_isInAction = false;
         m_step = STEP_PREPARE;
         m_context->deployStatus = false;
         m_context->deployErrorString = errorMsg;
      });
      connect(m_downloadClient.data(), &DownloadClientWrapper::downloadComplete, this, [&](){
         m_context->response.setDataItem("step", STEP_DOWNLOAD_COMPLETE);
         m_context->response.setStatus(true);
         m_context->response.setDataItem("msg", "下载软件包完成");
         writeInterResponse(m_context->request, m_context->response);
         m_eventLoop.exit();
         m_step = STEP_DOWNLOAD_COMPLETE;
      });
   }
   return m_downloadClient;
}

void NewDeployWrapper::cleanupTmpFiles()
{
   m_step = STEP_CLEANUP;
   m_context->response.setDataItem("step", STEP_CLEANUP);
   m_context->response.setDataItem("msg", "正在删除临时文件");
   writeInterResponse(m_context->request, m_context->response);
   if(Filesystem::dirExist(getDeployTmpDir())){
      Filesystem::deleteDir(getDeployTmpDir());
   }
}

void NewDeployWrapper::clearState()
{
   m_context.clear();
   m_step = STEP_PREPARE;
   if(!m_downloadClient.isNull()){
      m_downloadClient->clearState();
   }
   //清除残余文件
   if(!m_serviceInvoker.isNull()){
      m_serviceInvoker->disconnectFromServer();
   }
}


}//zhuchao
}//lxservice