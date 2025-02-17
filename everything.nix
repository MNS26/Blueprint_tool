{ runCommand, self }:

runCommand "everything" {} ''
  mkdir $out
  cd $out
  mkdir linux win32 win64
  cp ${self.packages.x86_64-linux.default}/bin/* linux/
  cp -L ${self.packages.mingwW64.default}/bin/* win64/
  cp -L ${self.packages.mingw32.default}/bin/* win32/
''
