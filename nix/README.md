# Prism Launcher Nix Packaging

## Installing a stable release (nixpkgs)

Prism Launcher is packaged in [nixpkgs](https://github.com/NixOS/nixpkgs/) since 22.11.

Check the [NixOS Wiki](https://wiki.nixos.org/wiki/Prism_Launcher) for up-to-date instructions, including how to configure the package with `override`.

## Installing a development release (flake)

We use [Cachix](https://cachix.org/) to cache our development and release builds.
If you want to avoid rebuilds you may add the Cachix bucket to your substituters, or use `--accept-flake-config`
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
              environment.systemPackages = [ prismlauncher.packages.${pkgs.stdenv.hostPlatform.system}.prismlauncher ];
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
> Depending on what revision of nixpkgs your system uses, this may result in binaries that differ from the above `packages` output.
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
If you want to avoid rebuilds you may add the Cachix bucket to your substituters.

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
    )).packages.${pkgs.stdenv.hostPlatform.system}.prismlauncher
  ];
}
```

### Using the overlay (`fetchTarball`)

Alternatively, if you don't want to use our `packages` output, you can add our overlay to your instance of nixpkgs.
This results in Prism using your system's libraries.

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

The wrapped package (`prismlauncher`) can be customized with `override`.
The available parameters (`additionalLibs`, `additionalPrograms`, `controllerSupport`, `gamemodeSupport`, `jdks`, `msaClientID` and `textToSpeechSupport`) and examples are documented on the [NixOS Wiki](https://wiki.nixos.org/wiki/Prism_Launcher#Useful_options).
The wiki describes the nixpkgs package, so its defaults can differ from this repository's. The defaults used here are defined in [`nix/wrapper.nix`](wrapper.nix) and apply to nixpkgs as well when this repository's overlay is used.
