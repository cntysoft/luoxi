import qbs 1.0
import qbs.File
import qbs.FileInfo
Product {
   name: "resources"
   type: ["copied luoxi resources"]
   Group {
      name: "assets"
      files : [
           "assets/**/*",
           "conf/nginx_conf.tpl",
       ]
      fileTags: ["asset files"]
      qbs.install: true
      qbs.installDir: project.resourcesInstallDir
      qbs.installSourceBase: "."
   }
   Rule {
      inputs: ["asset files"]
      Artifact {
         filePath: FileInfo.joinPaths(project.buildDirectory, project.resourcesInstallDir
                                     , FileInfo.relativePath(product.sourceDirectory, input.filePath))
         fileTags: ["copied luoxi resources"]
      }
      prepare: {
         var cmd = new JavaScriptCommand();
         cmd.description = "Copying resource " + input.fileName + " to build directory.";
         cmd.highlight = "codegen";
         cmd.sourceCode = function() { File.copy(input.filePath, output.filePath); }
         return cmd;
      }
   }
}
