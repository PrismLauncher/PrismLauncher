{
  description = "PolyMC";

  outputs = inputs: {
    overlay = self: super: rec {
      polymc = super.libsForQt5.callPackage ./nix/polymc {};
    };
  };
}