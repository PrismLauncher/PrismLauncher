{
  perSystem = {
    config,
    lib,
    pkgs,
    ...
  }: {
    pre-commit.settings = {
      hooks = {
        markdownlint.enable = true;

        deadnix.enable = true;
        nil.enable = true;
        nixfmt.enable = true;

        clang-format = {
          enable = true;
          types_or = ["c" "c++" "java" "json" "objective-c"];
        };
      };

      tools.clang-tools = lib.mkForce pkgs.clang-tools_18;
    };

    devShells.default = pkgs.mkShell {
      shellHook = ''
        ${config.pre-commit.installationScript}
      '';

      inputsFrom = [config.packages.prismlauncher-unwrapped];
      buildInputs = with pkgs; [ccache ninja];
    };

    formatter = pkgs.nixfmt-rfc-style;
  };
}
