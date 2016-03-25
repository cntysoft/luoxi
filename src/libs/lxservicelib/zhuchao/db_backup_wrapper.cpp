#include "db_backup.h"

#include <QProcess>
#include <QProcessEnvironment>
#include <QVariant>
#include <cstdlib>
#include <QDate>
#include <QStringList>
#include <QDebug>

#include "corelib/kernel/settings.h"
#include "corelib/kernel/application.h"
#include "corelib/global/common_funcs.h"
#include "corelib/kernel/errorinfo.h"
#include "corelib/io/filesystem.h"
#include "corelib/db/engine/engine.h"
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
using sn::corelib::dump_mysql_table;
using sn::corelib::db::engine::Engine;

const QString DbBackupWrapper::COMPRESS_FILENAME_TPL = "zhuchao_db_%1_%2.tar.gz";
const QString DbBackupWrapper::ZHUCHAO_DB_SAVED_DIR = "/zhuchao/dbbackup";

DbBackupWrapper::DbBackupWrapper(ServiceProvider& provider)
   :AbstractService(provider)
{
   Settings& settings = Application::instance()->getSettings();
   m_deployDir = settings.getValue("deployBaseDir", LUOXI_CFG_GROUP_GLOBAL, "/srv/www").toString() + "/zhuchao";
   m_dbHost = settings.getValue("dbHost", LUOXI_CFG_GROUP_GLOBAL).toString();
   m_dbUser = settings.getValue("dbUser", LUOXI_CFG_GROUP_GLOBAL).toString();
   m_dbPassword = settings.getValue("dbPassword", LUOXI_CFG_GROUP_GLOBAL).toString();
   m_dbMetadata.reset(new DbMetaData(new Engine(Engine::QMYSQL, {
                                                   {"host", m_dbHost},
                                                   {"username", m_dbUser},
                                                   {"password", m_dbPassword}
                                                })));
}

ServiceInvokeResponse DbBackupWrapper::backup(const ServiceInvokeRequest &request)
{
   if(m_step != STEP_PREPARE){
      throw_exception(ErrorInfo("状态错误"), getErrorContext());
   }
   m_step = STEP_INIT_CONTEXT;
   m_context.reset(new DbBackupContext);
   m_context->backupStatus = true;
   m_context->request = request;
   ServiceInvokeResponse response("ZhuChao/DbBackup/backup", true);
   response.setSerial(request.getSerial());
   m_context->response = response;
   generateSql();
   if(!m_context->backupStatus){
      goto process_error;
   }
   compressSqlFiles();
   if(!m_context->backupStatus){
      goto process_error;
   }
   uploadSqlCompressFile();
   if(!m_context->backupStatus){
      goto process_error;
   }
   m_step = STEP_FINISH;
   response.setStatus(true);
   response.setDataItem("msg", "数据库备份完成");
   response.setDataItem("step", STEP_FINISH);
   clearState();
   return response;
process_error:
   response.setDataItem("msg", m_context->backupErrorString);
   writeInterResponse(request, response);
   response.setStatus(false);
   response.setDataItem("step", STEP_ERROR);
   response.setError({-1, "数据库备份失败"});
   clearState();
   return response;
}

void DbBackupWrapper::generateSql()
{
   m_step = STEP_GENERATE_SQL;
   m_context->response.setDataItem("step", STEP_GENERATE_SQL);
   m_context->response.setDataItem("msg", "正在获取数据表数据");
   writeInterResponse(m_context->request, m_context->response);
   QStringList tableNames = m_dbMetadata->getTableNames("zhuchao");
   QString versionFilename = m_deployDir + "/VERSION";
   QString version;
   if(Filesystem::fileExist(versionFilename)){
      version = Filesystem::fileGetContents(versionFilename);
   }else{
      version = "unknow";
   }
   QString dbBackupFilename = COMPRESS_FILENAME_TPL.arg(version, QDate::currentDate().toString("yyyyMMdd"));
   m_context->dbBackupFilename = dbBackupFilename;
   QString savedDir = getBackupTmpDir()+'/'+dbBackupFilename.mid(0, dbBackupFilename.size() - 7);
   if(Filesystem::dirExist(savedDir)){
      Filesystem::deleteDirRecusive(savedDir);
   }
   try{
      for(const QString &tablename : tableNames){
         m_context->response.setDataItem("msg", QString("正在备份数据表%1").arg(tablename));
         writeInterResponse(m_context->request, m_context->response);
         dump_mysql_table(m_dbUser, m_dbPassword, "zhuchao", tablename, savedDir);
      }
   }catch(ErrorInfo e){
      m_context->backupStatus = false;
      m_context->backupErrorString = e.toString();
      return;
   }
}

void DbBackupWrapper::compressSqlFiles()
{
   m_step = STEP_COMPRESS;
   m_context->response.setDataItem("step", STEP_COMPRESS);
   m_context->response.setDataItem("msg", "正在压缩sql文件");
   writeInterResponse(m_context->request, m_context->response);
   QProcess process;
   process.setWorkingDirectory(getBackupTmpDir());
   QStringList args;
   args << "-czvf" << getBackupTmpDir() +'/'+m_context->dbBackupFilename << m_context->dbBackupFilename.mid(0, m_context->dbBackupFilename.size() - 7);
   process.start("tar", args);
   bool status = process.waitForFinished(-1);
   if(!status || process.exitCode() != 0){
      m_context->backupStatus = false;
      m_context->backupErrorString = process.readAllStandardError();
   }
}

void DbBackupWrapper::uploadSqlCompressFile()
{
   //获取相关的配置信息
   Settings& settings = Application::instance()->getSettings();
   QSharedPointer<UploadClientWrapper> uploader = getUploadClient(settings.getValue("metaserverHost").toString(), 
                                                                 settings.getValue("metaserverPort").toInt());
   uploader->changUploadDir(ZHUCHAO_DB_SAVED_DIR);
   uploader->upload(getBackupTmpDir() +'/'+m_context->dbBackupFilename);
   m_eventLoop.exec();
}

QString DbBackupWrapper::getBackupTmpDir()
{
   return StdDir::getBaseDataDir()+"/backup/tmp";
}

void DbBackupWrapper::cleanupTmpFiles()
{
   m_step = STEP_CLEANUP;
   m_context->response.setDataItem("step", STEP_CLEANUP);
   m_context->response.setDataItem("msg", "正在删除临时文件");
   writeInterResponse(m_context->request, m_context->response);
   if(Filesystem::dirExist(getBackupTmpDir())){
      Filesystem::deleteDir(getBackupTmpDir());
   }
}

QSharedPointer<UploadClientWrapper> DbBackupWrapper::getUploadClient(const QString &host, quint16 port)
{
   if(m_uploadClient.isNull()){
      m_uploadClient.reset(new UploadClientWrapper(getServiceInvoker(host, port)));
      connect(m_uploadClient.data(), &UploadClientWrapper::startUpload, this, [&](){
         m_context->response.setDataItem("step", STEP_UPLOAD);
         m_context->response.setDataItem("msg", "开始上传SQL压缩文件");
         writeInterResponse(m_context->request, m_context->response);
      });
      QObject::connect(m_uploadClient.data(), &UploadClientWrapper::uploadError, this, [&](int, const QString &errorMsg){
         m_eventLoop.exit();
         m_isInAction = false;
         m_step = STEP_PREPARE;
         m_context->backupStatus = false;
         m_context->backupErrorString = errorMsg;
      });
      connect(m_uploadClient.data(), &UploadClientWrapper::uploadSuccess, this, [&](){
         m_context->response.setDataItem("step", STEP_UPLOAD);
         m_context->response.setStatus(true);
         m_context->response.setDataItem("msg", "上传SQL压缩文件完成");
         writeInterResponse(m_context->request, m_context->response);
         m_eventLoop.exit();
      });
   }
   return m_uploadClient;
}

void DbBackupWrapper::clearState()
{
   m_context.clear();
   m_step = STEP_PREPARE;
   QString zipFilename = getBackupTmpDir() +'/'+m_context->dbBackupFilename;
   QString sqlDir = m_context->dbBackupFilename.mid(0, m_context->dbBackupFilename.size() - 7);
   if(Filesystem::fileExist(zipFilename)){
      Filesystem::deleteFile(zipFilename);
   }
   if(Filesystem::dirExist(sqlDir)){
      Filesystem::deleteDirRecusive(sqlDir);
   }
   //清除残余文件
   if(!m_serviceInvoker.isNull()){
      m_serviceInvoker->disconnectFromServer();
   }
}


}//zhuchao
}//lxservice