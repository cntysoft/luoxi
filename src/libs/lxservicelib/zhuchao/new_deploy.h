#ifndef LUOXI_SERVICE_ZHUCHAO_NEW_DEPLOY_H
#define LUOXI_SERVICE_ZHUCHAO_NEW_DEPLOY_H

#include <QString>
#include <QSharedPointer>
#include <QTcpSocket>

#include "lxservicelib/global_defs.h"
#include "corelib/network/rpc/abstract_service.h"
#include "corelib/network/rpc/invoke_meta.h"
#include "corelib/network/rpc/service_provider.h"
#include "corelib/db/engine/engine.h"
#include "lxservicelib/common/download_client.h"

namespace lxservice{
namespace zhuchao{

LUOXI_USING_SERVICE_NAMESPACES

using lxservice::common::DownloadClient;
using SqlEngine = sn::corelib::db::engine::Engine;

class LUOXI_SERVICE_EXPORT NewDeployWrapper : public AbstractService
{
   Q_OBJECT
public:
   struct NewDeployContext
   {
      QString targetVersion;
      bool withoutDb;
      QString pkgFilename;
      ServiceInvokeRequest request;
      ServiceInvokeResponse response;
      QString dbName;
      QString deployDir;
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
   const static int STEP_CLEANUP = 6;
   const static int STEP_FINISH = 7;
   const static int STEP_ERROR = 8;
   
   const static QString ZHUCHAO_PKG_NAME_TPL;
   const static QString DEPLOY_DIR_TPL;
   const static QString DB_NAME_TPL;
   const static QString DB_SQL_FILENAME_TPL;
public:
   NewDeployWrapper(ServiceProvider& provider);
   Q_INVOKABLE ServiceInvokeResponse deploy(const ServiceInvokeRequest &request);
protected:
   void downloadZhuChaoPkg(const QString &filename);
   void clearState();
   QString getDeployTmpDir();
   void unzipPkg(const QString &pkgFilename);
   void copyFilesToDeployDir();
   void createDatabase();
   void cleanupTmpFiles();
protected:
   QSharedPointer<DownloadClient> getDownloadClient(const QString &host, quint16 port);
   //protected:
   //   virtual void notifySocketDisconnect(QTcpSocket *socket);
protected:
   bool m_isInAction = false;
   QSharedPointer<NewDeployContext> m_context;
   int m_step = STEP_PREPARE;
   QString m_deployBaseDir;
   QSharedPointer<DownloadClient> m_downloadClient;
   QSharedPointer<SqlEngine> m_sqlEngine;
   int m_userId;
   int m_groupId;
   QString m_dbHost;
   QString m_dbUser;
   QString m_dbPassword;
};

}//zhuchao
}//lxservice

#endif // LUOXI_SERVICE_ZHUCHAO_NEW_DEPLOY_H
