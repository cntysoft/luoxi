import qbs
Project
{
   minimumQbsVersion: "1.4.2"
   qbsSearchPaths:["qbs-resources"]
   property bool releaseBuild : false
   property bool enableRPath: true
   property string libDirName: "lib"
   property string appInstallDir : "bin"
   property string installRoot: "/usr/local"
   property string resourcesInstallDir: "share/deploysystem"
   property string luoxiVersion: "0.0.1"
   property stringList libRPaths: {
      if (!enableRPath){
         return undefined;
      }else{
         return ["$ORIGIN/../" + libDirName];
      }
   }
   references : [
      "src/src.qbs",
      "share/share.qbs"
   ]
}
