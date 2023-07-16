{
  inputs,
  self,
  ...
}: {
  imports = [
    ./dev.nix
    ./distribution.nix
  ];

  _module.args = {
    # User-friendly version number.
    version = builtins.substring 0 8 self.lastModifiedDate;
  };

  perSystem = {system, ...}: {
    # Nixpkgs instantiated for supported systems with our overlay.
    _module.args.pkgs = import inputs.nixpkgs {
      inherit system;
      overlays = [self.overlays.default];
    };
  };

  # Supported systems.
  systems = [
    "x86_64-linux"
    "x86_64-darwin"
    "aarch64-linux"
    # Disabled due to qtbase being currently broken for "aarch64-darwin."
    # "aarch64-darwin"
  ];
}
