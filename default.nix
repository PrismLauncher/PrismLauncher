{system ? builtins.currentSystem, ...}: let
  flake = import ./nix/compat.nix;
in
  assert (flake.packages ? "${system}") || builtins.throw "${system} not supported! Please use the overlay"; flake.packages.${system}
