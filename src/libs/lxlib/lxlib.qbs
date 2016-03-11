import qbs 1.0
Product
{
   type: "dynamiclibrary"
   name : "lxlib"
   targetName : "lx"
   Depends { 
      name: "Qt"; 
      submodules: ["core", "network","websockets"]
   }
   Depends { name:"corelib"}
   Depends { name:"cpp"}
   destinationDirectory: "lib"
   cpp.defines: {
      var defines = [];
      if(product.type == "staticlibrary"){
         defines.push("LUOXI_STATIC_LIB");
      }else{
         defines.push("LUOXI_LIBRARY");
      }
      defines = defines.concat([
                                  'LUOXI_LIB_VERSION="'+ version+'"',
                                  'LUOXI_VERSION="' + project.luoxiVersion + '"'
                               ]);
      return defines;
   }
   cpp.visibility: "minimal"
   cpp.cxxLanguageVersion: "c++14"
   cpp.includePaths:[".", "../"]
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
   Group {
      name: "global"
      prefix: name+"/"
      files: [
           "common_funcs.cpp",
           "common_funcs.h",
           "const.h",
           "global.h",
       ]
   }
   Group {
      name : "kernel"
      prefix : name + "/"
      files : [
           "stddir.cpp",
           "stddir.h",
       ]
   }
   Group {
      name : "network"
      prefix : name + "/"
      files : [
         "multi_thread_server.cpp",
         "multi_thread_server.h"	
      ]
   }
}
