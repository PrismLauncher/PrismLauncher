{ self, quazip, libnbtplusplus }:

final: prev: rec {
  polymc = prev.libsForQt5.callPackage ./polymc {
    inherit self;
    submoduleQuazip = quazip;
    submoduleNbt = libnbtplusplus;
  };
}
