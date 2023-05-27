{
  inputs,
  self,
  version,
  ...
}: {
  perSystem = {pkgs, ...}: {
    packages = {
      inherit (pkgs) prismlauncher prismlauncher-qt5;
      default = pkgs.prismlauncher;
    };
  };

  flake = {
    overlays.default = _: prev: let
      # Helper function to build prism against different versions of Qt.
      mkPrism = qt:
        qt.callPackage ./package.nix {
          inherit (inputs) libnbtplusplus;
          inherit self version;
        };
    in {
      prismlauncher = mkPrism prev.qt6Packages;
      prismlauncher-qt5 = mkPrism prev.libsForQt5;
    };
  };
}
