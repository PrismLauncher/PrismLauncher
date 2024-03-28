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
        prismlauncher-unwrapped
        prismlauncher
        ;
      default = ourPackages.prismlauncher;
    };
  };

  flake = {
    overlays.default = final: prev: let
      version = builtins.substring 0 8 self.lastModifiedDate or "dirty";
    in {
      prismlauncher-unwrapped = prev.qt6Packages.callPackage ./pkg {
        inherit (inputs) libnbtplusplus;
        inherit ((final.darwin or prev.darwin).apple_sdk.frameworks) Cocoa;
        inherit version;
      };

      prismlauncher = prev.qt6Packages.callPackage ./pkg/wrapper.nix {
        inherit (final) prismlauncher-unwrapped;
      };
    };
  };
}
