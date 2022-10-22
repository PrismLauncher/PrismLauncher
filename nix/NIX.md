# Running on Nix

## Putting it in your system configuration

### On flakes-enabled nix

#### Directly installing

The `prismlauncher` flake provides a package which you can install along with
the rest of your packages

```nix
# In your flake.nix:
{
  inputs = {
    prismlauncher.url = "github:PrismLauncher/PrismLauncher";
  };
}
```

```nix
# And in your system configuration:
environment.systemPackages = [ prismlauncher.packages.${pkgs.system}.prismlauncher ];

# Or in your home-manager configuration:
home.packages = [ prismlauncher.packages.${pkgs.system}.prismlauncher ];
```

#### Using the overlay

Alternatively, you can overlay the prismlauncher version in nixpkgs which will
allow you to install using `pkgs` as you normally would while also using the
latest version

```nix
# In your flake.nix:
{
  inputs = {
    prismlauncher.url = "github:PrismLauncher/PrismLauncher";
  };
}
```

```nix
# And in your system configuration:
nixpkgs.overlays = [ inputs.prismlauncher.overlay ];
environment.systemPackages = [ pkgs.prismlauncher ];

# Or in your home-manager configuration:
config.nixpkgs.overlays = [ inputs.prismlauncher.overlay ];
home.packages = [ pkgs.prismlauncher ];
```

### Without flakes-enabled nix

#### Using channels

```sh
nix-channel --add https://github.com/PrismLauncher/PrismLauncher/archive/master.tar.gz prismlauncher
nix-channel --update prismlauncher
nix-env -iA prismlauncher
```

#### Using the overlay

```nix
# In your configuration.nix:
{
  nixpkgs.overlays = [
    (import (builtins.fetchTarball "https://github.com/PrismLauncher/PrismLauncher/archive/develop.tar.gz")).overlay
  ];

  environment.systemPackages = with pkgs; [ prismlauncher ];
}
```

## Running ad-hoc

If you're on a flakes-enabled nix you can run the launcher in one-line

```sh
nix run github:PrismLauncher/PrismLauncher
```
