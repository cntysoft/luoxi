#include <QTcpSocket>
#include <QDataStream>
#include <QDebug>

#include "multi_thread_server.h"

namespace lxlib{
namespace network{

static MultiThreadServer *globalServer = nullptr;

MultiThreadServer::MultiThreadServer(Application& app,QObject *parent)
   : AbstractMultiThreadServer(app, parent),
     m_serviceProvider(ServiceProvider::instance())
{
}

ServiceProvider& MultiThreadServer::getServiceProvider()
{
   return m_serviceProvider;
}

void MultiThreadServer::incomingConnection(qintptr socketDescriptor)
{
   //这里暂时不进行加密处理
   //暂时也不进行多线程实现
   QTcpSocket* socket = new QTcpSocket(this);
   ServiceProvider& provider = ServiceProvider::instance();
   provider.setUnderlineSocket((int) socketDescriptor, socket);
   socket->setSocketDescriptor(socketDescriptor);
   connect(socket, &QTcpSocket::readyRead, this, &MultiThreadServer::unboxRequest);
   connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
}

void MultiThreadServer::unboxRequest()
{
   QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
   char byte;
   while(!socket->atEnd()){
      socket->read(&byte, 1);
      if('\r' == byte){
         if(socket->bytesAvailable() >= 2){
            char forward1;
            char forward2;
            socket->read(&forward1, 1);
            socket->read(&forward2, 1);
            if('\n' == forward1 && '\t' == forward2){
               //解压当前的包
               QDataStream stream(m_packageUnitBuffer);
               ServiceInvokeRequest request;
               stream >> request;
               request.setSocketNum((int)socket->socketDescriptor());
               processRequest(request);
               m_packageUnitBuffer.clear();
            }else{
               m_packageUnitBuffer.append(byte);
               m_packageUnitBuffer.append(forward1);
               m_packageUnitBuffer.append(forward2);
            }
         }
      }else{
         m_packageUnitBuffer.append(byte);
      }
   }
}

MultiThreadServer::~MultiThreadServer()
{
}

void MultiThreadServer::processRequest(const ServiceInvokeRequest &request)
{
   m_serviceProvider.callService(request);
}

MultiThreadServer*& get_global_server()
{
   return globalServer;
}

void set_global_server(MultiThreadServer *server)
{
   globalServer = server;
}

}//network
}//umslib