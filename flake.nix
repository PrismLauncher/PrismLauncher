{
  description = "PolyMC flake";
  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  inputs.flake-utils.url = "github:numtide/flake-utils";
  inputs.flake-compat = {
    url = "github:edolstra/flake-compat";
    flake = false;
  };
  inputs.libnbtplusplus = {
    url = "github:multimc/libnbtplusplus";
    flake = false;
  };
  inputs.quazip = {
    url = "github:stachenov/quazip";
    flake = false;
  };

  outputs = args@{ self, nixpkgs, flake-utils, libnbtplusplus, quazip, ... }:
    let
      systems = [
        "aarch64-linux"
        # "aarch64-darwin" # qtbase is currently broken
        "i686-linux"
        "x86_64-darwin"
        "x86_64-linux"
      ];
    in {
      overlay = final: prev: {
        inherit (self.packages.${final.system}) polymc;
      };
    } // flake-utils.lib.eachSystem systems (system:
      let pkgs = import nixpkgs { inherit system; };
      in {
        packages = {
          polymc = pkgs.libsForQt5.callPackage ./packages/nix/polymc {
            inherit self;
            submoduleQuazip = quazip;
            submoduleNbt = libnbtplusplus;
          };
        };
        apps = {
          polymc = flake-utils.lib.mkApp {
            name = "polymc";
            drv = self.packages.${system}.polymc;
          };
        };
        defaultPackage = self.packages.${system}.polymc;
        defaultApp = self.apps.${system}.polymc;
      });
}
