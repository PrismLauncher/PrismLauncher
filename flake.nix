{
  description = "PolyMC flake";
  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  inputs.flake-utils.url = "github:numtide/flake-utils";
  inputs.libnbtplusplus = {
    url = "github:multimc/libnbtplusplus";
    flake = false;
  };
  inputs.quazip = {
    url = "github:multimc/quazip";
    flake = false;
  };

  outputs = inputs@{ self, nixpkgs, flake-utils, libnbtplusplus, quazip, ... }:
    flake-utils.lib.eachSystem [ "x86_64-linux" "aarch64-linux" ] (system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };
 
        packages = {
          polymc = pkgs.libsForQt5.callPackage ./packages/nix/polymc {
            inherit self;
            submoduleQuazip = quazip;
            submoduleNbt = libnbtplusplus;
          };
        };

        overlay = import ./packages/nix/overlay.nix {
          inherit self quazip libnbtplusplus;
        };

        apps = {
          polymc = flake-utils.lib.mkApp {
            name = "PolyMC";
            drv = packages.polymc;
          };
        };
      in
      {
        inherit packages overlay apps;
        defaultPackage = packages.polymc;
        defaultApp = apps.polymc;
      }
    );
}
