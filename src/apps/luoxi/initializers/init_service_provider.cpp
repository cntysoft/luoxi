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
   provider.addServiceToPool("KeleCloud/InstanceDeploy", [](ServiceProvider& provider)-> AbstractService*{
      return new lxservice::kelecloud::InstanceDeployWrapper(provider);
   });
   provider.addServiceToPool("ZhuChao/NewDeploy", [](ServiceProvider& provider)-> AbstractService*{
      return new lxservice::zhuchao::NewDeployWrapper(provider);
   });
}

void cleanup_service_provider()
{
//   ServiceProvider &provider = ServiceProvider::instance();
//   delete &provider;
}

}//luoxi