{
  outputs = { self, nixpkgs }:
  {
    packages = {
      mingw32.default = nixpkgs.legacyPackages.x86_64-linux.pkgsCross.mingw32.callPackage ./. {};
      mingwW64.default = nixpkgs.legacyPackages.x86_64-linux.pkgsCross.mingwW64.callPackage ./. {};
      x86_64-linux.default = nixpkgs.legacyPackages.x86_64-linux.callPackage ./. {};
    };
  };
}
