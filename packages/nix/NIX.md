# How to import

To import with flakes use
```nix
inputs = {
  polymc.url = "github:PolyMC/PolyMC";
};

...

nixpkgs.overlays = [ inputs.polymc.overlay.${system} ]; ## Within configuration.nix
```

To import without flakes use channels:

```
sudo -i nix-channel --add https://github.com/PolyMC/PolyMC/archive/master.tar.gz polymc
sudo -i nix-channel --update polymc
```
add `<polymc>` to imports in your `configuration.nix`

```nix
imports = [
  "<polymc>/packages/nix/overlay.nix"
];
```