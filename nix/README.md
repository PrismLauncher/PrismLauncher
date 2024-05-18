# Prism Launcher Nix Packaging

## Installing a stable release (nixpkgs)

Prism Launcher is packaged in [nixpkgs](https://github.com/NixOS/nixpkgs/) since 22.11.

See [Package variants](#package-variants) for a list of available packages.

## Installing a development release (flake)

We use [garnix](https://garnix.io/) to build and cache our development builds.
If you want to avoid rebuilds you may add the garnix cache to your substitutors, or use `--accept-flake-config`
to temporarily enable it when using `nix` commands.

Example (NixOS):

```nix
{...}:
{
  nix.settings = {
    trusted-substituters = [
      "https://cache.garnix.io"
    ];

    trusted-public-keys = [
      "cache.garnix.io:CTFPyKSLcx5RMJKfLo5EEPUObbA78b0YQ2DTCJXqr9g="
    ];
  };
}
```

### Using the overlay

After adding `github:PrismLauncher/PrismLauncher` to your flake inputs, you can add the `default` overlay to your nixpkgs instance.

Example:

```nix
{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    prismlauncher = {
      url = "github:PrismLauncher/PrismLauncher";
      # Optional: Override the nixpkgs input of prismlauncher to use the same revision as the rest of your flake
      # Note that overriding any input of prismlauncher may break reproducibility
      # inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = {nixpkgs, prismlauncher}: {
    nixosConfigurations.foo = nixpkgs.lib.nixosSystem {
      system = "x86_64-linux";

      modules = [
        ({pkgs, ...}: {
          nixpkgs.overlays = [prismlauncher.overlays.default];

          environment.systemPackages = [pkgs.prismlauncher];
        })
      ];
    };
  }
}
```

### Installing the package directly

Alternatively, if you don't want to use an overlay, you can install Prism Launcher directly by installing the `prismlauncher` package.
This way the installed package is fully reproducible.

Example:

```nix
{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    prismlauncher = {
      url = "github:PrismLauncher/PrismLauncher";
      # Optional: Override the nixpkgs input of prismlauncher to use the same revision as the rest of your flake
      # Note that overriding any input of prismlauncher may break reproducibility
      # inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = {nixpkgs, prismlauncher}: {
    nixosConfigurations.foo = nixpkgs.lib.nixosSystem {
      system = "x86_64-linux";

      modules = [
        ({pkgs, ...}: {
          environment.systemPackages = [prismlauncher.packages.${pkgs.system}.prismlauncher];
        })
      ];
    };
  }
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

We use [garnix](https://garnix.io/) to build and cache our development builds.
If you want to avoid rebuilds you may add the garnix cache to your substitutors.

Example (NixOS):

```nix
{...}:
{
  nix.settings = {
    trusted-substituters = [
      "https://cache.garnix.io"
    ];

    trusted-public-keys = [
      "cache.garnix.io:CTFPyKSLcx5RMJKfLo5EEPUObbA78b0YQ2DTCJXqr9g="
    ];
  };
}
```

### Using the overlay (`fetchTarball`)

We use flake-compat to allow using this Flake on a system that doesn't use flakes.

Example:

```nix
{pkgs, ...}: {
  nixpkgs.overlays = [(import (builtins.fetchTarball "https://github.com/PrismLauncher/PrismLauncher/archive/develop.tar.gz")).overlays.default];

  environment.systemPackages = [pkgs.prismlauncher];
}
```

### Installing the package directly (`fetchTarball`)

Alternatively, if you don't want to use an overlay, you can install Prism Launcher directly by installing the `prismlauncher` package.
This way the installed package is fully reproducible.

Example:

```nix
{pkgs, ...}: {
  environment.systemPackages = [(import (builtins.fetchTarball "https://github.com/PrismLauncher/PrismLauncher/archive/develop.tar.gz")).packages.${pkgs.system}.prismlauncher];
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

- `prismlauncher` - Preferred build using Qt 6
- `prismlauncher-qt5` - Legacy build using Qt 5 (i.e. for Qt 5 theming support)

Both of these packages also have `-unwrapped` counterparts, that are not wrapped and can therefore be customized even further than what the wrapper packages offer.

### Customizing wrapped packages

The wrapped packages (`prismlauncher` and `prismlauncher-qt5`) offer some build parameters to further customize the launcher's environment.

The following parameters can be overridden:

- `msaClientID` (default: `null`, requires full rebuild!) Client ID used for Microsoft Authentication
- `gamemodeSupport` (default: `true`) Turn on/off support for [Feral GameMode](https://github.com/FeralInteractive/gamemode)
- `jdks` (default: `[ jdk17 jdk8 ]`) Java runtimes added to `PRISMLAUNCHER_JAVA_PATHS` variable
- `additionalLibs` (default: `[ ]`) Additional libraries that will be added to `LD_LIBRARY_PATH`
