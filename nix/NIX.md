# How to import

To import with flakes use

```nix
{
  inputs = {
    prismlauncher.url = "github:PrismLauncher/PrismLauncher";
  };

...

  nixpkgs.overlays = [ inputs.prismlauncher.overlay ]; ## Within configuration.nix
  environment.systemPackages = with pkgs; [ prismlauncher ]; ##
}
```

To import without flakes use channels:

```sh
nix-channel --add https://github.com/PrismLauncher/PrismLauncher/archive/master.tar.gz prismlauncher
nix-channel --update prismlauncher
nix-env -iA prismlauncher
```

or alternatively you can use

```nix
{
  nixpkgs.overlays = [
    (import (builtins.fetchTarball "https://github.com/PrismLauncher/PrismLauncher/archive/develop.tar.gz")).overlay
  ];

  environment.systemPackages = with pkgs; [ prismlauncher ];
}
```
