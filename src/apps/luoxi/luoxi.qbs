import qbs 1.0
LuoxiApplication
{
   name : "luoxi"
   Depends { 
      name: "Qt"; 
      submodules: [
         "sql",
         "websockets"
      ]
   }
   cpp.includePaths: base.concat([
                                    ".","../../libs"
                                 ])
   cpp.defines: {
      var defines = [];
      defines.push('LUOXI_VERSION="' + project.upgrademgrMasterversion + '"');
      if(!project.releaseBuild){
         defines.push("LUOXI_DEBUG_BUILD")
      }
      return defines;
   }
}