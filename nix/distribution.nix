{
  inputs,
  self,
  ...
}: {
  perSystem = {pkgs, ...}: {
    packages = let
      ourPackages = self.overlays.default pkgs null;
    in {
      inherit (ourPackages) prismlauncher-qt5-unwrapped prismlauncher-qt5 prismlauncher-unwrapped prismlauncher;
      default = ourPackages.prismlauncher;
    };
  };

  flake = {
    overlays.default = final: _: let
      version = builtins.substring 0 8 self.lastModifiedDate;

      # common args for prismlauncher evaluations
      unwrappedArgs = {
        inherit (inputs) libnbtplusplus;
        inherit (final.darwin.apple_sdk.frameworks) Cocoa;
        inherit self version;
      };
    in rec {
      prismlauncher-qt5-unwrapped = final.libsForQt5.callPackage ./pkg unwrappedArgs;
      prismlauncher-qt5 = final.libsForQt5.callPackage ./pkg/wrapper.nix {prismlauncher-unwrapped = prismlauncher-qt5-unwrapped;};
      prismlauncher-unwrapped = final.qt6Packages.callPackage ./pkg unwrappedArgs;
      prismlauncher = final.qt6Packages.callPackage ./pkg/wrapper.nix {inherit prismlauncher-unwrapped;};
    };
  };
}
