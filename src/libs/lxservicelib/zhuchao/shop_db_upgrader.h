#ifndef LUOXI_SERVICE_ZHUCHAO_SHOP_DB_UPGRADER_H
#define LUOXI_SERVICE_ZHUCHAO_SHOP_DB_UPGRADER_H

#include <QString>
#include <QSharedPointer>
#include <QTcpSocket>
#include <QStringList>

#include "lxservicelib/global_defs.h"
#include "corelib/network/rpc/abstract_service.h"
#include "corelib/network/rpc/invoke_meta.h"
#include "corelib/network/rpc/service_provider.h"

#include "lxservicelib/common/download_client.h"
#include "corelib/upgrade/upgradeenv.h"
#include "corelib/db/engine/engine.h"

namespace lxservice{
namespace zhuchao{

LUOXI_USING_SERVICE_NAMESPACES

using lxservice::common::DownloadClientWrapper;
using UpgradeEnvEngine = sn::corelib::upgrade::UpgradeEnv;
using DbEngine = sn::corelib::db::engine::Engine;

class LUOXI_SERVICE_EXPORT ShopDbUpgraderWrapper : public AbstractService
{
   Q_OBJECT
public:
   struct UpgradeContext
   {
      bool forceDownloadPkg;
      QString fromVersion;
      QString toVersion;
      QString pkgFilename;
      ServiceInvokeRequest request;
      ServiceInvokeResponse *response;
      QString upgradeDir;
      QString backupDir;
      QString upgradeScript;
      bool upgradeStatus;
      QString upgradeErrorString;
   };
   const static int STEP_PREPARE = -1;
   const static int STEP_INIT_CONTEXT = 0;
   const static int STEP_DOWNLOAD_PKG = 1;
   const static int STEP_DOWNLOAD_COMPLETE = 2;
   const static int STEP_EXTRA_PKG = 3;
   const static int STEP_BACKUP_DB = 4;
   const static int STEP_RUN_UPGRADE_SCRIPT = 5;
   const static int STEP_CLEANUP = 6;
   const static int STEP_FINISH = 7;
   const static int STEP_ERROR = 8;
   
   const static QString ZHUCHAO_UPGRADE_PKG_NAME_TPL;
   const static QString ZHUCHAO_UPGRADE_SCRIPT_NAME_TPL;
   const static QString ZHUCHAO_DB_NAME_PREFIX;
public:
   ShopDbUpgraderWrapper(ServiceProvider& provider);
   Q_INVOKABLE ServiceInvokeResponse upgrade(const ServiceInvokeRequest &request);
protected:
   void downloadUpgradePkg(const QString &filename);
   void backupDatabase();
   bool runUpgradeScript();
   void upgradeComplete();
   void checkVersion();
   QSharedPointer<DownloadClientWrapper> getDownloadClient(const QString &host, quint16 port);
   void clearState();
   QString getBackupDir();
   QString getUpgradeTmpDir();
   void unzipPkg(const QString &pkgFilename);
   QSharedPointer<UpgradeEnvEngine> getUpgradeScriptEngine();
   void getShopDatabases(const QStringList &databases);
   void setupSuccessResponse(ServiceInvokeResponse &response);
   void setupFailureResponse(ServiceInvokeResponse &response);
protected:
   virtual void notifySocketDisconnect(QTcpSocket *socket);
protected:
   bool m_isInAction = false;
   QSharedPointer<UpgradeContext> m_context;
   int m_step = STEP_PREPARE;
   QSharedPointer<DownloadClientWrapper> m_downloadClient;
   QSharedPointer<UpgradeEnvEngine> m_upgradeScriptEngine;
   QSharedPointer<DbEngine> m_dbEngine;
   QString m_dbHost;
   QString m_dbUser;
   QString m_dbPassword;
};

}//zhuchao
}//lxservice

#endif // LUOXI_SERVICE_ZHUCHAO_SHOP_DB_UPGRADER_H
