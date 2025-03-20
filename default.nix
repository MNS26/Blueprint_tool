{ stdenv, protobuf, abseil-cpp, pkg-config, lz4, lib, util-linux, dpp, curl }:

stdenv.mkDerivation {
  name = "blueprint";
  src = ./.;
  buildInputs = [ lz4 protobuf dpp curl ];
  nativeBuildInputs = [ pkg-config protobuf util-linux ];
  preferLocalBuild = true;
  WINDOWS = stdenv.targetPlatform.isWindows;
}
