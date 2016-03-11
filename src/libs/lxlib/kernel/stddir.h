#ifndef LUOXI_LIB_KERNEL_STDDIR_H
#define LUOXI_LIB_KERNEL_STDDIR_H

#include <QString>

#include "lxlib/global/global.h"

#include "corelib/kernel/stddir.h"
#include "corelib/kernel/application.h"

namespace lxlib{
namespace kernel{

using BaseStdDir = sn::corelib::StdDir;

class LUOXI_LIB_EXPORT StdDir : public BaseStdDir
{
public:
   static QString getBaseDataDir();
   static QString getSoftwareRepoDir();
   static QString getMetaDir();
};

}//network
}//lxlib

#endif // LUOXI_LIB_KERNEL_STDDIR_H