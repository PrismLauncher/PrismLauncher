# Building the flatpak

### 1. Install tools

Install `flatpak` and `flatpak-builder` from your distribution repositories.

### 2. Install SDK

`flatpak install org.kde.Sdk org.kde.Platform`  
Pick version `5.15`

### 3. Get the yml flatpak file

You can download it directly from github, or clone the repo.

### 4. Build it

```
flatpak-builder --ccache --force-clean flatbuild packages/flatpak/org.polymc.PolyMC.yml
```

If you didn't clone the repo, the path might be different.

### 5. Install the custom build

```
flatpak-builder --user --install --ccache --force-clean flatbuild packages/org.polymc.PolyMC.yml
```
