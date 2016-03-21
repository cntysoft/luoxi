import qbs 1.0
Product {
   type : "application"
   Depends { 
      name: "Qt"; 
      submodules: ["core", "network"]
   }
   Depends {name:"cpp"}
//   Depends { name: "lxlib"}
   Depends { name: "corelib"}
   consoleApplication: true
   destinationDirectory: "bin"
   cpp.cxxLanguageVersion: "c++14"
   cpp.defines : {
      var defines = [];
      var resourceDir;
      var installRoot;
      if(qbs.buildVariant == "debug"){
         resourceDir = 'LUOXI_SHARE_RES_DIR="' + qbs.installRoot+'/'+project.resourcesInstallDir+ '"';
         installRoot = 'LUOXI_INSTALL_ROOT="' + qbs.installRoot + '"';
      }else{
         resourceDir = 'LUOXI_SHARE_RES_DIR="' + project.installRoot+'/'+project.resourcesInstallDir+ '"';
         installRoot = 'LUOXI_INSTALL_ROOT="' + project.installRoot + '"';
      }
      defines = defines.concat([
                                  installRoot,
                                  resourceDir,
                               ]);
      return defines;
   }
   Group {
      fileTagsFilter: product.type
      qbs.install: true
      qbs.installDir: project.appInstallDir
   }
}
