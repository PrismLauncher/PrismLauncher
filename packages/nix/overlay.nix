{ self }:

final: prev: rec {
  polymc = prev.libsForQt5.callPackage ./polymc { inherit self; };
}
