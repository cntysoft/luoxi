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
   provider.addServiceToPool("ServerStatus/ServerInfo", [](ServiceProvider& provider)-> AbstractService*{
      return new lxservice::serverstatus::ServerInfoWrapper(provider);
   });
   provider.addServiceToPool("KeleCloud/InstanceDeploy", [](ServiceProvider& provider)-> AbstractService*{
      return new lxservice::kelecloud::InstanceDeployWrapper(provider);
   });
   provider.addServiceToPool("ZhuChao/NewDeploy", [](ServiceProvider& provider)-> AbstractService*{
      return new lxservice::zhuchao::NewDeployWrapper(provider);
   });
   provider.addServiceToPool("ZhuChao/UpgradeDeploy", [](ServiceProvider& provider)-> AbstractService*{
      return new lxservice::zhuchao::UpgradeDeployWrapper(provider);
   });
   provider.addServiceToPool("ZhuChao/DbBackup", [](ServiceProvider& provider)-> AbstractService*{
      return new lxservice::zhuchao::DbBackupWrapper(provider);
   });
   provider.addServiceToPool("ZhuChao/ShopDbUpgrader", [](ServiceProvider& provider)-> AbstractService*{
      return new lxservice::zhuchao::ShopDbUpgraderWrapper(provider);
   });
}

void cleanup_service_provider()
{
   ServiceProvider &provider = ServiceProvider::instance();
   delete &provider;
}

}//luoxi