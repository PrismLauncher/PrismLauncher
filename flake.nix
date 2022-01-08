{
  description = "PolyMC flake";
  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  inputs.flake-utils.url = "github:numtide/flake-utils";

  outputs = inputs@{ self, nixpkgs, flake-utils,... }:
    flake-utils.lib.eachSystem [ "x86_64-linux" "aarch64-linux" ] (system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };
 
        packages = {
          polymc = pkgs.libsForQt5.callPackage ./package/nix {};
        };
        
        apps = {
          polymc = flake-utils.lib.mkApp {
            name = "PolyMC";
            drv = packages.polymc;
          };
        };
      in
      {
        inherit packages apps;

        defaultPackage = packages.polymc;
        defaultApp = apps.polymc;
        overlay = import ./packages/nix/overlay.nix;
      }
    );
}
