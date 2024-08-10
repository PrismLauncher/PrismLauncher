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

        alejandra.enable = true;
        deadnix.enable = true;
        nil.enable = true;

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

        echo ""
        echo -e "\e[1m\e[32mPrism Launcher:\e[0m\e[32m Use 'env LAUNCHER_DIR=path/to/cmake/install/prefix prismlauncher' to run your local builds with wrapped libraries.\e[0m"
        echo -e "\e[1m\e[32mPrism Launcher:\e[0m\e[32m Example: 'env LAUNCHER_DIR=\$PWD/install prismlauncher'\e[0m"
        echo ""
      '';

      inputsFrom = [config.packages.prismlauncher-unwrapped];
      buildInputs = with pkgs; [ccache ninja config.packages.prismlauncher-launchscript];
    };

    formatter = pkgs.alejandra;
  };
}
