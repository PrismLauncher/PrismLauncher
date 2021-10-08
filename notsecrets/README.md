# (Not) Secrets

This is a dummy implementation of MultiMC's _Secrets_ library, used to store private information needed for certain functions of MMC in a secure manner
(obfuscated or encrypted).

Currently, this library contains:
- Your Microsoft Authentication app's client ID. **This is required to use Microsoft accounts to play!**
  - If omitted, adding Microsoft accounts will be completely disabled.

Forks of this project should provide their own implementation of this library to enable these functions properly.
