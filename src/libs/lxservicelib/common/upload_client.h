#ifndef LUOXI_SERVICE_COMMON_UPLOAD_CLIENT_H
#define LUOXI_SERVICE_COMMON_UPLOAD_CLIENT_H

#include "lxservicelib/global_defs.h"

#include "corelib/network/rpc/abstract_service.h"
#include "corelib/network/rpc/invoke_meta.h"
#include "corelib/network/rpc/service_invoker.h"

#include <QFile>

LUOXI_USING_SERVICE_NAMESPACES

namespace lxservice{
namespace common{

using sn::corelib::network::ServiceInvoker;

class LUOXI_SERVICE_EXPORT UploadClientWrapper : public QObject
{
   Q_OBJECT
public:
   const static int UPLOAD_STEP_PREPARE = 0;
   const static int UPLOAD_STEP_META_RECEIVED = 1;
   const static int UPLOAD_STEP_START = 2;
   const static int UPLOAD_STEP_PROCESS = 3;
   const static int UPLOAD_STEP_FINISH = 4;
   struct UploadContext
   {
      QString filename;
      QFile *targetFile = nullptr;
      int fileSize = 0;
      int uploaded = 0;
   };
   friend void upload_client_init_upload_handler(const ServiceInvokeResponse &response, void* args);
   friend void upload_client_upload_cycle_handler(const ServiceInvokeResponse &response, void* args);
   friend void upload_client_notify_complete_handler(const ServiceInvokeResponse &response, void* args);
public:
   UploadClientWrapper(QSharedPointer<ServiceInvoker> serviceInvoker);
   void upload(const QString &filename);
   void clearState();
   UploadClientWrapper& changUploadDir(const QString &uploadDir);
   QString getUploadDir();
   ~UploadClientWrapper();
protected:
   void emitUploadError(int errorCode, const QString &errorMsg);
   void emitUploadProgress(int uploaded, int totalSize, const QString &filename);
   void emitUploadSuccess();
   void beginUploadData();
   void uploadCycle();
protected slots:
   void connectToServerHandler();
   void connectErrorHandler();
signals:
   void startUpload();
   void uploadError(int errorCode, const QString &errorMsg);
   void uploadProgress(int uploaded, int totalSize, const QString &filename);
   void uploadSuccess();
protected:
   QSharedPointer<ServiceInvoker> m_serviceInvoker;
   QString m_uploadDir;
   int m_cycleSize;
   int m_chunkSize;
   QSharedPointer<UploadContext> m_context;
   int m_step = UPLOAD_STEP_PREPARE;
   bool m_status;
};

}//common
}//lxservice

#endif // LUOXI_SERVICE_COMMON_UPLOAD_CLIENT_H
