{ stdenv, protobuf_20, abseil-cpp, pkg-config, lz4, lib }:

stdenv.mkDerivation {
  name = "blueprint";
  src = ./.;
  buildInputs = [ lz4 protobuf_20 ];
  nativeBuildInputs = [ pkg-config protobuf_20 ];
  preferLocalBuild = true;
}
