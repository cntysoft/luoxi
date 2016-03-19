import qbs 1.0
Product
{
   type: "dynamiclibrary"
   name : "lxservicelib"
   targetName : "lxservice"
   Depends { 
      name: "Qt"; 
      submodules: ["core", "network","websockets","qml"]
   }
   Depends { name:"corelib"}
   Depends { name:"lxlib"}
   Depends { name:"cpp"}
   destinationDirectory: "lib"
   cpp.defines: {
      var defines = [];
      if(product.type == "staticlibrary"){
         defines.push("LUOXI_SERVICE_STATIC_LIB");
      }else{
         defines.push("LUOXI_SERVICE_LIBRARY");
      }
      defines = defines.concat([
                                  'LUOXI_SERVICE_LIB_VERSION="'+ version+'"',
                                  'LUOXI_VERSION="' + project.luoxiVersion+'"'
                               ]);
      return defines;
   }
   cpp.visibility: "minimal"
   cpp.cxxLanguageVersion: "c++14"
   cpp.includePaths:[".","../lxlib/", "../lxservicelib/", "../"]
   Export {
      Depends { name: "cpp" }
      Depends { name: "Qt"; submodules: ["core"] }
      cpp.rpaths: ["$ORIGIN/../lib"]
      cpp.includePaths: [product.sourceDirectory+"../"]
   }
   Group {
      fileTagsFilter: product.type.concat("dynamiclibrary_symlink")
      qbs.install: true
      qbs.installDir: "lib"
   }
   files: [
      "global_defs.h",
      "macros.h",
      "service_repo.h",
   ]
   Group {
      name : "kelecloud"
      prefix : name + "/"
      files : [
         "instance_deploy.h",
         "instance_deploy_wrapper.cpp",
      ]
   }
   Group {
      name : "common"
      prefix : name + "/"
      files : [
         "download_client.cpp",
         "download_client.h",
      ]
   }
   Group {
      name : "zhuchao"
      prefix : name + "/"
      files : [
           "new_deploy.h",
           "new_deploy_wrapper.cpp",
           "upgrade.h",
           "upgrade_wrapper.cpp",
       ]
   }
}
