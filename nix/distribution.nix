{
  inputs,
  self,
  ...
}: {
  perSystem = {pkgs, ...}: {
    packages = let
      ourPackages = self.overlays.default (pkgs // ourPackages) pkgs;
    in
      ourPackages // {default = ourPackages.prismlauncher;};
  };

  flake.overlays.default = final: prev: let
    # common args for prismlauncher evaluations
    unwrappedArgs = {
      version = builtins.substring 0 7 self.rev or "dirty";
      inherit (inputs) libnbtplusplus;
      inherit (final.darwin.apple_sdk.frameworks) Cocoa;
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
}
