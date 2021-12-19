<p align="center">
  <img src="https://avatars2.githubusercontent.com/u/5411890" alt="MultiMC logo"/>
</p>

MultiMC 5
=========

MultiMC is a custom launcher for Minecraft that focuses on predictability, long term stability and simplicity.

## Development
If you want to contribute, talk to us on [Discord](https://discord.gg/multimc) first.

While blindly submitting PRs is definitely possible, they're not necessarily going to get accepted.

We aren't looking for flashy features, but expanding upon the existing feature set without distruption or endangering future viability of the project is OK.

### Building
If you want to build MultiMC yourself, check [BUILD.md](BUILD.md) for build instructions.

### Code formatting
Just follow the existing formatting.

In general, in order of importance:
* Make sure your IDE is not messing up line endings or whitespace and avoid using linters.
* Prefer readability over dogma.
* Keep to the existing formatting.
* Indent with 4 space unless it's in a submodule.
* Keep lists (of arguments, parameters, initializers...) as lists, not paragraphs. It should either read from top to bottom, or left to right. Not both.


## Translations
Translations can be done [on crowdin](https://translate.multimc.org). Please avoid making direct pull requests to the translations repository.

## Forking/Redistributing/Custom builds policy
We keep Launcher open source because we think it's important to be able to see the source code for a project like this, and we do so using the Apache license.

Part of the reason for using the Apache license is that we don't want people using the "MultiMC" name when redistributing the project. This means people must take the time to go through the source code and remove all references to "MultiMC", including but not limited to the project icon and the title of windows, (no *MultiMC-fork* in the title).

Apache covers reasonable use for the name - a mention of the project's origins in the About dialog and the license is acceptable. However, it should be abundantly clear that the project is a fork *without* implying that you have our blessing.


## License
Copyright &copy; 2013-2021 MultiMC Contributors

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this program except in compliance with the License. You may obtain a copy of the License at [http://www.apache.org/licenses/LICENSE-2.0](http://www.apache.org/licenses/LICENSE-2.0).

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

## Build status
### Linux (Intel32)
<a href="https://teamcity.multimc.org/viewType.html?buildTypeId=Launcher_Launcher_Linux32_Build&guest=1">
Build: <img src="https://teamcity.multimc.org/app/rest/builds/buildType:(id:Launcher_Launcher_Linux32_Build)/statusIcon"/>
</a>
<a href="https://teamcity.multimc.org/viewType.html?buildTypeId=Launcher_Launcher_Linux32_Deploy&guest=1">
Deploy: <img src="https://teamcity.multimc.org/app/rest/builds/buildType:(id:Launcher_Launcher_Linux32_Deploy)/statusIcon"/>
</a>

### Linux (AMD64)
<a href="https://teamcity.multimc.org/viewType.html?buildTypeId=Launcher_Launcher_Linux64_Build&guest=1">
Build: <img src="https://teamcity.multimc.org/app/rest/builds/buildType:(id:Launcher_Launcher_Linux64_Build)/statusIcon"/>
</a>
<a href="https://teamcity.multimc.org/viewType.html?buildTypeId=Launcher_Launcher_Linux64_Deploy&guest=1">
Deploy: <img src="https://teamcity.multimc.org/app/rest/builds/buildType:(id:Launcher_Launcher_Linux64_Deploy)/statusIcon"/>
</a>

### macOS (AMD64)
<a href="https://teamcity.multimc.org/viewType.html?buildTypeId=Launcher_Launcher_MacOS_Build&guest=1">
Build: <img src="https://teamcity.multimc.org/app/rest/builds/buildType:(id:Launcher_Launcher_MacOS_Build)/statusIcon"/>
</a>
<a href="https://teamcity.multimc.org/viewType.html?buildTypeId=Launcher_Launcher_MacOS_Deploy&guest=1">
Deploy: <img src="https://teamcity.multimc.org/app/rest/builds/buildType:(id:Launcher_Launcher_MacOS_Deploy)/statusIcon"/>
</a>

### Windows (Intel32)
<a href="https://teamcity.multimc.org/viewType.html?buildTypeId=Launcher_Launcher_Windows_Build&guest=1">
Build: <img src="https://teamcity.multimc.org/app/rest/builds/buildType:(id:Launcher_Launcher_Windows_Build)/statusIcon"/>
</a>
<a href="https://teamcity.multimc.org/viewType.html?buildTypeId=Launcher_Launcher_Windows_Deploy&guest=1">
Deploy: <img src="https://teamcity.multimc.org/app/rest/builds/buildType:(id:Launcher_Launcher_Windows_Deploy)/statusIcon"/>
</a>
