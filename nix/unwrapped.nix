{
  lib,
  stdenv,
  cmake,
  cmark,
  extra-cmake-modules,
  gamemode,
  jdk17,
  kdePackages,
  libnbtplusplus,
  ninja,
  qrencode,
  self,
  stripJavaArchivesHook,
  tomlplusplus,
  zlib,
  msaClientID ? null,
  libarchive,
}:

let
  date =
    let
      # YYYYMMDD
      date' = lib.substring 0 8 self.lastModifiedDate;
      year = lib.substring 0 4 date';
      month = lib.substring 4 2 date';
      date = lib.substring 6 2 date';
    in
    if (self ? "lastModifiedDate") then
      lib.concatStringsSep "-" [
        year
        month
        date
      ]
    else
      "unknown";
in

stdenv.mkDerivation {
  pname = "prismlauncher-unwrapped";
  version = "10.0-unstable-${date}";

  src = lib.fileset.toSource {
    root = ../.;
    fileset = lib.fileset.unions [
      ../CMakeLists.txt
      ../COPYING.md

      ../buildconfig
      ../cmake
      ../launcher
      ../libraries
      ../program_info
      ../tests
    ];
  };

  postUnpack = ''
    rm -rf source/libraries/libnbtplusplus
    ln -s ${libnbtplusplus} source/libraries/libnbtplusplus
  '';

  nativeBuildInputs = [
    cmake
    ninja
    extra-cmake-modules
    jdk17
    stripJavaArchivesHook
  ];

  buildInputs = [
    cmark
    kdePackages.qtbase
    kdePackages.qtnetworkauth
    qrencode
    libarchive
    tomlplusplus
    zlib
  ]
  ++ lib.optional stdenv.hostPlatform.isLinux gamemode;

  cmakeFlags = [
    # downstream branding
    (lib.cmakeFeature "Launcher_BUILD_PLATFORM" "nixpkgs")
  ]
  ++ lib.optionals (msaClientID != null) [
    (lib.cmakeFeature "Launcher_MSA_CLIENT_ID" (toString msaClientID))
  ]
  ++ lib.optionals stdenv.hostPlatform.isDarwin [
    # we wrap our binary manually
    (lib.cmakeFeature "INSTALL_BUNDLE" "nodeps")
    # disable built-in updater
    (lib.cmakeFeature "MACOSX_SPARKLE_UPDATE_FEED_URL" "''")
    (lib.cmakeFeature "CMAKE_INSTALL_PREFIX" "${placeholder "out"}/Applications/")
  ];

  doCheck = true;

  dontWrapQtApps = true;

  meta = {
    description = "Free, open source launcher for Minecraft";
    longDescription = ''
      Allows you to have multiple, separate instances of Minecraft (each with
      their own mods, texture packs, saves, etc) and helps you manage them and
      their associated options with a simple interface.
    '';
    homepage = "https://prismlauncher.org/";
    license = lib.licenses.gpl3Only;
    maintainers = with lib.maintainers; [
      Scrumplex
      getchoo
    ];
    mainProgram = "prismlauncher";
    platforms = lib.platforms.linux ++ lib.platforms.darwin;
  };
}
