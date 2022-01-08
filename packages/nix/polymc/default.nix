{ lib, mkDerivation, fetchFromGitHub, cmake, jdk8, jdk, zlib, file, makeWrapper, xorg, libpulseaudio, qtbase, libGL, msaClientID ? "" }:

let
  libpath = with xorg; lib.makeLibraryPath [ libX11 libXext libXcursor libXrandr libXxf86vm libpulseaudio libGL ];
in 
mkDerivation rec {
  pname = "polymc";
  version = "1.0.4";
  src = fetchFromGitHub {
    owner = "PolyMC";
    repo = "PolyMC";
    rev = "${version}";
    sha256 = "sha256-8aya0KfV9F+i2qBpweWcR9hwyTSQkqn2wHdtkCEeNvk=";
    fetchSubmodules = true;
  };
  nativeBuildInputs = [ cmake file makeWrapper ];
  buildInputs = [ qtbase jdk8 zlib ];

  patches = [ ./0001-pick-latest-java-first.patch ];

  postPatch = ''
    # hardcode jdk paths
    substituteInPlace launcher/java/JavaUtils.cpp \
      --replace 'scanJavaDir("/usr/lib/jvm")' 'javas.append("${jdk}/lib/openjdk/bin/java")' \
      --replace 'scanJavaDir("/usr/lib32/jvm")' 'javas.append("${jdk8}/lib/openjdk/bin/java")'
  '';

  cmakeFlags = [ "-DLauncher_LAYOUT=lin-system" ];

  postInstall = ''
    # xorg.xrandr needed for LWJGL [2.9.2, 3) https://github.com/LWJGL/lwjgl/issues/128
    wrapProgram $out/bin/polymc \
      --set GAME_LIBRARY_PATH /run/opengl-driver/lib:${libpath} \
      --prefix PATH : ${lib.makeBinPath [ xorg.xrandr ]}

    substituteInPlace $out/share/applications/org.polymc.PolyMC.desktop --replace 'Exec=' 'Exec=${placeholder "out"}/bin/polymc'
  '';

  meta = with lib; {
    homepage = "https://github.com/PolyMC/PolyMC";
    description = "A free, open source launcher for Minecraft";
    longDescription = ''
      Allows you to have multiple, separate instances of Minecraft (each with their own mods, texture packs, saves, etc) and helps you manage them and their associated options with a simple interface.
    '';
    platforms = platforms.linux;
    license = licenses.gpl3;
    maintainers = with maintainers; [ cidkid ];
  };
}