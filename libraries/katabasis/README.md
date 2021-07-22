# Katabasis - MS-flavoerd OAuth for Qt, derived from the O2 library

This library's sole purpose is to make interacting with MSA and various MSA and XBox authenticated services less painful.

It may be possible to backport some of the changes to O2 in the future, but for the sake of going fast, all compatibility concerns have been ignored.

[You can find the original library's git repository here.](https://github.com/pipacs/o2)

Notes to contributors:

   * Please follow the coding style of the existing source, where reasonable
   * Code contributions are released under Simplified BSD License, as specified in LICENSE. Do not contribute if this license does not suit your code
   * If you are interested in working on this, come to the MultiMC Discord server and talk first

## Installation

Clone the Github repository, integrate the it into your CMake build system.

The library is static only, dynamic linking and system-wide installation are out of scope and undesirable.

## Usage

At this stage, don't, unless you want to help with the library itself.

This is an experimental fork of the O2 library and is undergoing a big design/architecture shift in order to support different features:

* Multiple accounts
* Multi-stage authentication/authorization schemes
* Tighter control over token chains and their storage
* Talking to complex APIs and individually authorized microservices
* Token lifetime management, 'offline mode' and resilience in face of network failures
* Token and claims/entitlements validation
* Caching of some API results
* XBox magic
* Mojang magic
* Generally, magic that you would spend weeks on researching while getting confused by contradictory/incomplete documentation (if any is available)
