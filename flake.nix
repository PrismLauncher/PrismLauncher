{
  description = "PolyMC";

  outputs = inputs: {
    overlay = import ./overlay.nix;
  };
}