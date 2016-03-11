#ifndef LUOXI_SERVICE_COMMON_DOWNLOAD_CLIENT_H
#define LUOXI_SERVICE_COMMON_DOWNLOAD_CLIENT_H

#include "lxservicelib/global_defs.h"

#include "corelib/network/rpc/abstract_service.h"
#include "corelib/network/rpc/invoke_meta.h"
#include "corelib/network/rpc/service_invoker.h"

#include <QFile>

LUOXI_USING_SERVICE_NAMESPACES

namespace lxservice{
namespace common{

using sn::corelib::network::ServiceInvoker;

class LUOXI_SERVICE_EXPORT DownloadClient : public QObject
{
   Q_OBJECT
public:
   const static int DOWNLOAD_STEP_PREPARE = 0;
   const static int DOWNLOAD_STEP_META_RECEIVED = 1;
   const static int DOWNLOAD_STEP_START = 2;
   const static int DOWNLOAD_STEP_PROCESS = 3;
   const static int DOWNLOAD_STEP_FINISH = 4;
   struct DownloadContext
   {
      QString filename;
      QFile *targetFile = nullptr;
      int fileSize = 0;
      int cycleSize = -1;
      int chunkSize = -1;
      int downloadPointer = -1;
   };
   const static QString CC_UPGRADE_PKG_NAME_TPL;
   friend void init_download_handler(const ServiceInvokeResponse &response, void* args);
   friend void download_cycle_handler(const ServiceInvokeResponse &response, void* args);
public:
   DownloadClient(QSharedPointer<ServiceInvoker> serviceInvoker);
   void download(const QString &filename);
   void emitDownloadError(int errorCode, const QString &errorMsg);
   void emitDownloadComplete();
   void clearState();
   ~DownloadClient();
protected:
   void beginRetrieveData();
   void downloadCycle();
protected slots:
   void connectToServerHandler();
signals:
   void beginDownload();
   void downloadError(int errorCode, const QString &errorMsg);
   void downloadComplete();
protected:
   QSharedPointer<DownloadContext> m_context;
   int m_step = DOWNLOAD_STEP_PREPARE;
   QSharedPointer<ServiceInvoker> m_serviceInvoker;
};

}//common
}//lxservice

#endif // LUOXI_SERVICE_COMMON_DOWNLOAD_CLIENT_H
