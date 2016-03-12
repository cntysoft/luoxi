#ifndef LUOXI_SERVICE_KELECLOUD_INSTANCE_DEPLOY_H
#define LUOXI_SERVICE_KELECLOUD_INSTANCE_DEPLOY_H

#include <QString>
#include <QSharedPointer>
#include <QTcpSocket>

#include "lxservicelib/global_defs.h"
#include "corelib/network/rpc/abstract_service.h"
#include "corelib/network/rpc/invoke_meta.h"
#include "corelib/network/rpc/service_provider.h"
#include "corelib/db/engine/engine.h"
#include "corelib/cloud/aliyun/dns/dns_resolve.h"
#include "corelib/cloud/aliyun/dns/dns_api_caller.h"
#include "lxservicelib/common/download_client.h"

namespace lxservice{
namespace kelecloud{

LUOXI_USING_SERVICE_NAMESPACES

using lxservice::common::DownloadClient;
using SqlEngine = sn::corelib::db::engine::Engine;
using sn::corelib::cloud::aliyun::dns::DnsResolve;
using sn::corelib::cloud::aliyun::dns::DnsApiCaller;

class LUOXI_SERVICE_EXPORT InstanceDeployWrapper : public AbstractService
{
   Q_OBJECT
public:
   struct InstanceDeployContext
   {
      QString currentVersion;
      QString instanceKey;
      QString pkgFilename;
      ServiceInvokeRequest request;
      ServiceInvokeResponse response;
      
      QString dbName;
      QString deployDir;
      QString nginxCfgFilename;
      bool deployStatus;
      QString deployErrorString;
   };
   const static int STEP_PREPARE = -1;
   const static int STEP_INIT_CONTEXT = 0;
   const static int STEP_DOWNLOAD_PKG = 1;
   const static int STEP_DOWNLOAD_COMPLETE = 2;
   const static int STEP_EXTRA_PKG = 3;
   const static int STEP_COPY_FILES = 4;
   const static int STEP_CREATE_DB = 5;
   const static int STEP_ADD_DOMAIN_RECORD = 6;
   const static int STEP_RESTART_SERVER = 7;
   const static int STEP_CLEANUP = 8;
   const static int STEP_FINISH = 9;
   const static int STEP_ERROR = 10;
   
   const static QString KELESHOP_PKG_NAME_TPL;
   const static QString DEPLOY_DIR_TPL;
   const static QString NGINX_CFG_FILENAME_TPL;
   const static QString DB_NAME_TPL;
   const static QString DB_SQL_FILENAME_TPL;
   const static QString KELESHOP_DOMAIN;
   const static QString INSTANCE_SUBDOMAIN_TPL;
public:
   InstanceDeployWrapper(ServiceProvider& provider);
   Q_INVOKABLE ServiceInvokeResponse deploy(const ServiceInvokeRequest &request);
protected:
   void downloadUpgradePkg(const QString &filename);
   void clearState();
   QString getDeployTmpDir();
   void unzipPkg(const QString &pkgFilename);
   void copyFilesToDeployDir();
   void createDatabase();
   void addDomainRecord();
protected:
   QSharedPointer<DownloadClient> getDownloadClient(const QString &host, quint16 port);
   QSharedPointer<DnsResolve> getDnsResolver();
   //protected:
   //   virtual void notifySocketDisconnect(QTcpSocket *socket);
protected:
   bool m_isInAction = false;
   QSharedPointer<InstanceDeployContext> m_context;
   int m_step = STEP_PREPARE;
   QString m_deployBaseDir;
   QString m_nginxConfDir;
   QString m_deployServerAddress;
   QSharedPointer<DownloadClient> m_downloadClient;
   QSharedPointer<SqlEngine> m_sqlEngine;
   QSharedPointer<DnsResolve> m_dnsResolver;
   int m_userId;
   int m_groupId;
   QString m_dbHost;
   QString m_dbUser;
   QString m_dbPassword;
};

}//kelecloud
}//lxservice

#endif // LUOXI_SERVICE_KELECLOUD_INSTANCE_DEPLOY_H
