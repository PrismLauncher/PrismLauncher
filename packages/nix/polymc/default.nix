{ lib
, mkDerivation
, fetchFromGitHub
, makeDesktopItem
, substituteAll
, fetchpatch
, cmake
, jdk8
, jdk
, zlib
, file
, makeWrapper
, xorg
, libpulseaudio
, qtbase
, libGL
# submodules
, isFlakeBuild ? false
, self ? ""
, submoduleNbt ? ""
, submoduleQuazip ? ""
}:

let
  gameLibraryPath = with xorg; lib.makeLibraryPath [
    libX11
    libXext
    libXcursor
    libXrandr
    libXxf86vm
    libpulseaudio
    libGL
  ];
in

mkDerivation rec {
  pname = "polymc";
  version = if isFlakeBuild then "nightly" else "1.0.4";

  src = if isFlakeBuild then lib.cleanSource self
    else fetchFromGitHub {
    owner = "PolyMC";
    repo = "PolyMC";
    rev = "${version}";
    sha256 = "sha256-8aya0KfV9F+i2qBpweWcR9hwyTSQkqn2wHdtkCEeNvk=";
    fetchSubmodules = true;
  };

  nativeBuildInputs = [ cmake file makeWrapper ];
  buildInputs = [ qtbase jdk8 zlib ];

  postUnpack = if isFlakeBuild then ''
    mkdir source/libraries/{libnbtplusplus,quazip}
    cp -a ${submoduleNbt}/* source/libraries/libnbtplusplus
    cp -a ${submoduleQuazip}/* source/libraries/quazip
    chmod a+r+w source/libraries/{libnbtplusplus,quazip}/*
  '' else "";

  cmakeFlags = [
    "-DLauncher_LAYOUT=lin-system"
  ];

  desktopItem = makeDesktopItem {
    name = "polymc";
    exec = "polymc";
    icon = "polymc";
    desktopName = "PolyMC";
    genericName = "Minecraft Launcher";
    comment = "A custom launcher for Minecraft";
    categories = "Game;";
    extraEntries = ''
      Keywords=game;Minecraft;
    '';
  };

  postInstall = ''
    install -Dm644 ../launcher/resources/multimc/scalable/launcher.svg $out/share/pixmaps/multimc.svg
    install -Dm755 ${desktopItem}/share/applications/polymc.desktop -t $out/share/applications
    # xorg.xrandr needed for LWJGL [2.9.2, 3) https://github.com/LWJGL/lwjgl/issues/128
    wrapProgram $out/bin/polymc \
      --set GAME_LIBRARY_PATH /run/opengl-driver/lib:${gameLibraryPath} \
      --prefix PATH : ${lib.makeBinPath [ xorg.xrandr jdk ]}
  '';
}
