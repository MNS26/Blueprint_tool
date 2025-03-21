{ stdenv
, abseil-cpp
, curl
, dpp
, lib
, lz4
, pkg-config
, protobuf
, util-linux
}:

stdenv.mkDerivation {
  name = "blueprint";
  src = ./.;
  buildInputs = [ lz4 protobuf curl ];
  nativeBuildInputs = [ pkg-config protobuf util-linux ];
  preferLocalBuild = true;
  WINDOWS = stdenv.targetPlatform.isWindows;
}
