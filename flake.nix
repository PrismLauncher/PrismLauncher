{
  description = "A custom launcher for Minecraft that allows you to easily manage multiple installations of Minecraft at once (Fork of MultiMC)";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
    flake-compat = { url = "github:edolstra/flake-compat"; flake = false; };
    libnbtplusplus = { url = "github:multimc/libnbtplusplus"; flake = false; };
  };

  outputs = { self, nixpkgs, libnbtplusplus, ... }:
    let
      # Generate a user-friendly version number.
      version = builtins.substring 0 8 self.lastModifiedDate;

      # System types to support (qtbase is currently broken for "aarch64-darwin")
      supportedSystems = [ "x86_64-linux" "x86_64-darwin" "aarch64-linux" ];

      # Helper function to generate an attrset '{ x86_64-linux = f "x86_64-linux"; ... }'.
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;

      # Nixpkgs instantiated for supported system types.
      pkgs = forAllSystems (system: nixpkgs.legacyPackages.${system});
    in
    {
      packages = forAllSystems (system: {
        polymc = pkgs.${system}.libsForQt5.callPackage ./nix { inherit version self libnbtplusplus; };
        polymc-qt6 = pkgs.${system}.qt6Packages.callPackage ./nix { inherit version self libnbtplusplus; };
      });

      defaultPackage = forAllSystems (system: self.packages.${system}.polymc);

      apps = forAllSystems (system: { polymc = { type = "app"; program = "${self.defaultPackage.${system}}/bin/polymc"; }; });
      defaultApp = forAllSystems (system: self.apps.${system}.polymc);

      overlay = final: prev: { polymc = self.defaultPackage.${final.system}; };
    };
}
