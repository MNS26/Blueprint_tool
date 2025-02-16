{ stdenv, protobuf, abseil-cpp, pkg-config, lz4, lib }:

stdenv.mkDerivation {
  name = "blueprint";
  src = ./.;
  buildInputs = [ lz4 protobuf ];
  nativeBuildInputs = [ pkg-config protobuf ];
  preferLocalBuild = true;
}
