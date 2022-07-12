# How to import

To import with flakes use
```nix
{
  inputs = {
    polymc.url = "github:PolyMC/PolyMC";
  };

...

  nixpkgs.overlays = [ inputs.polymc.overlay ]; ## Within configuration.nix
  environment.systemPackages = with pkgs; [ polymc ]; ##
}
```

To import without flakes use channels:

```sh
nix-channel --add https://github.com/PolyMC/PolyMC/archive/master.tar.gz polymc
nix-channel --update polymc
nix-env -iA polymc
```

or alternatively you can use

```nix
{
  nixpkgs.overlays = [
    (import (builtins.fetchTarball "https://github.com/PolyMC/PolyMC/archive/develop.tar.gz")).overlay
  ];

  environment.systemPackages = with pkgs; [ polymc ];
}
```
