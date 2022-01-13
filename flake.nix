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
    url = "github:multimc/quazip";
    flake = false;
  };

  outputs = args@{ self, nixpkgs, flake-utils, libnbtplusplus, quazip, ... }:
    {
      overlay = final: prev: {
        inherit (self.packages.${final.system}) polymc;
      };
    } // flake-utils.lib.eachDefaultSystem (system:
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
