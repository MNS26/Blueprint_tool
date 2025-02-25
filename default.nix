{ stdenv, protobuf, abseil-cpp, pkg-config, lz4, lib, util-linux }:

stdenv.mkDerivation {
  name = "blueprint";
  src = ./.;
  buildInputs = [ lz4 protobuf ];
  nativeBuildInputs = [ pkg-config protobuf util-linux ];
  preferLocalBuild = true;
  WINDOWS = stdenv.targetPlatform.isWindows;
}
