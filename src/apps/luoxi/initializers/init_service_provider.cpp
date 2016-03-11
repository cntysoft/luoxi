#include "initializers/initializer_cleanup_funcs.h"
#include "lxservicelib/service_repo.h"
#include "corelib/network/rpc/service_provider.h"
#include "corelib/network/rpc/abstract_service.h"

namespace luoxi{

using sn::corelib::network::ServiceProvider;
using sn::corelib::network::AbstractService;

void init_service_provider()
{
   ServiceProvider& provider = ServiceProvider::instance();
//   provider.addServiceToPool("Upgrader/UpgradeCloudController", [](ServiceProvider& provider)-> AbstractService*{
//      return new umsservice::upgrader::UpgradeCloudControllerWrapper(provider);
//   });
//   //   provider.addServiceToPool("ServerStatus/Info", [](ServiceProvider& provider)-> AbstractService*{
//   //                            return new ummservice::serverstatus::Info(provider);
//   //                         });
//   //   provider.addServiceToPool("Common/Uploader", [](ServiceProvider& provider)-> AbstractService*{
//   //                            return new ummservice::common::Uploader(provider);
//   //                         });
   provider.addServiceToPool("KeleCloud/InstanceDeploy", [](ServiceProvider& provider)-> AbstractService*{
      return new lxservice::kelecloud::InstanceDeployWrapper(provider);
   });
}

void cleanup_service_provider()
{
//   ServiceProvider &provider = ServiceProvider::instance();
//   delete &provider;
}

}//luoxi