{ stdenv, protobufc, protobuf, abseil-cpp, pkg-config, lz4}:

stdenv.mkDerivation {
  name = "blueprint";
  src = ./.;
  buildInputs = [ protobufc protobuf abseil-cpp pkg-config lz4 ];
}
