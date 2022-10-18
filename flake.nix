{
  description = "A custom launcher for Minecraft that allows you to easily manage multiple installations of Minecraft at once (Fork of MultiMC)";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
    flake-compat = { url = "github:edolstra/flake-compat"; flake = false; };
    libnbtplusplus = { url = "github:PrismLauncher/libnbtplusplus"; flake = false; };
    tomlplusplus = { url = "github:marzer/tomlplusplus"; flake = false; };
  };

  outputs = { self, nixpkgs, libnbtplusplus, tomlplusplus, ... }:
    let
      # User-friendly version number.
      version = builtins.substring 0 8 self.lastModifiedDate;

      # Supported systems (qtbase is currently broken for "aarch64-darwin")
      supportedSystems = [ "x86_64-linux" "x86_64-darwin" "aarch64-linux" ];

      # Helper function to generate an attrset '{ x86_64-linux = f "x86_64-linux"; ... }'.
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;

      # Nixpkgs instantiated for supported systems.
      pkgs = forAllSystems (system: nixpkgs.legacyPackages.${system});

      packagesFn = pkgs: rec {
        prismlauncher = pkgs.libsForQt5.callPackage ./nix { inherit version self libnbtplusplus tomlplusplus; };
        prismlauncher-qt6 = pkgs.qt6Packages.callPackage ./nix { inherit version self libnbtplusplus tomlplusplus; };
      };
    in
    {
      packages = forAllSystems (system:
        let packages = packagesFn pkgs.${system}; in
        packages // { default = packages.prismlauncher; }
      );

      overlay = final: packagesFn;
    };
}
