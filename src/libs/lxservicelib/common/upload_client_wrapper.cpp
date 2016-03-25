#include "upload_client.h"

#include <QVariant>
#include "corelib/io/filesystem.h"

namespace lxservice{
namespace common{

using sn::corelib::Filesystem;

void upload_client_init_upload_handler(const ServiceInvokeResponse &response, void* args)
{
   UploadClientWrapper *self = static_cast<UploadClientWrapper*>(args);
   if(!response.getStatus()){
      self->emitUploadError(response.getError().first, response.getError().second);
      self->clearState();
   }else{
      self->m_step = UploadClientWrapper::UPLOAD_STEP_META_RECEIVED;
      self->beginUploadData();
   }
}

void upload_client_upload_cycle_handler(const ServiceInvokeResponse &response, void* args)
{
   UploadClientWrapper *self = static_cast<UploadClientWrapper*>(args);
   if(response.getStatus()){
      self->m_context->uploaded += self->m_chunkSize;
      self->emitUploadProgress(self->m_context->uploaded, self->m_context->fileSize, self->m_context->filename);
      QVariant cycleComplete = response.getDataItem("cycleComplete");
      if(!cycleComplete.isNull() && cycleComplete.toBool()){
         self->uploadCycle();
      }
   }else{
      self->emitUploadError(response.getError().first, response.getError().second);
      self->clearState();
   }
}

void upload_client_notify_complete_handler(const ServiceInvokeResponse &response, void* args)
{
   UploadClientWrapper *self = static_cast<UploadClientWrapper*>(args);
   if(response.getStatus()){
      self->emitUploadSuccess();
   }else{
      self->emitUploadError(response.getError().first, response.getError().second);
   }
}

UploadClientWrapper::UploadClientWrapper(QSharedPointer<ServiceInvoker> serviceInvoker)
   : m_serviceInvoker(serviceInvoker),
     m_cycleSize(1),
     m_chunkSize(1024*512),
     m_status(true)
{
   connect(m_serviceInvoker.data(), &ServiceInvoker::connectedToServerSignal, this, &UploadClientWrapper::connectToServerHandler);
   connect(m_serviceInvoker.data(), &ServiceInvoker::connectErrorSignal, this, &UploadClientWrapper::connectErrorHandler);
}

void UploadClientWrapper::connectToServerHandler()
{
   ServiceInvokeRequest request("Common/Uploader", "init", {
                                   {"filename", m_context->filename},
                                   {"filesize", m_context->fileSize},
                                   {"cycleSize", m_cycleSize},
                                   {"chunkSize", m_chunkSize},
                                   {"uploadDir", m_uploadDir}
                                });
   m_serviceInvoker->request(request, upload_client_init_upload_handler, static_cast<void*>(this));
}

void UploadClientWrapper::connectErrorHandler()
{
   m_status = false;
   emit uploadError(1, "连接到meta server服务器失败");
}

UploadClientWrapper& UploadClientWrapper::changUploadDir(const QString &uploadDir)
{
   m_uploadDir = uploadDir;
   return *this;
}

QString UploadClientWrapper::getUploadDir()
{
   return m_uploadDir;
}

void UploadClientWrapper::upload(const QString &filename)
{
   emit startUpload();
   m_context.reset(new UploadContext);
   m_context->filename = filename;
   if(!Filesystem::fileExist(filename)){
      m_status = false;
      emit uploadError(2, QString("文件%1不存在").arg(filename));
      clearState();
      return;
   }
   m_context->targetFile = new QFile(filename);
   if(!m_context->targetFile->open(QFile::ReadOnly)){
      m_status = false;
      emit uploadError(3, QString("文件 : %1 打开失败").arg(filename));
      clearState();
      return;
   }
   m_context->fileSize = m_context->targetFile->size();
   m_serviceInvoker->connectToServer();
}

void UploadClientWrapper::beginUploadData()
{
   emit uploadProgress(0, m_context->fileSize, m_context->filename);
   uploadCycle();
}

void UploadClientWrapper::uploadCycle()
{
   for(int i = 0; i < m_cycleSize; i++){
      if(m_context->uploaded < m_context->fileSize){
         ServiceInvokeRequest request("Common/Uploader", "receiveData");
         request.setExtraData(m_context->targetFile->read(m_chunkSize));
         m_serviceInvoker->request(request, upload_client_upload_cycle_handler, static_cast<void*>(this));
      }else{
         ServiceInvokeRequest request("Common/Uploader", "notifyUploadComplete");
         m_serviceInvoker->request(request, upload_client_notify_complete_handler, static_cast<void*>(this));
      }
   }
}

void UploadClientWrapper::clearState()
{
   if(!m_context.isNull() && nullptr != m_context->targetFile){
      delete m_context->targetFile;
   }
   m_context.clear();
   m_step = UPLOAD_STEP_PREPARE;
   m_status = true;
}

void UploadClientWrapper::emitUploadError(int errorCode, const QString &errorMsg)
{
   m_status = false;
   emit uploadError(errorCode, errorMsg);
}

void UploadClientWrapper::emitUploadSuccess()
{
   emit uploadSuccess();
}

void UploadClientWrapper::emitUploadProgress(int uploaded, int totalSize, const QString &filename)
{
   emit uploadProgress(uploaded, totalSize, filename);
}

UploadClientWrapper::~UploadClientWrapper()
{
   disconnect(m_serviceInvoker.data(), &ServiceInvoker::connectedToServerSignal, this, &UploadClientWrapper::connectToServerHandler);
   disconnect(m_serviceInvoker.data(), &ServiceInvoker::connectErrorSignal, this, &UploadClientWrapper::connectErrorHandler);
}

}//common
}//lxservice