{
  description = "PolyMC";

  outputs = inputs: {
    overlay = import ./packages/nix/overlay.nix;
  };
}
