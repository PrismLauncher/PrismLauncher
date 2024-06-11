{
  stdenvNoCC,
  bash,
  # these are passed by the wrapper
  # we don't use them
  msaClientID ? null,
  gamemodeSupport ? null,
}:
stdenvNoCC.mkDerivation {
  pname = "prismlauncher-launch-script";
  version = "0";

  src = ../../launcher/Launcher.in;

  nativeBuildInputs = [bash];

  dontConfigure = true;
  dontBuild = true;

  unpackCmd = "install -D $curSrc source/Launcher.in";

  postPatch = ''
    substitute Launcher.in prismlauncher \
      --replace-fail "@Launcher_APP_BINARY_NAME@" "prismlauncher" \
      --replace-fail "@LIB_SUFFIX@" ""
  '';

  installPhase = ''
    runHook preInstall

    install -t $out/bin/ -Dm755 prismlauncher

    runHook postInstall
  '';

  passthru = {
    # Make deadnix stop warning about these
    inherit msaClientID gamemodeSupport;
  };
}
