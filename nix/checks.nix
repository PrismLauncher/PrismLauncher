{
  runCommand,
  deadnix,
  llvmPackages_18,
  markdownlint-cli,
  nixfmt-rfc-style,
  statix,
  self,
}:
{
  formatting =
    runCommand "check-formatting"
      {
        nativeBuildInputs = [
          deadnix
          llvmPackages_18.clang-tools
          markdownlint-cli
          nixfmt-rfc-style
          statix
        ];
      }
      ''
        cd ${self}

        echo "Running clang-format...."
        clang-format --dry-run --style='file' --Werror */**.{c,cc,cpp,h,hh,hpp}

        echo "Running deadnix..."
        deadnix --fail

        echo "Running markdownlint..."
        markdownlint --dot .

        echo "Running nixfmt..."
        nixfmt --check .

        echo "Running statix"
        statix check .

        touch $out
      '';
}
