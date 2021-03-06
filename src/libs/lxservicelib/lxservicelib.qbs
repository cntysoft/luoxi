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
           "download_client.h",
           "download_client_wrapper.cpp",
           "upload_client.h",
           "upload_client_wrapper.cpp",
       ]
   }
   Group {
      name : "zhuchao"
      prefix : name + "/"
      files : [
           "db_backup.h",
           "db_backup_wrapper.cpp",
           "new_deploy.h",
           "new_deploy_wrapper.cpp",
           "shop_db_upgrader.h",
           "shop_db_upgrader_wrapper.cpp",
           "upgrade_deploy.h",
           "upgrade_deploy_wrapper.cpp",
       ]
   }
   
   Group {
      name : "serverstatus"
      prefix : name + "/"
      files : [
           "server_info.h",
           "server_info_wrapper.cpp",
       ]
   }
}
