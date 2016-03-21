#ifndef LUOXI_SERVICE_COMMON_SERVER_INFO_H
#define LUOXI_SERVICE_COMMON_SERVER_INFO_H

#include <QSharedPointer>
#include <QString>
#include <QMap>

#include "lxservicelib/global_defs.h"
#include "corelib/network/rpc/abstract_service.h"
#include "corelib/network/rpc/invoke_meta.h"
#include "corelib/network/rpc/service_provider.h"

namespace lxservice{
namespace common{

LUOXI_USING_SERVICE_NAMESPACES

class LUOXI_SERVICE_EXPORT ServerInfoWrapper : public AbstractService
{
   Q_OBJECT
public:
   ServerInfoWrapper(ServiceProvider& provider);
   Q_INVOKABLE ServiceInvokeResponse getVersionInfo(const ServiceInvokeRequest &request);
};


}//common
}//lxservice

#endif // LUOXI_SERVICE_COMMON_SERVER_INFO_H
