#include "instance_deploy.h"

#include "corelib/kernel/settings.h"
#include "corelib/kernel/application.h"
#include "corelib/global/common_funcs.h"
#include "corelib/kernel/errorinfo.h"
#include "corelib/io/filesystem.h"
#include "lxlib/global/const.h"
#include "lxlib/kernel/stddir.h"

#include <QDebug>

namespace lxservice{
namespace kelecloud{

using sn::corelib::Settings;
using sn::corelib::Application;
using sn::corelib::throw_exception;
using sn::corelib::ErrorInfo;
using sn::corelib::Filesystem;
using lxlib::kernel::StdDir;

const QString InstanceDeployWrapper::KELESHOP_PKG_NAME_TPL = "keleshop_%1.tar.gz";

InstanceDeployWrapper::InstanceDeployWrapper(ServiceProvider& provider)
   :AbstractService(provider)
{
   Settings& settings = Application::instance()->getSettings();
   m_deployBaseDir = settings.getValue("deployBaseDir", LUOXI_CFG_GROUP_GLOBAL, "/srv/www").toString();
   m_userId = settings.getValue("keleshopDeployUserId", LUOXI_CFG_GROUP_GLOBAL, 30).toInt();
   m_groupId = settings.getValue("keleshopDeployGroupId", LUOXI_CFG_GROUP_GLOBAL, 8).toInt();
}

ServiceInvokeResponse InstanceDeployWrapper::deploy(const ServiceInvokeRequest &request)
{
   if(m_step != STEP_PREPARE){
      throw_exception(ErrorInfo("状态错误"), getErrorContext());
   }
   m_context.reset(new InstanceDeployContext);
   QMap<QString, QVariant> args = request.getArgs();
   checkRequireFields(args, {"currentVersion", "instanceKey"});
   QString softwareRepoDir = StdDir::getSoftwareRepoDir();
   m_context->deployStatus = true;
   m_context->currentVersion = args.value("currentVersion").toString();
   //获取数据库信息
   Settings& settings = Application::instance()->getSettings();
   m_context->dbHost = settings.getValue("dbHost", LUOXI_CFG_GROUP_GLOBAL).toString();
   m_context->dbUser = settings.getValue("dbUser", LUOXI_CFG_GROUP_GLOBAL).toString();
   m_context->dbPassword = settings.getValue("dbPassword", LUOXI_CFG_GROUP_GLOBAL).toString();
   QString baseFilename = QString(KELESHOP_PKG_NAME_TPL).arg(m_context->currentVersion);
   QString upgradePkgFilename = softwareRepoDir + '/' + baseFilename;
   m_context->pkgFilename = upgradePkgFilename;
   m_context->request = request;
   ServiceInvokeResponse response("KeleCloud/InstanceDeploy/deploy", true);
   response.setSerial(request.getSerial());
   m_context->response = response;
   //   if(!Filesystem::fileExist(upgradePkgFilename)){
   //      //下载升级文件到本地
   //      downloadUpgradePkg(baseFilename);
   //   }
   downloadUpgradePkg(baseFilename);
   if(!m_context->deployStatus){
      response.setDataItem("step", STEP_DOWNLOAD_PKG);
      response.setDataItem("msg", m_context->deployErrorString);
      writeInterResponse(request, response);
      response.setStatus(false);
      response.setDataItem("step", STEP_ERROR);
      response.setError({-1, "创建站点失败"});
      //      clearState();
      return response;
   }
   m_context->response.setDataItem("step", STEP_EXTRA_PKG);
   m_context->response.setDataItem("msg", "正在解压升级压缩包");
   m_context->response.setStatus(true);
   writeInterResponse(m_context->request, m_context->response);
   //      unzipPkg(m_context->pkgFilename);
   //   backupScriptFiles();
   //   upgradeFiles();
   //   runUpgradeScript();
   //   if(!m_context->upgradeStatus){
   //      response.setDataItem("msg", m_context->upgradeErrorString);
   //      writeInterResponse(request, response);
   //      response.setStatus(false);
   //      response.setDataItem("step", STEP_ERROR);
   //      response.setError({-1, "升级失败"});
   //      clearState();
   //      return response;
   //   }
   //   upgradeComplete();
   //   response.setSerial(request.getSerial());
   //   response.setIsFinal(true);
   //   response.setStatus(true);
   //   response.setDataItem("msg", "升级完成");
   //   response.setDataItem("step", STEP_FINISH);
   //return response;
}

void InstanceDeployWrapper::downloadUpgradePkg(const QString &filename)
{
   //获取相关的配置信息
   Settings& settings = Application::instance()->getSettings();
   QSharedPointer<DownloadClient> downloader = getDownloadClient(settings.getValue("metaserverHost").toString(), 
                                                                 settings.getValue("metaserverPort").toInt());
   downloader->download(filename);
   m_eventLoop.exec();
}

QSharedPointer<DownloadClient> InstanceDeployWrapper::getDownloadClient(const QString &host, quint16 port)
{
   if(m_downloadClient.isNull()){
      m_downloadClient.reset(new DownloadClient(getServiceInvoker(host, port)));
      connect(m_downloadClient.data(), &DownloadClient::beginDownload, this, [&](){
         m_context->response.setDataItem("step", STEP_DOWNLOAD_PKG);
         m_context->response.setDataItem("msg", "开始下载软件包");
         writeInterResponse(m_context->request, m_context->response);
      });
      QObject::connect(m_downloadClient.data(), &DownloadClient::downloadError, this, [&](int, const QString &errorMsg){
         m_eventLoop.exit();
         m_isInAction = false;
         m_step = STEP_PREPARE;
         m_context->deployStatus = false;
         m_context->deployErrorString = errorMsg;
      });
      connect(m_downloadClient.data(), &DownloadClient::downloadComplete, this, [&](){
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

}//kelecloud
}//lxservice