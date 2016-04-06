#include <QProcess>
#include <QFileInfo>
#include <QJSEngine>
#include <QJSValue>
#include <QFile>
#include <QSqlQuery>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QScopedPointer>

#include "shop_db_upgrader.h"


#include "lxlib/global/const.h"
#include "lxlib/kernel/stddir.h"
#include "corelib/kernel/settings.h"
#include "corelib/kernel/application.h"
#include "corelib/io/filesystem.h"
#include "corelib/global/common_funcs.h"
#include "corelib/kernel/errorinfo.h"
#include "corelib/utils/version.h"
#include "corelib/db/metadata/metadata.h"
#include "corelib/upgrade/upgrade_env_script_object.h"
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
using DbMetaData = sn::corelib::db::metadata::Metadata;
using sn::corelib::upgrade::UpgradeEnvScriptObject;

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
   m_context->backupDir = getBackupDir();
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
   for(const QString &name : databases){
      upgradeCycle(name);
   }
   upgradeComplete();
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

void ShopDbUpgraderWrapper::getShopDatabases(QStringList &databases)
{
   QSharedPointer<QSqlQuery> query = m_dbEngine->query("show databases");
   QRegularExpression regex("zhuchao_site_\\d+$");
   regex.setPatternOptions(QRegularExpression::MultilineOption);
   while(query->next()){
      QString name = query->value(0).toString();
      QRegularExpressionMatch m = regex.match(name);
      if(m.hasMatch()){
         databases << name;
      }
   }
}

void ShopDbUpgraderWrapper::upgradeCycle(const QString &dbname)
{
   m_step = STEP_CYCLE_BEGIN;
   m_context->response->setDataItem("msg", QString("开始升级数据库%1").arg(dbname));
   writeInterResponse(m_context->request, *m_context->response);
   m_context->currentDbname = dbname;
   if(!m_dbEngine->changeSchema(dbname)){
      m_context->response->setDataItem("msg", QString("升级错误：%1").arg(m_dbEngine->getLastErrorString()));
      writeInterResponse(m_context->request, *m_context->response);
      return;
   }
   if(!checkVersion()){
      return;
   }
   backupDatabase();
   runUpgradeScript();
   if(!m_context->upgradeStatus){
      m_context->response->setDataItem("msg", QString("运行升级脚本错误：%1").arg(m_context->upgradeErrorString));
      writeInterResponse(m_context->request, *m_context->response);
      return;
   }
   cycleComplete();
}

bool ShopDbUpgraderWrapper::checkVersion()
{
   QSharedPointer<QSqlQuery> query = m_dbEngine->query("SELECT `value` FROM `sys_m_std_config` WHERE `key` = 'dbVersion'");
   if(query->size() == 0){
      return true;
   }
   query->first();
   Version currentVersion(query->value(0).toString());
   Version fromVersion(m_context->fromVersion);
   Version toVersion(m_context->toVersion);
   if(toVersion <= currentVersion){
      m_context->response->setDataItem("msg", QString("系统已经是最新版，无须更新"));
      writeInterResponse(m_context->request, *m_context->response);
      return false;
   }
   if(fromVersion != currentVersion){
      m_context->response->setDataItem("msg", QString("起始版本跟系统当前版本号不一致"));
      writeInterResponse(m_context->request, *m_context->response);
      return false;
   }
   return true;
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
   m_step = STEP_BACKUP_DB;
   m_context->response->setDataItem("step", STEP_BACKUP_DB);
   m_context->response->setStatus(true);
   m_context->response->setDataItem("msg", "正在备份数据库");
   writeInterResponse(m_context->request, *m_context->response);
   QScopedPointer<DbMetaData> metadata(new DbMetaData(m_dbEngine));
   QStringList tableNames(metadata->getTableNames(m_context->currentDbname));
   QString dbBackupDir = m_context->backupDir+"/shop/dbbackup/" + m_context->currentDbname;
   if(!Filesystem::dirExist(dbBackupDir)){
      Filesystem::createPath(dbBackupDir);
   }
   for(int i = 0; i < tableNames.size(); i++){
      dump_mysql_table(m_dbUser, m_dbPassword, m_context->currentDbname, tableNames[i], dbBackupDir);
   }
}

bool ShopDbUpgraderWrapper::runUpgradeScript()
{
   m_step = STEP_RUN_UPGRADE_SCRIPT;
   m_context->response->setDataItem("step", STEP_RUN_UPGRADE_SCRIPT);
   m_context->response->setStatus(true);
   m_context->response->setDataItem("msg", "正在运行升级脚本");
   writeInterResponse(m_context->request, *m_context->response);
   QSharedPointer<UpgradeEnvEngine> scriptEngine = getUpgradeScriptEngine();
   QSharedPointer<UpgradeEnvScriptObject> upgradeEnvScriptObject = scriptEngine->getUpgradeEnvScriptObject();
   upgradeEnvScriptObject->changeUnderlineDbSchema(m_context->currentDbname);
   return scriptEngine->exec(m_context->upgradeScript);
}

void ShopDbUpgraderWrapper::cycleComplete()
{
   m_step = STEP_RUN_UPGRADE_SCRIPT;
   m_context->response->setDataItem("step", STEP_RUN_UPGRADE_SCRIPT);
   m_context->response->setStatus(true);
   QSharedPointer<QSqlQuery> query = m_dbEngine->query("SELECT `value` FROM `sys_m_std_config` WHERE `key` = 'dbVersion' AND `sys_m_std_config`.`group` = 'Kernel'");
   if(query->size() == 0){
      m_dbEngine->query(QString("INSERT INTO `sys_m_std_config` (`key`, `group`, `value`) VALUES ('dbVersion', 'Kernel', '%1')").arg(m_context->toVersion));
   }else{
      m_dbEngine->query(QString("UPDATE `sys_m_std_config` SET `value` = '%1' WHERE `sys_m_std_config`.`key` = 'dbVersion' AND `sys_m_std_config`.`group` = 'Kernel'").arg(m_context->toVersion));
   }
   m_context->response->setDataItem("msg", QString("数据库%1升级完成").arg(m_context->currentDbname));
   writeInterResponse(m_context->request, *m_context->response);
}

void ShopDbUpgraderWrapper::upgradeComplete()
{
   m_step= STEP_FINISH;
   //更新版本文件
   if(Filesystem::fileExist(m_context->upgradeDir)){
      Filesystem::deleteDirRecusive(m_context->upgradeDir);
   }
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
   if(m_upgradeScriptEngine.isNull()){
      m_upgradeScriptEngine.reset(new UpgradeEnvEngine(m_dbHost, m_dbUser, 
                                                       m_dbPassword));
      m_upgradeScriptEngine->initUpgradeEnv();
      QJSEngine &engine = m_upgradeScriptEngine->getJsEngine();
      QJSValue env = engine.newObject();
      env.setProperty("backupDir", m_context->backupDir);
      env.setProperty("upgradeDir", m_context->upgradeDir);
      m_upgradeScriptEngine->registerContextObject("UpgradeMeta", env, true);
      connect(m_upgradeScriptEngine.data(),& UpgradeEnvEngine::excpetionSignal, [&](ErrorInfo errorInfo){
         m_context->upgradeStatus = false;
         m_context->upgradeErrorString = errorInfo.toString();
      });
      connect(m_upgradeScriptEngine.data(),& UpgradeEnvEngine::logMsgSignal, [&](const QString &msg){
         m_context->response->setDataItem("msg", msg);
         m_context->response->setDataItem("step", STEP_RUN_UPGRADE_SCRIPT);
         writeInterResponse(m_context->request, *m_context->response);
      });
   }
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
