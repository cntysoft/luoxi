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
                                  'UMS_LIB_VERSION="'+ version+'"'
                               ]);
      return defines;
   }
   cpp.visibility: "minimal"
   cpp.cxxLanguageVersion: "c++14"
   cpp.includePaths:[".","../umslib/", "../"]
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
}
