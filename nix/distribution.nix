{
  inputs,
  self,
  version,
  ...
}: {
  perSystem = {pkgs, ...}: {
    packages = {
      inherit (pkgs) prismlauncher-qt5-unwrapped prismlauncher-qt5 prismlauncher-unwrapped prismlauncher;
      default = pkgs.prismlauncher;
    };
  };

  flake = {
    overlays.default = final: prev: let
      # Helper function to build prism against different versions of Qt.
      mkPrism = qt:
        qt.callPackage ./package.nix {
          inherit (inputs) libnbtplusplus;
          inherit self version;
        };
    in {
      prismlauncher-qt5-unwrapped = mkPrism final.libsForQt5;
      prismlauncher-qt5 = prev.prismlauncher-qt5.override {prismlauncher-unwrapped = final.prismlauncher-qt5-unwrapped;};
      prismlauncher-unwrapped = mkPrism final.qt6Packages;
      prismlauncher = prev.prismlauncher.override {inherit (final) prismlauncher-unwrapped;};
    };
  };
}
