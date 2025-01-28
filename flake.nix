{
  outputs = { self, nixpkgs }:
  {
    packages = {
      aarch64-multiplatform.default = nixpkgs.legacyPackages.x86_64-linux.pkgsCross.aarch64-multiplatform.callPackage ./. {};
      mingw32.default = nixpkgs.legacyPackages.x86_64-linux.pkgsCross.mingw32.callPackage ./. {};
      mingwW64.default = nixpkgs.legacyPackages.x86_64-linux.pkgsCross.mingwW64.callPackage ./. {};
      x86_64-linux.default = nixpkgs.legacyPackages.x86_64-linux.callPackage ./. {};
    };
  };
}
