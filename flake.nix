{
  description = "A custom launcher for Minecraft that allows you to easily manage multiple installations of Minecraft at once (Fork of MultiMC)";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    pre-commit-hooks = {
      url = "github:cachix/pre-commit-hooks.nix";
      inputs.nixpkgs.follows = "nixpkgs";
      inputs.flake-utils.follows = "flake-utils";
    };
    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };
    libnbtplusplus = {
      url = "github:PrismLauncher/libnbtplusplus";
      flake = false;
    };
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    pre-commit-hooks,
    libnbtplusplus,
    ...
  }: let
    # User-friendly version number.
    version = builtins.substring 0 8 self.lastModifiedDate;

    # Supported systems (qtbase is currently broken for "aarch64-darwin")
    supportedSystems = with flake-utils.lib.system; [
      x86_64-linux
      x86_64-darwin
      aarch64-linux
    ];

    packagesFn = pkgs: {
      prismlauncher-qt5 = pkgs.libsForQt5.callPackage ./nix {
        inherit version self libnbtplusplus;
      };
      prismlauncher = pkgs.qt6Packages.callPackage ./nix {
        inherit version self libnbtplusplus;
      };
    };
  in
    flake-utils.lib.eachSystem supportedSystems (system: let
      pkgs = nixpkgs.legacyPackages.${system};
    in {
      checks = {
        pre-commit-check = pre-commit-hooks.lib.${system}.run {
          src = ./.;
          hooks = {
            markdownlint.enable = true;

            alejandra.enable = true;
            deadnix.enable = true;

            clang-format = {
              enable =
                false; # As most of the codebase is **not** formatted, we don't want clang-format yet
              types_or = ["c" "c++"];
            };
          };
        };
      };

      packages = let
        packages = packagesFn pkgs;
      in
        packages // {default = packages.prismlauncher;};

      devShells.default = pkgs.mkShell {
        inherit (self.checks.${system}.pre-commit-check) shellHook;
        packages = with pkgs; [
          nodePackages.markdownlint-cli
          alejandra
          deadnix
          clang-tools
        ];

        inputsFrom = [self.packages.${system}.default];
        buildInputs = with pkgs; [ccache ninja];
      };
    })
    // {
      overlays.default = final: _: (packagesFn final);
    };
}
