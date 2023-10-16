{
  inputs,
  self,
  ...
}: {
  perSystem = {
    lib,
    pkgs,
    ...
  }: {
    packages = let
      ourPackages = lib.fix (final: self.overlays.default final pkgs);
    in {
      inherit
        (ourPackages)
        prismlauncher-qt5-unwrapped
        prismlauncher-qt5
        prismlauncher-unwrapped
        prismlauncher
        ;
      default = ourPackages.prismlauncher;
    };
  };

  flake = {
    overlays.default = final: prev: let
      version = builtins.substring 0 8 self.lastModifiedDate or "dirty";

      filteredSelf = inputs.nix-filter.lib.filter {
        root = ../.;
        include = [
          "buildconfig"
          "cmake"
          "launcher"
          "libraries"
          "program_info"
          "tests"
          ../COPYING.md
          ../CMakeLists.txt
        ];
      };

      # common args for prismlauncher evaluations
      unwrappedArgs = {
        self = filteredSelf;

        inherit (inputs) libnbtplusplus;
        inherit ((final.darwin or prev.darwin).apple_sdk.frameworks) Cocoa;
        inherit version;
      };
    in {
      prismlauncher-qt5-unwrapped = prev.libsForQt5.callPackage ./pkg unwrappedArgs;

      prismlauncher-qt5 = prev.libsForQt5.callPackage ./pkg/wrapper.nix {
        prismlauncher-unwrapped = final.prismlauncher-qt5-unwrapped;
      };

      prismlauncher-unwrapped = prev.qt6Packages.callPackage ./pkg unwrappedArgs;

      prismlauncher = prev.qt6Packages.callPackage ./pkg/wrapper.nix {
        inherit (final) prismlauncher-unwrapped;
      };
    };
  };
}
