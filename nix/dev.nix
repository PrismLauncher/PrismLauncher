{
  inputs,
  self,
  ...
}: {
  perSystem = {
    system,
    pkgs,
    ...
  }: {
    checks = {
      pre-commit-check = inputs.pre-commit-hooks.lib.${system}.run {
        src = self;
        hooks = {
          markdownlint.enable = true;

          alejandra.enable = true;
          deadnix.enable = true;
          nil.enable = true;

          clang-format = {
            enable = true;
            types_or = ["c" "c++"];
          };
        };
      };
    };

    devShells.default = pkgs.mkShell {
      inherit (self.checks.${system}.pre-commit-check) shellHook;
      packages = with pkgs; [
        nodePackages.markdownlint-cli
        alejandra
        deadnix
        clang-tools
        nil
      ];

      inputsFrom = [self.packages.${system}.prismlauncher-unwrapped];
      buildInputs = with pkgs; [ccache ninja];
    };

    formatter = pkgs.alejandra;
  };
}
