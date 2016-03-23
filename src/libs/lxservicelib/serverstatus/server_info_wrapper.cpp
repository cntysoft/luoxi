#include <QMap>
#include <QString>
#include <QVariant>

#include "server_info.h"
#include "lxlib/global/common_funcs.h"
#include "lxlib/kernel/stddir.h"
#include "corelib/io/filesystem.h"

namespace lxservice{
namespace serverstatus{

using lxlib::kernel::StdDir;
using sn::corelib::Filesystem;

ServerInfoWrapper::ServerInfoWrapper(ServiceProvider &provider)
   : AbstractService(provider)
{
}

ServiceInvokeResponse ServerInfoWrapper::getVersionInfo(const ServiceInvokeRequest &request)
{
   ServiceInvokeResponse response("ServerStatus/ServerInfo/getVersionInfo", true);
   response.setSerial(request.getSerial());
   response.setDataItem("version", lxlib::global::get_luoxi_version());
   response.setIsFinal(true);
   return response;
}

}//serverstatus
}//lxservice