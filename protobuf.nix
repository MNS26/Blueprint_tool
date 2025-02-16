{ stdenv, fetchFromGitHub, autoreconfHook, gcc }:

stdenv.mkDerivation {
  name = "protobuf-v20.2";
  src = fetchFromGitHub {
    owner = "protocolbuffers";
    repo = "protobuf";
    rev = "v20.2";
    sha256 = "sha256-7hLTIujvYIGRqBQgPHrCq0XOh0GJrePBszXJnBFaXVM=";
  };
  nativeBuildInputs = [ autoreconfHook gcc ];
  enableParallelBuilding = true;
}
