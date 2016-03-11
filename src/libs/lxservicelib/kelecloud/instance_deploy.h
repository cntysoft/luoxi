#ifndef LUOXI_SERVICE_KELECLOUD_INSTANCE_DEPLOY_H
#define LUOXI_SERVICE_KELECLOUD_INSTANCE_DEPLOY_H

#include <QString>
#include <QSharedPointer>
#include <QTcpSocket>

#include "lxservicelib/global_defs.h"
#include "corelib/network/rpc/abstract_service.h"
#include "corelib/network/rpc/invoke_meta.h"
#include "corelib/network/rpc/service_provider.h"

#include "lxservicelib/common/download_client.h"

namespace lxservice{
namespace kelecloud{

LUOXI_USING_SERVICE_NAMESPACES

using lxservice::common::DownloadClient;

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
      QString dbHost;
      QString dbUser;
      QString dbPassword;
      QString dbName;
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
public:
   InstanceDeployWrapper(ServiceProvider& provider);
   Q_INVOKABLE ServiceInvokeResponse deploy(const ServiceInvokeRequest &request);
protected:
   void downloadUpgradePkg(const QString &filename);
   QSharedPointer<DownloadClient> getDownloadClient(const QString &host, quint16 port);
//   void clearState();
//protected:
//   virtual void notifySocketDisconnect(QTcpSocket *socket);
protected:
   bool m_isInAction = false;
   QSharedPointer<InstanceDeployContext> m_context;
   int m_step = STEP_PREPARE;
   QString m_deployBaseDir;
   QSharedPointer<DownloadClient> m_downloadClient;
   int m_userId;
   int m_groupId;
};

}//kelecloud
}//lxservice

#endif // LUOXI_SERVICE_KELECLOUD_INSTANCE_DEPLOY_H
