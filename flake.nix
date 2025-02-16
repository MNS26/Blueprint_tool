{
  outputs = { self, nixpkgs }:
  let
    overlay = self: super: {
      abseil-cpp = null;
      protobuf_20 = self.callPackage ./protobuf.nix {
        inherit (nixpkgs.legacyPackages.x86_64-linux) gcc;
      };
      protobuf = self.protobuf_20;
    };
  in
  {
    packages = {
      aarch64-multiplatform.default = nixpkgs.legacyPackages.x86_64-linux.pkgsCross.aarch64-multiplatform.callPackage ./. {};
      mingw32.default = nixpkgs.legacyPackages.x86_64-linux.pkgsCross.mingw32.callPackage ./. {};
      mingwW64.default = (nixpkgs.legacyPackages.x86_64-linux.extend overlay).pkgsCross.mingwW64.callPackage ./. {};
      x86_64-linux.default = nixpkgs.legacyPackages.x86_64-linux.callPackage ./. {};
    };
  };
}
