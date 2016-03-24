#ifndef LUOXI_SERVICE_ZHUCHAO_DB_BACKUP_H
#define LUOXI_SERVICE_ZHUCHAO_DB_BACKUP_H

#include <QString>
#include <QSharedPointer>
#include <QTcpSocket>

#include "lxservicelib/global_defs.h"
#include "corelib/network/rpc/abstract_service.h"
#include "corelib/network/rpc/invoke_meta.h"
#include "corelib/network/rpc/service_provider.h"
#include "corelib/db/metadata/metadata.h"

namespace lxservice{
namespace zhuchao{

LUOXI_USING_SERVICE_NAMESPACES

using DbMetaData = sn::corelib::db::metadata::Metadata;

class LUOXI_SERVICE_EXPORT DbBackupWrapper : public AbstractService
{
   Q_OBJECT
public:
   struct DbBackupContext
   {
      ServiceInvokeRequest request;
      ServiceInvokeResponse response;
      QString dbBackupFilename;
      bool backupStatus;
      QString backupErrorString;
   };
   const static int STEP_PREPARE = -1;
   const static int STEP_INIT_CONTEXT = 0;
   const static int STEP_GENERATE_SQL = 1;
   const static int STEP_COMPRESS = 2;
   const static int STEP_UPLOAD = 3;
   const static int STEP_FINISH = 4;
   const static int STEP_CLEANUP = 5;
   const static int STEP_ERROR = 6;
   
   const static QString COMPRESS_FILENAME_TPL;
public:
   DbBackupWrapper(ServiceProvider& provider);
   Q_INVOKABLE ServiceInvokeResponse backup(const ServiceInvokeRequest &request);
protected:
   void clearState();
   void generateSql();
   void compressSqlFiles();
   QString getBackupTmpDir();
   void cleanupTmpFiles();
protected:
   //protected:
   //   virtual void notifySocketDisconnect(QTcpSocket *socket);
protected:
   bool m_isInAction = false;
   QSharedPointer<DbBackupContext> m_context;
   QSharedPointer<DbMetaData> m_dbMetadata;
   int m_step = STEP_PREPARE;
   QString m_deployDir;
   QString m_dbHost;
   QString m_dbUser;
   QString m_dbPassword;
};

}//zhuchao
}//lxservice

#endif // LUOXI_SERVICE_ZHUCHAO_DB_BACKUP_H
