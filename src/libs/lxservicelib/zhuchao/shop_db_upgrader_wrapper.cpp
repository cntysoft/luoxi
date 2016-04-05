#include <QProcess>
#include <QFileInfo>
#include <QJSEngine>
#include <QJSValue>
#include <QFile>
#include <QSqlQuery>

#include "shop_db_upgrader.h"

#include "corelib/kernel/settings.h"
#include "corelib/kernel/application.h"
#include "lxlib/global/const.h"
#include "lxlib/kernel/stddir.h"
#include "corelib/io/filesystem.h"
#include "corelib/global/common_funcs.h"
#include "corelib/kernel/errorinfo.h"
#include "corelib/utils/version.h"
#include <QDebug>

namespace lxservice{
namespace zhuchao{

using sn::corelib::Settings;
using sn::corelib::Application;
using lxlib::kernel::StdDir;
using sn::corelib::Filesystem;
using sn::corelib::throw_exception;
using sn::corelib::ErrorInfo;
using sn::corelib::dump_mysql_table;
using sn::corelib::utils::Version;

const QString ShopDbUpgraderWrapper::ZHUCHAO_UPGRADE_PKG_NAME_TPL = "zhuchaoweb_patch_%1_%2.tar.gz";
const QString ShopDbUpgraderWrapper::ZHUCHAO_UPGRADE_SCRIPT_NAME_TPL = "shop_upgradescript_%1_%2.js";
const QString ShopDbUpgraderWrapper::ZHUCHAO_DB_NAME_PREFIX = "zhuchao_site_";

ShopDbUpgraderWrapper::ShopDbUpgraderWrapper(ServiceProvider &provider)
   : AbstractService(provider)
{
   Settings& settings = Application::instance()->getSettings();
   m_dbHost = settings.getValue("dbHost", LUOXI_CFG_GROUP_GLOBAL).toString();
   m_dbUser = settings.getValue("dbUser", LUOXI_CFG_GROUP_GLOBAL).toString();
   m_dbPassword = settings.getValue("dbPassword", LUOXI_CFG_GROUP_GLOBAL).toString();
   m_dbEngine.reset(new DbEngine(DbEngine::QMYSQL, {
                                  {"host", m_dbHost},
                                  {"username", m_dbUser},
                                  {"password", m_dbPassword}
                               }));
}

ServiceInvokeResponse ShopDbUpgraderWrapper::upgrade(const ServiceInvokeRequest &request)
{
   if(m_step != STEP_PREPARE){
      throw_exception(ErrorInfo("状态错误"), getErrorContext());
   }
   m_context.reset(new UpgradeContext);
   QMap<QString, QVariant> args = request.getArgs();
   checkRequireFields(args, {"fromVersion", "toVersion", "forceDownloadPkg"});
   QString softwareRepoDir = StdDir::getSoftwareRepoDir();
   m_context->upgradeStatus = true;
   m_context->fromVersion = args.value("fromVersion").toString();
   m_context->toVersion = args.value("toVersion").toString();
   m_context->forceDownloadPkg = args.value("forceDownloadPkg").toBool();
   QString baseFilename = QString(ZHUCHAO_UPGRADE_PKG_NAME_TPL).arg(m_context->fromVersion, m_context->toVersion);
   QString upgradePkgFilename = softwareRepoDir + '/' + baseFilename;
   m_context->pkgFilename = upgradePkgFilename;
   m_context->request = request;
   QString fileModifyMetaFilename(getUpgradeTmpDir()+ '/' +QString(ZHUCHAO_UPGRADE_PKG_NAME_TPL).arg(m_context->fromVersion, m_context->toVersion));
   m_context->upgradeDir = fileModifyMetaFilename.mid(0, fileModifyMetaFilename.size() - 7);
   m_context->upgradeScript = m_context->upgradeDir + '/' + QString(ZHUCHAO_UPGRADE_SCRIPT_NAME_TPL).arg(m_context->fromVersion, m_context->toVersion);
   ServiceInvokeResponse response("ZhuChao/Upgrade/upgrade", true);
   response.setSerial(request.getSerial());
   m_context->response = &response;
   if(!Filesystem::fileExist(upgradePkgFilename) || m_context->forceDownloadPkg){
      downloadUpgradePkg(baseFilename);
      if(!m_context->upgradeStatus){
         setupFailureResponse(response);
         return response;
      }
   }
   m_context->response->setDataItem("step", STEP_EXTRA_PKG);
   m_context->response->setDataItem("msg", "正在解压升级压缩包");
   m_context->response->setStatus(true);

   //升级过程
   writeInterResponse(m_context->request, *m_context->response);
   unzipPkg(m_context->pkgFilename);
   if(!m_context->upgradeStatus){
      setupFailureResponse(response);
      return response;
   }
   if(!Filesystem::fileExist(m_context->upgradeScript)){
      response.setDataItem("msg", "升级脚本不存在，不需要升级");
      writeInterResponse(request, response);
      setupSuccessResponse(response);
      return response;
   }
   //变量定义
   QStringList databases;
   getShopDatabases(databases);
//   backupScriptFiles();
//   upgradeFiles();
//   if(!m_context->withoutUpgradeScript){
//      runUpgradeScript();
//      if(!m_context->upgradeStatus){
//         response.setDataItem("msg", m_context->upgradeErrorString);
//         writeInterResponse(request, response);
//         response.setStatus(false);
//         response.setDataItem("step", STEP_ERROR);
//         response.setError({-1, "升级失败"});
//         clearState();
//         return response;
//      }
//   }
//   upgradeComplete();
   setupSuccessResponse(response);
   return response;
}

void ShopDbUpgraderWrapper::setupSuccessResponse(ServiceInvokeResponse &response)
{
   response.setIsFinal(true);
   response.setStatus(true);
   response.setDataItem("msg", "升级完成");
   response.setDataItem("step", STEP_FINISH);
}

void ShopDbUpgraderWrapper::setupFailureResponse(ServiceInvokeResponse &response)
{
   response.setDataItem("msg", m_context->upgradeErrorString);
   writeInterResponse(m_context->request, response);
   response.setStatus(false);
   response.setDataItem("step", STEP_ERROR);
   response.setError({-1, "升级失败"});
   clearState();
}

void ShopDbUpgraderWrapper::getShopDatabases(const QStringList &databases)
{
   QSharedPointer<QSqlQuery> query = m_dbEngine->query("show databases");
   while(query->next()){
      qDebug() << query->value(0).toString();
   }
}

void ShopDbUpgraderWrapper::checkVersion()
{
//   QString versionFilename = m_deployDir + "/VERSION";
//   //无版本文件强行更新
//   if(!Filesystem::fileExist(versionFilename)){
//      return;
//   }
//   QString versionStr = QString(Filesystem::fileGetContents(versionFilename));
//   Version currentVersion(versionStr);
//   Version fromVersion(m_context->fromVersion);
//   Version toVersion(m_context->toVersion);
//   if(toVersion <= currentVersion){
//      clearState();
//      throw_exception(ErrorInfo("系统已经是最新版，无须更新"), getErrorContext());
//   }
//   if(fromVersion != currentVersion){
//      clearState();
//      throw_exception(ErrorInfo("起始版本跟系统当前版本号不一致"), getErrorContext());
//   }
}

void ShopDbUpgraderWrapper::downloadUpgradePkg(const QString &filename)
{
   //获取相关的配置信息
   Settings& settings = Application::instance()->getSettings();
   QSharedPointer<DownloadClientWrapper> downloader = getDownloadClient(settings.getValue("metaserverHost").toString(), 
                                                                 settings.getValue("metaserverPort").toInt());
   downloader->download(filename);
   m_eventLoop.exec();
}

void ShopDbUpgraderWrapper::backupDatabase()
{
//   m_step = STEP_BACKUP_DB;
//   m_context->response->setDataItem("step", STEP_BACKUP_DB);
//   m_context->response->setStatus(true);
//   m_context->response->setDataItem("msg", "正在备份数据库");
//   writeInterResponse(m_context->request, *m_context->response);
//   QString metaFilename = m_context->upgradeDir+'/'+QString(ZHUCHAO_UPGRADE_DB_META_NAME_TPL)
//         .arg(m_context->fromVersion, m_context->toVersion);
//   if(!Filesystem::fileExist(metaFilename)){
//      return;
//   }
//   QJsonParseError parserError;
//   QJsonDocument doc = QJsonDocument::fromJson(Filesystem::fileGetContents(metaFilename), &parserError);
//   if(QJsonParseError::NoError != parserError.error){
//      clearState();
//      throw_exception(ErrorInfo("文件变动元信息JSON文件解析错误"), getErrorContext());
//   }
//   QStringList needCopies;
//   QJsonObject rootObj = doc.object();
//   QJsonArray itemsVariant = rootObj.take("delete").toArray();
//   QJsonArray::const_iterator it = itemsVariant.constBegin();
//   QJsonArray::const_iterator endMarker = itemsVariant.constEnd();
//   while(it != endMarker){
//      needCopies.append((*it).toString());
//      it++;
//   }
//   itemsVariant = rootObj.take("modify").toArray();
//   it = itemsVariant.constBegin();
//   endMarker = itemsVariant.constEnd();
//   while(it != endMarker){
//      needCopies.append((*it).toString());
//      it++;
//   }
//   QString dbBackupDir = m_context->backupDir+"/dbbackup";
//   if(!Filesystem::dirExist(dbBackupDir)){
//      Filesystem::createPath(dbBackupDir);
//   }
//   for(int i = 0; i < needCopies.size(); i++){
//      m_context->response->setDataItem("msg", QString("正在备份数据表 %1").arg(needCopies[i]));
//      writeInterResponse(m_context->request, m_context->response);
//      dump_mysql_table(m_context->dbUser, m_context->dbPassword, ZHUCHAO_DB_NAME, needCopies[i], dbBackupDir);
//   }
}

bool ShopDbUpgraderWrapper::runUpgradeScript()
{
   m_step = STEP_RUN_UPGRADE_SCRIPT;
   m_context->response->setDataItem("step", STEP_RUN_UPGRADE_SCRIPT);
   m_context->response->setStatus(true);
   m_context->response->setDataItem("msg", "正在运行升级脚本");
   writeInterResponse(m_context->request, *m_context->response);
   QString upgradeScriptFilename = m_context->upgradeDir+'/'+QString(ZHUCHAO_UPGRADE_SCRIPT_NAME_TPL)
         .arg(m_context->fromVersion, m_context->toVersion);
   if(!Filesystem::fileExist(upgradeScriptFilename)){
      return true;
   }
   QSharedPointer<UpgradeEnvEngine> scriptEngine = getUpgradeScriptEngine();
   return scriptEngine->exec(upgradeScriptFilename);
}

void ShopDbUpgraderWrapper::upgradeComplete()
{
   m_step= STEP_FINISH;
//   //更新版本文件
//   QString versionFilename = m_deployDir + "/VERSION";
//   Filesystem::filePutContents(versionFilename, m_context->toVersion);
//   //设置权限
//   Filesystem::traverseFs(m_deployDir, 0, [&](QFileInfo &fileinfo, int){
//      Filesystem::chown(fileinfo.absoluteFilePath(), m_userId, m_groupId);
//   });
   clearState();
}

void ShopDbUpgraderWrapper::clearState()
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

void ShopDbUpgraderWrapper::unzipPkg(const QString &pkgFilename)
{
   QString targetExtraDir(getUpgradeTmpDir());
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
      m_context->upgradeStatus = false;
      m_context->upgradeErrorString = process.errorString();
   }
}

QSharedPointer<DownloadClientWrapper> ShopDbUpgraderWrapper::getDownloadClient(const QString &host, quint16 port)
{
   if(m_downloadClient.isNull()){
      m_downloadClient.reset(new DownloadClientWrapper(getServiceInvoker(host, port)));
      connect(m_downloadClient.data(), &DownloadClientWrapper::beginDownload, this, [&](){
         m_context->response->setDataItem("step", STEP_DOWNLOAD_PKG);
         m_context->response->setDataItem("msg", "开始下载软件包");
         writeInterResponse(m_context->request, *m_context->response);
      });
      QObject::connect(m_downloadClient.data(), &DownloadClientWrapper::downloadError, this, [&](int, const QString &errorMsg){
         m_eventLoop.exit();
         m_isInAction = false;
         m_step = STEP_PREPARE;
         m_context->upgradeStatus = false;
         m_context->upgradeErrorString = errorMsg;
      });
      connect(m_downloadClient.data(), &DownloadClientWrapper::downloadComplete, this, [&](){
         m_context->response->setDataItem("step", STEP_DOWNLOAD_COMPLETE);
         m_context->response->setStatus(true);
         m_context->response->setDataItem("msg", "下载软件包完成");
         writeInterResponse(m_context->request, *m_context->response);
         m_eventLoop.exit();
         m_step = STEP_DOWNLOAD_COMPLETE;
      });
   }
   return m_downloadClient;
}

QSharedPointer<UpgradeEnvEngine> ShopDbUpgraderWrapper::getUpgradeScriptEngine()
{
//   if(m_upgradeScriptEngine.isNull()){
//      m_upgradeScriptEngine.reset(new UpgradeEnvEngine(m_context->dbHost, m_context->dbUser, 
//                                                       m_context->dbPassword, ZHUCHAO_DB_NAME));
//      m_upgradeScriptEngine->initUpgradeEnv();
//      QJSEngine &engine = m_upgradeScriptEngine->getJsEngine();
//      QJSValue env = engine.newObject();
//      env.setProperty("backupDir", m_context->backupDir);
//      env.setProperty("upgradeDir", m_context->upgradeDir);
//      m_upgradeScriptEngine->registerContextObject("UpgradeMeta", env, true);
//      connect(m_upgradeScriptEngine.data(),& UpgradeEnvEngine::excpetionSignal, [&](ErrorInfo errorInfo){
//         m_context->upgradeStatus = false;
//         m_context->upgradeErrorString = errorInfo.toString();
//      });
//      connect(m_upgradeScriptEngine.data(),& UpgradeEnvEngine::logMsgSignal, [&](const QString &msg){
//         m_context->response->setDataItem("msg", msg);
//         m_context->response->setDataItem("step", STEP_RUN_UPGRADE_SCRIPT);
//         writeInterResponse(m_context->request, m_context->response);
//      });
//   }
   return m_upgradeScriptEngine;
}

QString ShopDbUpgraderWrapper::getBackupDir()
{
   return StdDir::getBaseDataDir()+"/backup";
}

QString ShopDbUpgraderWrapper::getUpgradeTmpDir()
{
   return StdDir::getBaseDataDir()+"/upgrade/tmp";
}

void ShopDbUpgraderWrapper::notifySocketDisconnect(QTcpSocket*)
{
   clearState();
}

}//zhuchao
}//lxservice
