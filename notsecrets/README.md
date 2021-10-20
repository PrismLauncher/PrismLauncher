# DevLauncher (Not) Secrets

This is a dummy implementation of MultiMC's _Secrets_ library, used to store private information needed for:
- Application name and logo (and branding in general)
- Various URLs and API endpoints
- Your Microsoft Identity Platform client ID. **This is required to use Microsoft accounts to play!**
  - If omitted, adding Microsoft accounts will be completely disabled.

## MultiMC development

In its current state, the `notsecrets` library is suitable for MultiMC code contributions (the code builds as `DevLauncher`).

All you have to do is add the Microsoft client ID. See `Secrets.cpp` for details.

## Forking

Forks of this project that intend to distribute binaries to users should use their own implementation of this library that does not impersonate MultiMC in any way (see the Apache 2.0 license, common sense and trademark law).

A fork is a serious business, especially if you intend to distribute binaries to users. This is because you need to agree to the Microsoft identity platform Terms of Use:

https://docs.microsoft.com/en-us/legal/microsoft-identity-platform/terms-of-use

If you truly want to accept such an agreement, a starting point is to copy `notsecrets` to `secrets`, enable the `Launcher_EMBED_SECRETS` CMake option and customize the files.

We do not want confused users asking for help with your fork in MultiMC Discord or similar locations.
