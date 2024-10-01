# Prism Launcher Nix Packaging

## Installing a stable release (nixpkgs)

Prism Launcher is packaged in [nixpkgs](https://github.com/NixOS/nixpkgs/) since 22.11.

See [Package variants](#package-variants) for a list of available packages.

## Installing a development release (flake)

We use [cachix](https://cachix.org/) to cache our development and release builds.
If you want to avoid rebuilds you may add the Cachix bucket to your substitutors, or use `--accept-flake-config`
to temporarily enable it when using `nix` commands.

Example (NixOS):

```nix
{
  nix.settings = {
    trusted-substituters = [ "https://prismlauncher.cachix.org" ];

    trusted-public-keys = [
      "prismlauncher.cachix.org-1:9/n/FGyABA2jLUVfY+DEp4hKds/rwO+SCOtbOkDzd+c="
    ];
  };
}
```

### Installing the package directly

After adding `github:PrismLauncher/PrismLauncher` to your flake inputs, you can access the flake's `packages` output.

Example:

```nix
{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    prismlauncher = {
      url = "github:PrismLauncher/PrismLauncher";

      # Optional: Override the nixpkgs input of prismlauncher to use the same revision as the rest of your flake
      # Note that this may break the reproducibility mentioned above, and you might not be able to access the binary cache
      #
      # inputs.nixpkgs.follows = "nixpkgs";

      # This is not required for Flakes
      inputs.flake-compat.follows = "";
    };
  };

  outputs =
    { nixpkgs, prismlauncher, ... }:
    {
      nixosConfigurations.foo = nixpkgs.lib.nixosSystem {
        modules = [
          ./configuration.nix

          (
            { pkgs, ... }:
            {
              environment.systemPackages = [ prismlauncher.packages.${pkgs.system}.prismlauncher ];
            }
          )
        ];
      };
    };
}
```

### Using the overlay

Alternatively, if you don't want to use our `packages` output, you can add our overlay to your nixpkgs instance.
This will ensure Prism is built with your system's packages.

> [!WARNING]
> Depending on what revision of nixpkgs your system uses, this may result in binaries that differ from the above `packages` output
> If this is the case, you will not be able to use the binary cache

Example:

```nix
{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    prismlauncher = {
      url = "github:PrismLauncher/PrismLauncher";

      # Optional: Override the nixpkgs input of prismlauncher to use the same revision as the rest of your flake
      # Note that this may break the reproducibility mentioned above, and you might not be able to access the binary cache
      #
      # inputs.nixpkgs.follows = "nixpkgs";

      # This is not required for Flakes
      inputs.flake-compat.follows = "";
    };
  };

  outputs =
    { nixpkgs, prismlauncher, ... }:
    {
      nixosConfigurations.foo = nixpkgs.lib.nixosSystem {
        modules = [
          ./configuration.nix

          (
            { pkgs, ... }:
            {
              nixpkgs.overlays = [ prismlauncher.overlays.default ];

              environment.systemPackages = [ pkgs.prismlauncher ];
            }
          )
        ];
      };
    };
}
```

### Installing the package ad-hoc (`nix shell`, `nix run`, etc.)

You can simply call the default package of this flake.

Example:

```shell
nix run github:PrismLauncher/PrismLauncher

nix shell github:PrismLauncher/PrismLauncher

nix profile install github:PrismLauncher/PrismLauncher
```

## Installing a development release (without flakes)

We use [Cachix](https://cachix.org/) to cache our development and release builds.
If you want to avoid rebuilds you may add the Cachix bucket to your substitutors.

Example (NixOS):

```nix
{
  nix.settings = {
    trusted-substituters = [ "https://prismlauncher.cachix.org" ];

    trusted-public-keys = [
      "prismlauncher.cachix.org-1:9/n/FGyABA2jLUVfY+DEp4hKds/rwO+SCOtbOkDzd+c="
    ];
  };
}
```

### Installing the package directly (`fetchTarball`)

We use flake-compat to allow using this Flake on a system that doesn't use flakes.

Example:

```nix
{ pkgs, ... }:
{
  environment.systemPackages = [
    (import (
      builtins.fetchTarball "https://github.com/PrismLauncher/PrismLauncher/archive/develop.tar.gz"
    )).packages.${pkgs.system}.prismlauncher
  ];
}
```

### Using the overlay (`fetchTarball`)

Alternatively, if you don't want to use our `packages` output, you can add our overlay to your instance of nixpkgs.
This results in Prism using your system's libraries

Example:

```nix
{ pkgs, ... }:
{
  nixpkgs.overlays = [
    (import (
      builtins.fetchTarball "https://github.com/PrismLauncher/PrismLauncher/archive/develop.tar.gz"
    )).overlays.default
  ];

  environment.systemPackages = [ pkgs.prismlauncher ];
}
```

### Installing the package ad-hoc (`nix-env`)

You can add this repository as a channel and install its packages that way.

Example:

```shell
nix-channel --add https://github.com/PrismLauncher/PrismLauncher/archive/develop.tar.gz prismlauncher

nix-channel --update prismlauncher

nix-env -iA prismlauncher.prismlauncher
```

## Package variants

Both Nixpkgs and this repository offer the following packages:

- `prismlauncher` - The preferred build, wrapped with everything necessary to run the launcher and Minecraft
- `prismlauncher-unwrapped` - A minimal build that allows for advanced customization of the launcher's runtime environment

### Customizing wrapped packages

The wrapped package (`prismlauncher`) offers some build parameters to further customize the launcher's environment.

The following parameters can be overridden:

- `additionalLibs` (default: `[ ]`) Additional libraries that will be added to `LD_LIBRARY_PATH`
- `additionalPrograms` (default: `[ ]`) Additional libraries that will be added to `PATH`
- `controllerSupport` (default: `isLinux`) Turn on/off support for controllers on Linux (macOS will always have this)
- `gamemodeSupport` (default: `isLinux`) Turn on/off support for [Feral GameMode](https://github.com/FeralInteractive/gamemode) on Linux
- `jdks` (default: `[ jdk21 jdk17 jdk8 ]`) Java runtimes added to `PRISMLAUNCHER_JAVA_PATHS` variable
- `msaClientID` (default: `null`, requires full rebuild!) Client ID used for Microsoft Authentication
- `textToSpeechSupport` (default: `isLinux`) Turn on/off support for text-to-speech on Linux (macOS will always have this)
- `withWaylandGLFW` (default: `isLinux`) Build with support for native Wayland via a custom GLFW
