# PolyMC (Not) Secrets

This is a dummy implementation of PolyMC's _Secrets_ library, used to store private information needed for:
- Application name and logo (and branding in general)
- Various URLs and API endpoints
- Your Microsoft Identity Platform client ID. **This is required to use Microsoft accounts to play!**
  - If omitted, adding Microsoft accounts will be completely disabled.

## MultiMC development

In its current state, the `notsecrets` library is suitable for PolyMC code contributions.

All you have to do is add the Microsoft client ID. See `Secrets.cpp` for details.
