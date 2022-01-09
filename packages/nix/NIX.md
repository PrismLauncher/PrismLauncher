# How to import

To import with flakes use
```nix
inputs = {
  polymc.url = "github:PolyMC/PolyMC";
};

...

nixpkgs.overlays = [ inputs.polymc.overlay.${system} ]; ## Within configuration.nix
environment.systemPackages = with pkgs; [ polymc ]; ##
```

To import without flakes use channels:

```
nix-channel --add https://github.com/PolyMC/PolyMC/archive/master.tar.gz polymc
nix-channel --update polymc
nix-env -iA polymc
```

or alternatively you can use

```
nixpkgs.overlays = [
  (import (builtins.fetchTarball "https://github.com/lourkeur/PolyMC/archive/develop.tar.gz")).overlay
];

environment.systemPackages = with pkgs; [ polymc ];
```