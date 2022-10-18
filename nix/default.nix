{ stdenv
, lib
, fetchFromGitHub
, cmake
, ninja
, jdk8
, jdk
, ghc_filesystem
, zlib
, file
, wrapQtAppsHook
, xorg
, libpulseaudio
, qtbase
, quazip
, libGL
, msaClientID ? ""
, extraJDKs ? [ ]
, extra-cmake-modules

  # flake
, self
, version
, libnbtplusplus
, tomlplusplus
, enableLTO ? false
}:

let
  # Libraries required to run Minecraft
  libpath = with xorg; lib.makeLibraryPath [
    libX11
    libXext
    libXcursor
    libXrandr
    libXxf86vm
    libpulseaudio
    libGL
  ];

  # This variable will be passed to Minecraft by Prism Launcher
  gameLibraryPath = libpath + ":/run/opengl-driver/lib";

  javaPaths = lib.makeSearchPath "bin/java" ([ jdk jdk8 ] ++ extraJDKs);
in

stdenv.mkDerivation rec {
  pname = "prismlauncher";
  inherit version;

  src = lib.cleanSource self;

  nativeBuildInputs = [ cmake extra-cmake-modules ninja jdk ghc_filesystem file wrapQtAppsHook ];
  buildInputs = [ qtbase quazip zlib ];

  dontWrapQtApps = true;

  postUnpack = ''
    # Copy libnbtplusplus
    rm -rf source/libraries/libnbtplusplus
    mkdir source/libraries/libnbtplusplus
    cp -a ${libnbtplusplus}/* source/libraries/libnbtplusplus
    chmod a+r+w source/libraries/libnbtplusplus/*
    # Copy tomlplusplus
    rm -rf source/libraries/tomlplusplus
    mkdir source/libraries/tomlplusplus
    cp -a ${tomlplusplus}/* source/libraries/tomlplusplus
    chmod a+r+w source/libraries/tomlplusplus/*
  '';

  cmakeFlags = [
    "-GNinja"
    "-DLauncher_QT_VERSION_MAJOR=${lib.versions.major qtbase.version}"
  ] ++ lib.optionals enableLTO [ "-DENABLE_LTO=on" ]
    ++ lib.optionals (msaClientID != "") [ "-DLauncher_MSA_CLIENT_ID=${msaClientID}" ];

  # we have to check if the system is NixOS before adding stdenv.cc.cc.lib (#923)
  postInstall = ''
    # xorg.xrandr needed for LWJGL [2.9.2, 3) https://github.com/LWJGL/lwjgl/issues/128
    wrapQtApp $out/bin/prismlauncher \
      --run '[ -f /etc/NIXOS ] && export LD_LIBRARY_PATH="${stdenv.cc.cc.lib}/lib:$LD_LIBRARY_PATH"' \
      --prefix LD_LIBRARY_PATH : ${gameLibraryPath} \
      --prefix PRISMLAUNCHER_JAVA_PATHS : ${javaPaths} \
      --prefix PATH : ${lib.makeBinPath [ xorg.xrandr ]}
  '';

  meta = with lib; {
    homepage = "https://prismlauncher.org/";
    downloadPage = "https://prismlauncher.org/download/";
    changelog = "https://github.com/PrismLauncher/PrismLauncher/releases";
    description = "A free, open source launcher for Minecraft";
    longDescription = ''
      Allows you to have multiple, separate instances of Minecraft (each with
      their own mods, texture packs, saves, etc) and helps you manage them and
      their associated options with a simple interface.
    '';
    platforms = platforms.unix;
    license = licenses.gpl3Only;
    maintainers = with maintainers; [ starcraft66 kloenk ];
  };
}
