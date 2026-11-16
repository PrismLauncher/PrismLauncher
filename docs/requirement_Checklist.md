Technical Quality

Group ID: Group F

[x] Syntax & Execution: Course CI runs on a clean `ubuntu-24.04` runner. The build job (checkout, Qt and vcpkg setup, CMake configure, Debug build, CTest, install, artifact upload) succeeded in [run 36435815684](https://github.com/Colby-Frison/PrismLauncher/actions/runs/36435815684) and again in the strict-boundary run [37239138792](https://github.com/Colby-Frison/PrismLauncher/actions/runs/37239138792). The workflow uses `actions/checkout@v7` and `actions/upload-artifact@v7`. Shell steps are short command lines, not scripts assembled from unpinned downloads. With the greylist removed, the architecture job fails until the tree matches the proposed design; the build job can still succeed in parallel.

[x] Task Completeness: The build job configures, compiles, runs `ctest --preset linux`, installs with `cmake --install`, and uploads `prismlauncher-linux-debug`. The architecture-boundaries job runs `tools/check_architecture_boundaries.py` and the checker unit tests. None of those steps is an `echo` placeholder.

[x] Architectural Conformance: The target diagram separates tasks and networking, the instance and account domain, provider adapters, and UI. `architecture-boundaries` enforces those rules with no greylist. Any forbidden include fails the job. [Run 37239138792](https://github.com/Colby-Frison/PrismLauncher/actions/runs/37239138792) shows that red gate: 17 cross-layer includes (domain/net into UI) failed Architecture boundaries while Build and test (Linux) still passed, and the workflow conclusion was `failure`. `Launcher_logic` stays one library because the architecture notes say target extraction comes after service interfaces exist.

[x] Pipeline Maintainability: `.github/workflows/course-ci.yml` is two jobs with pinned action tags. The boundary job is checkout plus two `python3` commands and does not install Qt. The build job still uses the repository's existing setup action.

---

AI-Assisted Verification

[x] Origin Tracking: The first pipeline and the boundary scan were drafted by the assistant from the architecture docs and the checklist. The group chose the layer rules, kept the upstream workflows disabled, rejected a four-library split, and later removed the baseline greylist so CI matches the proposed design rather than ratifying current debt. That history is in `docs/ci-cd-pipeline-report.md`.

[x] Security & Action Auditing: The workflow file adds only `actions/checkout@v7` and `actions/upload-artifact@v7`. Both are published GitHub actions at major versions already used in this repository. The boundary job sets `contents: read` and calls no third-party action. The build job keeps `contents: read` and `packages: write` for the vcpkg cache. It does not request an id token and does not read signing secrets. No step pipes a remote script into a shell.

[x] Defect & Hallucination Remediation: The checker accepts only `--root`. That flag is implemented in the script and covered by `tests/architecture/test_boundaries.py` (illegal include, legal UI include, clean tree). Local unit tests pass; a local run of the checker against the real tree exits nonzero with the violation list.

[x] Team Comprehension: On a push to `develop` or a pull request, the boundary job and the Linux build start together. The boundary job fails if any forbidden include remains. The build job fails if configure, compile, CTest, install, or the artifact upload fails. A boundary failure does not skip the build, so the group still gets compile and test evidence while the architecture gate stays red. The overall workflow conclusion is `failure` when either job fails.

[x] Human Oversight Gates: This workflow does not deploy to production and does not use credentials. The uploaded install tree is a build artifact, not a release. A release that signs or publishes a package would be a separate, human-triggered workflow. There is no exception list for architectural debt; clearing a red gate requires removing the forbidden include and a human review of that change.