#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QFileInfo>
#include <QJSEngine>
#include <QJSValue>
#include <QFile>

#include "upgrade_deploy.h"

#include "corelib/kernel/settings.h"
#include "corelib/kernel/application.h"
#include "lxlib/global/const.h"
#include "lxlib/kernel/stddir.h"
#include "corelib/io/filesystem.h"
#include "corelib/global/common_funcs.h"
#include "corelib/kernel/errorinfo.h"
#include "corelib/utils/version.h"

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

const QString UpgradeDeployWrapper::ZHUCHAO_UPGRADE_PKG_NAME_TPL = "zhuchaoweb_patch_%1_%2.tar.gz";
const QString UpgradeDeployWrapper::ZHUCHAO_UPGRADE_DB_META_NAME_TPL = "dbmeta_%1_%2.json";
const QString UpgradeDeployWrapper::ZHUCHAO_UPGRADE_SCRIPT_NAME_TPL = "upgradescript_%1_%2.js";
const QString UpgradeDeployWrapper::ZHUCHAO_DB_NAME = "zhuchao";

UpgradeDeployWrapper::UpgradeDeployWrapper(ServiceProvider &provider)
   : AbstractService(provider)
{
   Settings& settings = Application::instance()->getSettings();
   m_deployDir = settings.getValue("deployBaseDir", LUOXI_CFG_GROUP_GLOBAL, "/srv/www").toString() + "/zhuchao";
   m_userId = settings.getValue("webDeployUserId", LUOXI_CFG_GROUP_GLOBAL, 30).toInt();
   m_groupId = settings.getValue("webDeployGroupId", LUOXI_CFG_GROUP_GLOBAL, 8).toInt();
}

ServiceInvokeResponse UpgradeDeployWrapper::upgrade(const ServiceInvokeRequest &request)
{
   if(m_step != STEP_PREPARE){
      throw_exception(ErrorInfo("状态错误"), getErrorContext());
   }
   m_context.reset(new UpgradeContext);
   QMap<QString, QVariant> args = request.getArgs();
   checkRequireFields(args, {"fromVersion", "toVersion", "forceUpgrade", "withoutUpgradeScript"});
   QString softwareRepoDir = StdDir::getSoftwareRepoDir();
   m_context->upgradeStatus = true;
   m_context->fromVersion = args.value("fromVersion").toString();
   m_context->toVersion = args.value("toVersion").toString();
   m_context->forceUpgrade = args.value("forceUpgrade").toBool();
   m_context->withoutUpgradeScript = args.value("withoutUpgradeScript").toBool();
   if(!m_context->forceUpgrade){
      checkVersion();
   }
   //获取数据库信息
   Settings& settings = Application::instance()->getSettings();
   m_context->dbHost = settings.getValue("dbHost", LUOXI_CFG_GROUP_GLOBAL).toString();
   m_context->dbUser = settings.getValue("dbUser", LUOXI_CFG_GROUP_GLOBAL).toString();
   m_context->dbPassword = settings.getValue("dbPassword", LUOXI_CFG_GROUP_GLOBAL).toString();
   QString baseFilename = QString(ZHUCHAO_UPGRADE_PKG_NAME_TPL).arg(m_context->fromVersion, m_context->toVersion);
   QString upgradePkgFilename = softwareRepoDir + '/' + baseFilename;
   m_context->pkgFilename = upgradePkgFilename;
   m_context->request = request;
   ServiceInvokeResponse response("ZhuChao/Upgrade/upgrade", true);
   response.setSerial(request.getSerial());
   m_context->response = response;
   downloadUpgradePkg(baseFilename);
   if(!m_context->upgradeStatus){
      response.setDataItem("step", STEP_DOWNLOAD_PKG);
      response.setDataItem("msg", m_context->upgradeErrorString);
      writeInterResponse(request, response);
      response.setStatus(false);
      response.setDataItem("step", STEP_ERROR);
      response.setError({-1, "升级失败"});
      clearState();
      return response;
   }
   m_context->response.setDataItem("step", STEP_EXTRA_PKG);
   m_context->response.setDataItem("msg", "正在解压升级压缩包");
   m_context->response.setStatus(true);
   writeInterResponse(m_context->request, m_context->response);
   unzipPkg(m_context->pkgFilename);
   backupScriptFiles();
   upgradeFiles();
   if(!m_context->withoutUpgradeScript){
      runUpgradeScript();
      if(!m_context->upgradeStatus){
         response.setDataItem("msg", m_context->upgradeErrorString);
         writeInterResponse(request, response);
         response.setStatus(false);
         response.setDataItem("step", STEP_ERROR);
         response.setError({-1, "升级失败"});
         clearState();
         return response;
      }
   }
   upgradeComplete();
   response.setSerial(request.getSerial());
   response.setIsFinal(true);
   response.setStatus(true);
   response.setDataItem("msg", "升级完成");
   response.setDataItem("step", STEP_FINISH);
   return response;
}

void UpgradeDeployWrapper::checkVersion()
{
   QString versionFilename = m_deployDir + "/VERSION";
   //无版本文件强行更新
   if(!Filesystem::fileExist(versionFilename)){
      return;
   }
   QString versionStr = QString(Filesystem::fileGetContents(versionFilename));
   Version currentVersion(versionStr);
   Version fromVersion(m_context->fromVersion);
   Version toVersion(m_context->toVersion);
   if(toVersion <= currentVersion){
      clearState();
      throw_exception(ErrorInfo("系统已经是最新版，无须更新"), getErrorContext());
   }
   if(fromVersion != currentVersion){
      clearState();
      throw_exception(ErrorInfo("起始版本跟系统当前版本号不一致"), getErrorContext());
   }
}

void UpgradeDeployWrapper::downloadUpgradePkg(const QString &filename)
{
   //获取相关的配置信息
   Settings& settings = Application::instance()->getSettings();
   QSharedPointer<DownloadClientWrapper> downloader = getDownloadClient(settings.getValue("metaserverHost").toString(), 
                                                                 settings.getValue("metaserverPort").toInt());
   downloader->download(filename);
   m_eventLoop.exec();
}

void UpgradeDeployWrapper::backupScriptFiles()
{
   m_step = STEP_BACKUP_FILES;
   m_context->response.setDataItem("step", STEP_BACKUP_FILES);
   m_context->response.setStatus(true);
   m_context->response.setDataItem("msg", "正在备份程序文件");
   writeInterResponse(m_context->request, m_context->response);
   QString fileModifyMetaFilename(getUpgradeTmpDir()+ '/' +QString(ZHUCHAO_UPGRADE_PKG_NAME_TPL).arg(m_context->fromVersion, m_context->toVersion));
   QString upgradeDir = fileModifyMetaFilename.mid(0, fileModifyMetaFilename.size() - 7);
   fileModifyMetaFilename = upgradeDir + QString("/%1_%2.json").arg(m_context->fromVersion, m_context->toVersion);
   m_context->upgradeDir = upgradeDir;
   QJsonParseError parserError;
   QJsonDocument doc = QJsonDocument::fromJson(Filesystem::fileGetContents(fileModifyMetaFilename), &parserError);
   if(QJsonParseError::NoError != parserError.error){
      clearState();
      throw_exception(ErrorInfo("文件变动元信息JSON文件解析错误"), getErrorContext());
   }
   QStringList needCopies;
   QJsonObject rootObj = doc.object();
   QJsonArray itemsVariant = rootObj.take("delete").toArray();
   QJsonArray::const_iterator it = itemsVariant.constBegin();
   QJsonArray::const_iterator endMarker = itemsVariant.constEnd();
   while(it != endMarker){
      needCopies.append((*it).toString());
      m_context->deleteFiles.append((*it).toString());
      it++;
   }
   itemsVariant = rootObj.take("modify").toArray();
   it = itemsVariant.constBegin();
   endMarker = itemsVariant.constEnd();
   while(it != endMarker){
      needCopies.append((*it).toString());
      m_context->modifyFiles.append((*it).toString());
      it++;
   }
   itemsVariant = rootObj.take("senchaChangedProjects").toArray();
   it = itemsVariant.constBegin();
   endMarker = itemsVariant.constEnd();
   while(it != endMarker){
      m_context->senchaChangedProjects.append((*it).toString());
      it++;
   }
   if(needCopies.isEmpty()){
      return;
   }
   QString backupDir(getBackupDir()+QString("/%1_%2").arg(m_context->fromVersion, m_context->toVersion));
   m_context->backupDir = backupDir;
   if(!Filesystem::dirExist(backupDir)){
      Filesystem::createPath(backupDir);
   }
   m_context->response.setDataItem("step", STEP_BACKUP_FILES);
   m_context->response.setStatus(true);
   m_context->response.setDataItem("msg", "正在复制PHP相关文件");
   writeInterResponse(m_context->request, m_context->response);
   for(int i = 0; i < needCopies.size(); i++){
      Filesystem::copyFile(m_deployDir+'/'+needCopies[i], backupDir+'/'+needCopies[i]);
   }
   m_context->response.setDataItem("step", STEP_BACKUP_FILES);
   m_context->response.setStatus(true);
   m_context->response.setDataItem("msg", "正在复制JS程序文件");
   writeInterResponse(m_context->request, m_context->response);
   //复制js部分
   Filesystem::traverseFs(m_deployDir+"/PlatformJs", 0, [&](QFileInfo file, int){
      Filesystem::copyFile(file.absoluteFilePath(), file.absoluteFilePath().replace(m_deployDir, backupDir));
   });
}

void UpgradeDeployWrapper::upgradeFiles()
{
   m_step = STEP_BACKUP_FILES;
   m_context->response.setDataItem("step", STEP_UPGRADE_FILES);
   m_context->response.setStatus(true);
   m_context->response.setDataItem("msg", "正在升级程序文件");
   writeInterResponse(m_context->request, m_context->response);
   QStringList &modifyFiles = m_context->modifyFiles;
   for(int i = 0; i < modifyFiles.size(); i++){
      Filesystem::copyFile(m_context->upgradeDir+'/'+modifyFiles[i], m_deployDir+'/'+modifyFiles[i], true);
   }
   QStringList &deleteFiles = m_context->deleteFiles;
   for(int i = 0; i < deleteFiles.size(); i++){
      Filesystem::deleteFile(deleteFiles[i]);
   }
   QString relativeDir("/PlatformJs/build/production");
   QString senchaProjectDir = m_deployDir+relativeDir;
   if(!m_context->senchaChangedProjects.isEmpty()){
      for(QString projectName : m_context->senchaChangedProjects){
         if(Filesystem::dirExist(senchaProjectDir+'/'+projectName)){
            Filesystem::deleteDirRecusive(senchaProjectDir+'/'+projectName);
         }
         Filesystem::createPath(senchaProjectDir+'/'+projectName);
         Filesystem::copyDir(m_context->upgradeDir+relativeDir+'/'+projectName, senchaProjectDir);
      }
   }
}

void UpgradeDeployWrapper::backupDatabase()
{
   m_step = STEP_BACKUP_DB;
   m_context->response.setDataItem("step", STEP_BACKUP_DB);
   m_context->response.setStatus(true);
   m_context->response.setDataItem("msg", "正在备份数据库");
   writeInterResponse(m_context->request, m_context->response);
   QString metaFilename = m_context->upgradeDir+'/'+QString(ZHUCHAO_UPGRADE_DB_META_NAME_TPL)
         .arg(m_context->fromVersion, m_context->toVersion);
   if(!Filesystem::fileExist(metaFilename)){
      return;
   }
   QJsonParseError parserError;
   QJsonDocument doc = QJsonDocument::fromJson(Filesystem::fileGetContents(metaFilename), &parserError);
   if(QJsonParseError::NoError != parserError.error){
      clearState();
      throw_exception(ErrorInfo("文件变动元信息JSON文件解析错误"), getErrorContext());
   }
   QStringList needCopies;
   QJsonObject rootObj = doc.object();
   QJsonArray itemsVariant = rootObj.take("delete").toArray();
   QJsonArray::const_iterator it = itemsVariant.constBegin();
   QJsonArray::const_iterator endMarker = itemsVariant.constEnd();
   while(it != endMarker){
      needCopies.append((*it).toString());
      it++;
   }
   itemsVariant = rootObj.take("modify").toArray();
   it = itemsVariant.constBegin();
   endMarker = itemsVariant.constEnd();
   while(it != endMarker){
      needCopies.append((*it).toString());
      it++;
   }
   QString dbBackupDir = m_context->backupDir+"/dbbackup";
   if(!Filesystem::dirExist(dbBackupDir)){
      Filesystem::createPath(dbBackupDir);
   }
   for(int i = 0; i < needCopies.size(); i++){
      m_context->response.setDataItem("msg", QString("正在备份数据表 %1").arg(needCopies[i]));
      writeInterResponse(m_context->request, m_context->response);
      dump_mysql_table(m_context->dbUser, m_context->dbPassword, ZHUCHAO_DB_NAME, needCopies[i], dbBackupDir);
   }
}

bool UpgradeDeployWrapper::runUpgradeScript()
{
   m_step = STEP_RUN_UPGRADE_SCRIPT;
   m_context->response.setDataItem("step", STEP_RUN_UPGRADE_SCRIPT);
   m_context->response.setStatus(true);
   m_context->response.setDataItem("msg", "正在运行升级脚本");
   writeInterResponse(m_context->request, m_context->response);
   QString upgradeScriptFilename = m_context->upgradeDir+'/'+QString(ZHUCHAO_UPGRADE_SCRIPT_NAME_TPL)
         .arg(m_context->fromVersion, m_context->toVersion);
   if(!Filesystem::fileExist(upgradeScriptFilename)){
      return true;
   }
   QSharedPointer<UpgradeEnvEngine> scriptEngine = getUpgradeScriptEngine();
   return scriptEngine->exec(upgradeScriptFilename);
}

void UpgradeDeployWrapper::upgradeComplete()
{
   m_step= STEP_FINISH;
   //更新版本文件
   QString versionFilename = m_deployDir + "/VERSION";
   Filesystem::filePutContents(versionFilename, m_context->toVersion);
   //设置权限
   Filesystem::traverseFs(m_deployDir, 0, [&](QFileInfo &fileinfo, int){
      Filesystem::chown(fileinfo.absoluteFilePath(), m_userId, m_groupId);
   });
   clearState();
}

void UpgradeDeployWrapper::clearState()
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

void UpgradeDeployWrapper::unzipPkg(const QString &pkgFilename)
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
      throw ErrorInfo(process.errorString());
   }
}

QSharedPointer<DownloadClientWrapper> UpgradeDeployWrapper::getDownloadClient(const QString &host, quint16 port)
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
         m_context->upgradeStatus = false;
         m_context->upgradeErrorString = errorMsg;
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

QSharedPointer<UpgradeEnvEngine> UpgradeDeployWrapper::getUpgradeScriptEngine()
{
   if(m_upgradeScriptEngine.isNull()){
      m_upgradeScriptEngine.reset(new UpgradeEnvEngine(m_context->dbHost, m_context->dbUser, 
                                                       m_context->dbPassword, ZHUCHAO_DB_NAME));
      m_upgradeScriptEngine->setMetaInfo("ConfigBaseDir", m_deployDir+"/Config");
      m_upgradeScriptEngine->initUpgradeEnv();
      QJSEngine &engine = m_upgradeScriptEngine->getJsEngine();
      QJSValue env = engine.newObject();
      env.setProperty("deployDir", m_deployDir);
      env.setProperty("backupDir", m_context->backupDir);
      env.setProperty("upgradeDir", m_context->upgradeDir);
      m_upgradeScriptEngine->registerContextObject("UpgradeMeta", env, true);
      connect(m_upgradeScriptEngine.data(),& UpgradeEnvEngine::excpetionSignal, [&](ErrorInfo errorInfo){
         m_context->upgradeStatus = false;
         m_context->upgradeErrorString = errorInfo.toString();
      });
      connect(m_upgradeScriptEngine.data(),& UpgradeEnvEngine::logMsgSignal, [&](const QString &msg){
         m_context->response.setDataItem("msg", msg);
         m_context->response.setDataItem("step", STEP_RUN_UPGRADE_SCRIPT);
         writeInterResponse(m_context->request, m_context->response);
      });
   }
   return m_upgradeScriptEngine;
}

QString UpgradeDeployWrapper::getBackupDir()
{
   return StdDir::getBaseDataDir()+"/backup";
}

QString UpgradeDeployWrapper::getUpgradeTmpDir()
{
   return StdDir::getBaseDataDir()+"/upgrade/tmp";
}

void UpgradeDeployWrapper::notifySocketDisconnect(QTcpSocket*)
{
   clearState();
}

}//zhuchao
}//lxservice
