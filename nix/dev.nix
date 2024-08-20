{
  perSystem =
    { pkgs, self', ... }:
    {
      devShells.default = pkgs.mkShell {
        inputsFrom = [ self'.packages.prismlauncher-unwrapped ];
        buildInputs = with pkgs; [
          ccache
          ninja
        ];
      };

      formatter = pkgs.nixfmt-rfc-style;
    };
}
