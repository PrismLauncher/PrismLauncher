<p align="center">
  <img src="https://avatars2.githubusercontent.com/u/5411890" alt="MultiMC logo"/>
</p>

MultiMC 5
=========

MultiMC is a custom launcher for Minecraft that allows you to easily manage multiple installations of Minecraft at once. It also allows you to easily install and remove mods by simply dragging and dropping. Here are the current [features](https://github.com/MultiMC/MultiMC5/wiki#features) of MultiMC.


## Development
The project uses C++ and Qt5 as the language and base framework. This might seem odd in the Minecraft community, but allows using 25MB of RAM, where other tools use an excessive amount of resources for no reason.

We can do more, with less, on worse hardware and leave more resources for the game while keeping the launcher running and providing extra features.

If you want to contribute, either talk to us on [Discord](https://discord.gg/0k2zsXGNHs0fE4Wm), [IRC](http://webchat.esper.net/?nick=&channels=MultiMC)(esper.net/#MultiMC) or pick up some item from [workflowy](https://workflowy.com/s/2EyDMcp7CU) - there are many.

### Building
If you want to build MultiMC yourself, check [BUILD.md](BUILD.md) for build instructions.

The ci server is running at [ci.multimc.org](http://ci.multimc.org), where you can watch the builds happen in (or very close to) real time. It can also serve as a nice reference on how to set up the build environment on your end.

According to travis.ci, the builds are currently [![Build Status](https://travis-ci.org/MultiMC/MultiMC5.svg?branch=develop)](https://travis-ci.org/MultiMC/MultiMC5)

### Code formatting
We use [Clang Format](http://clang.llvm.org/docs/ClangFormat.html) to format the project. We highly recommend setting it up so the project stays well formatted.


## Translations
Translations can be done either directly in the [translations repository](https://github.com/MultiMC/MultiMC5). For more details, see: [Translating-MultiMC](https://github.com/MultiMC/MultiMC5/wiki/Translating-MultiMC).

## Forking/Redistributing
We keep MultiMC open source because we think it's important to be able to see the source code for a project like this, and we do so using the Apache license.

Part of the reason for using the Apache license is we don't want people using the "MultiMC" name when redistributing the project. This means people must take the time to go through the source code and remove all references to "MultiMC", including but not limited to the project icon and the title of windows, (no *MultiMC-fork* in the title).

Apache covers reasonable use for the name - a mention of the project's origins in the About dialog and the license is acceptable. However, it should be abundantly clear that the project is a fork *without* implying that you have our blessing.


## License
Copyright &copy; 2013-2019 MultiMC Contributors

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this program except in compliance with the License. You may obtain a copy of the License at [http://www.apache.org/licenses/LICENSE-2.0](http://www.apache.org/licenses/LICENSE-2.0).

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
