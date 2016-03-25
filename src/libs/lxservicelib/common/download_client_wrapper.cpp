#include <QFile>
#include "lxlib/kernel/stddir.h"
#include "download_client.h"
#include "corelib/io/filesystem.h"
#include <QDebug>

namespace lxservice{
namespace common{

using sn::corelib::Filesystem;
using lxlib::kernel::StdDir;

void init_download_handler(const ServiceInvokeResponse &response, void* args)
{
   DownloadClientWrapper *self = static_cast<DownloadClientWrapper*>(args);
   if(!response.getStatus()){
      self->emitDownloadError(response.getError().first, response.getError().second);
      self->clearState();
   }else{
      const QMap<QString, QVariant> &args = response.getData();
      self->m_step = DownloadClientWrapper::DOWNLOAD_STEP_META_RECEIVED;
      self->m_context->chunkSize = args.value("chunkSize").toInt();
      self->m_context->cycleSize = args.value("cycleSize").toInt();
      self->m_context->fileSize = args.value("fileSize").toInt();
      self->beginRetrieveData();
   }
}

void download_cycle_handler(const ServiceInvokeResponse &response, void* args)
{
   DownloadClientWrapper *self = static_cast<DownloadClientWrapper*>(args);
   if(!response.getStatus()){
      self->emitDownloadError(response.getError().first, response.getError().second);
      self->clearState();
   }else{
      //保存数据
      QByteArray data = response.getExtraData();
      if(data.size() == 0){
         self->downloadCycle();
         return;
      }else{
         self->m_context->downloadPointer += data.size();
         self->m_context->targetFile->write(data);
      }
      if(self->m_context->downloadPointer < self->m_context->fileSize){
         self->downloadCycle();
      }else{
         self->m_context->targetFile->flush();
         self->m_context->targetFile->close();
         ServiceInvokeRequest request("Common/DownloadServer", "notifyComplete");
         self->m_serviceInvoker->request(request);
         self->clearState();
         self->emitDownloadComplete();
      }
   }
}

void DownloadClientWrapper::emitDownloadError(int errorCode, const QString &errorMsg)
{
   emit downloadError(errorCode, errorMsg);
}

void DownloadClientWrapper::emitDownloadComplete()
{
   emit downloadComplete();
}

DownloadClientWrapper::DownloadClientWrapper(QSharedPointer<ServiceInvoker> serviceInvoker)
   :m_serviceInvoker(serviceInvoker),m_status(true)
{
   connect(m_serviceInvoker.data(), &ServiceInvoker::connectedToServerSignal, this, &DownloadClientWrapper::connectToServerHandler);
   connect(m_serviceInvoker.data(), &ServiceInvoker::connectErrorSignal, this, &DownloadClientWrapper::connectErrorHandler);
}

void DownloadClientWrapper::download(const QString &filename)
{
   emit beginDownload();
   m_step = DOWNLOAD_STEP_START;
   m_serviceInvoker->connectToServer();
   m_context.reset(new DownloadContext);
   m_context->filename = filename;
}

void DownloadClientWrapper::connectToServerHandler()
{
   
   ServiceInvokeRequest request("Common/DownloadServer", "init", {
                                   {"filename", m_context->filename}
                                });
   m_serviceInvoker->request(request, init_download_handler, static_cast<void*>(this));
}

void DownloadClientWrapper::connectErrorHandler()
{
   m_status = false;
   emit downloadError(-1, "连接到下载服务器失败");
}

void DownloadClientWrapper::beginRetrieveData()
{
   QString repoDir(StdDir::getSoftwareRepoDir());
   if(!Filesystem::fileExist(repoDir)){
      Filesystem::createPath(repoDir);
   }
   m_context->targetFile = new QFile(StdDir::getSoftwareRepoDir() + '/' + m_context->filename);
   if(!m_context->targetFile->open(QIODevice::WriteOnly|QIODevice::Truncate)){
      emit downloadError(-1, QString("打开本地文件出错 : %1").arg(m_context->targetFile->errorString()));
      ServiceInvokeRequest request("Common/DownloadServer", "terminal");
      m_serviceInvoker->request(request);
      clearState();
      return;
   }
   m_step = DOWNLOAD_STEP_PROCESS;
   m_context->downloadPointer = 0;
   downloadCycle();
}

void DownloadClientWrapper::downloadCycle()
{
   int retrieveSize = qMin(m_context->fileSize - m_context->downloadPointer, m_context->chunkSize);
   ServiceInvokeRequest request("Common/DownloadServer", "sendData", {
                                   {"retrieveSize", retrieveSize},
                                   {"startPointer", m_context->downloadPointer}
                                });
   m_serviceInvoker->request(request, download_cycle_handler, static_cast<void*>(this));
}

DownloadClientWrapper::~DownloadClientWrapper()
{
   disconnect(m_serviceInvoker.data(), &ServiceInvoker::connectedToServerSignal, this, &DownloadClientWrapper::connectToServerHandler);
   disconnect(m_serviceInvoker.data(), &ServiceInvoker::connectErrorSignal, this, &DownloadClientWrapper::connectErrorHandler);
}

void DownloadClientWrapper::clearState()
{
   if(!m_context.isNull() && nullptr != m_context->targetFile){
      delete m_context->targetFile;
   }
   m_context.clear();
   m_step = DOWNLOAD_STEP_PREPARE;
   m_status = true;
}

}//common
}//lxservice