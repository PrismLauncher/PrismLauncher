# MultiMC 0.4.8

Fluffy and functional!

## **IMPORTANT**

- GH-1402: MultiMC will keep its binary filename after an update if you rename it.

  Note that this doesn't happen with this (0.4.8) update yet, because the old update method is still used.

  If you renamed `MultiMC.exe` for any reason, you will have to manually remove the renamed file after the update and rename the new `MultiMC.exe`.

  Future updates should no longer have this issue.


## New featueres

- GH-1047, GH-1233: MultiMC now includes basic Minecraft world management.

  This is a new page in the console/edit instance window.

  You can:
  - Copy worlds
  - Delete worlds
  - Copy the world seed value
  - Run MCEdit - the MCEdit feature has been moved here.

- GH-1217: MultiMC now tracks instance play time and displays it when the instance is selected.

- New buttons on the top toolbar:
  - GH-1238: button for the [MultiMC subreddit](https://www.reddit.com/r/MultiMC/).
  - GH-1397: button for joining the [MultiMC discord voice/chat server](https://discord.gg/0k2zsXGNHs0fE4Wm).

    Both are there for you to interact with other MultiMC users and us.

- GH-253, GH-1300: MultiMC can now be started with the `-l "Instance ID"` parameter, launching the specified instance directly.

## Improvements

- Instance list
  - GH-1121: Instances are now selected after you create them.
  - GH-93: When copying an instance, you can tell MultiMC to not copy the worlds.

- Mod and resource pack lists
  - GH-1237: Mod info is now clickable and selectable.
  - GH-1322: Mod description `...` link will no longer pop up multiple dialogs.
  - GH-1178: When dragged and dropped, folder based mods and resource packs will be copied properly on OSX.

- MCEdit integration:
  - GH-1009: MCEdit Unified on linux is now recognized properly.

- Mojang login and accounts:
  - GH-1158: An unique ID is generated on the MultiMC side before login, instead of letting the server decide.
  - When a password is required, the user login is partially obscured.
  - Dropdown menu on the main window now lists profiles, not accounts.

- Modpacks:
  - GH-1140: Modpack downloads now check for update on the server even if the file is alreaddy locally available.
  - GH-1148: When creating an instance from modpack, the instance name will be guessed based on the modpack file or URL (unless you set it yourself).
  - GH-1280: While importing modpacks, the progress dialog now says what is happening.
  - When selecting the modpack fiels in the new instance dialog, the contents are selected for easy replacement.

- Instance settings
  - Wrapper commands now use the proper UI field and do not get replaced with pre-launch commands.

- Minecraft launching:
  - GH-1053, GH-1338: Minecraft launching has been completely redone.
  - GH-1275: Server resource pack folder is created on launch.
    - This is a workaround for Minecraft bug MCL-3732.
  - GH-1320: Improve compatibility with non-Oracle Java.
  - GH-1355: MMC environment will no longer leak into Minecraft after MultiMC updates itself.

- Minecraft log:
  - Java exception detection in Minecraft logs has been improved.
  - GH-719: You can now use your own [paste.ee](https://paste.ee/) account for uploading logs.
    - New [paste.ee](https://paste.ee/) settings page has been added to the global settings dialog.
  - GH-1197: Text colors in log window now adapt to the background color.
  - GH-1164: The censor filter could be initialized with empty values, leading to unreadable log.
  - GH-1008, GH-1046, GH-1067: Log size limiting.
    
    The log window now has a configurable limit for the number of lines remembered. You can also specify whether it stops logging or forgets on the fly once the limit is breached.

    This prevents the MultiMC log window from using too much memory on logging. The default limit is 100000 lines and the logging stops.

    Minecraft logging this much is a sign of a problem that needs to be fixed. Any complaints should be addressed to the responsible mod authors.

- Screenshot upload
  - GH-1339: While uploading screenshots, the console window will not close (prevents a crash).

- Other logs:
  - GH-926: 'Other Logs' now has a button for removing all log files.
  - Hidden log files are shown in 'Other logs'.

- User skins:
  - MultiMC now uses [crafatar.com](https://crafatar.com/) for skin downloads instead of the Mojang servers.

- Java:
  - GH-1365: MultiMC now supports Java 9 and its new version numbering.
  - GH-1262: Java can now be placed in a folder relative to MultiMC's folder. This allows bundling of JREs with MultiMC.

- Translations:
  - GH-1313: Some parts of the MultiMC user interface have been marked as 'not for translation'.

## Internals and internal bug fixes

- GH-1052: All the dependencies were rebuilt and the build environment upgraded to the latest compiler versions.
- GH-1051: The CDPATH environment variable is now ignored.
- GH-77, GH-1059, GH-1060: The MultiMC updater is no longer used or necessary.

  It is only present to preserve compatibility with previous versions.
  Updates now work properly on Windows systems when you have unicode (like ❄, Ǣ or Ω) characters in the path.

- GH-1069, GH-1100: `LD_LIBRARY_PATH` and `LD_PRELOAD` environment variables supplied to MultiMC now don't affect MultiMC, but affect the launched game.

  This means you can use something like the Steam overlay in MultiMC instances on linux.
  If you need to use these variables for MultiMC itself, you can use `MMC_LIBRARY_PATH` and `MMC_PRELOAD` instead.

- GH-1389: External processes (like folder views, editors, etc.) are now started in a way that prevents the MultiMC environment to be reused by them.
- GH-1355: If `LD_LIBRARY_PATH` contains any of MultiMC's internal folders, this will not propagate to started processes.
- GH-1231, GH-1378: libpng is now included with the linux version of MultiMC
- GH-1202: SSL certificates are now rebuilt on start on OSX.

- GH-1303: Translations and notification cache are stored in the normal data folder now, not alongside the binaries. This only affects third party linux packaging.
- GH-1266, GH-1301,
- GH-1360: Development and other unstable versions of MultiMC now uses github commits instead of this manually maintained changelog.

# Previous releases

## 0.4.7

This is what 0.4.6 should have been. Oh well, at least it's here now!

### Functional changes
- GH-974: A copy of the libstdc++ library is now included in linux releases, improving compatibility
- GH-985: Jar mods are now movable and removable after adding
- GH-983: Use `minecraft.jar` as the main jar when using jar mods - fixes NEI in Legacy Minecraft versions
- GH-977: Fix FTB paths on Windows

   This removes some very old compatibility code. If you get any issues, make sure you run the FTB Launcher and let it update its files.
- GH-992 and GH-1003: Improved performance when saving settings:
  - Bad performance was caused by improved data consistency
  - Each config file is now saved only once, not once for every setting
  - When loading FTB instances, there are no writes to config files anymore
- GH-991: Implemented wrapper command functionality:

  There is an extra field in the MultiMC Java settings that allows running Java inside a wrapper program or script. This means you can run Minecraft with wrappers like `optirun` and get better performance with hybrid graphics on linux without workarounds.
- GH-997: Fixed saving of multi-line settings. This fixes notes.
- GH-967: It is now possible to add patches (Forge and LiteLoader) to tracked FTB instances properly.

  Libraries added by the patches will be taken from MultiMC's `libraries` folder, while the tracked patches will use FTB's folders.

- GH-1011 and GH-1015: Fixed various issues when the patch versions aren't complete

  This applies when Minecraft versions are missing or when patches are broken and the profile is manipulated by adding, moving, removing, customizing and reverting patches.

- GH-1021: Builtin legacy Minecraft versions aren't customizable anymore

   The internal format for Legacy Minecraft versions does not translate to the external patch format and would cause crashes
- GH-1016: MultiMC prints a list of mods, coremods (contents of the coremods folder) and jar mods to the log on instance start. This should help with troubleshooting.
- GH-1031: Icons are exported and imported along with instances

    This only applies if the icon was custom (not built-in) when exporting and the user doesn't choose an icon while importing the pack.

### UI changes
- GH-970: Fixed help button for the External tools and Accounts dialog pages not linking to the proper wiki places
  - Same for the Versions dialog page

- GH-994: Rearranged the buttons on the Versions page to make jar mods less prominent

  Using the `Add jar mods` button will also show a nag dialog until it's been used successfully

## 0.4.6

Long time coming, this release brought a lot of incremental improvements and fixes.

### Functional changes
- Old version.json and custom.json version files will be transformed into a minecraft version patch:
  - The process is automated
  - LWJGL entries are stripped from the original file - you may have to re-do LWJGL version customizations
  - Old files will be renamed - .old extension is added
- It's now possible to:
  - Customize, edit and revert builtin version patches (Minecraft, LWJGL)
  - Edit custom version patches (Forge, LiteLoader, other)
- Blocked various environment variables from affecting Minecraft:
  - `JAVA_ARGS`
  - `CLASSPATH`
  - `CONFIGPATH`
  - `JAVA_HOME`
  - `JRE_HOME`
  - `_JAVA_OPTIONS`
  - `JAVA_OPTIONS`
  - `JAVA_TOOL_OPTIONS`
  - If you rely on those in any way, now would be a time to fix that
- Improved handling of LWJGL on OSX (.dylib vs. .jnilib extensions)
- Jar mods are now always put into a generated temporary Minecraft jar instead of being put on the classpath
- PermGen settings:
  - Changed default PermGen value to 128M because of many issues from new users
  - MultiMC now recognizes the Java version used and will not add PermGen settings to Java >= 1.8
- Implemented simple modpack import and export feature:
  - Export allows selecting which files go into the resulting zip archive
  - Only MultiMC instances for now, other pack formats are planned
  - Import is either from local file or URL, URL can't have ad/click/pay gates
- Instance copy doesn't follow symlinks on Linux anymore
  - Still does on Windows because copying symlinks requires Administrator level access
- Instance delete doesn't follow symlinks anymore - anywhere
- MCEdit tool now recognizes MCEdit2.exe as a valid file to runtime
- Log uploads now follow the maximum allowed paste sizes of paste.ee and are encoded properly
- MultiMC now doesn't use a proxy by default
- Running profilers now works on Windows
- MultiMC will warn you if you run it from WinRAR or temporary folders
- Minecraft process ID is printed in the log on start
- SSL certificates are fixed on OSX 10.10.3 and newer - see [explanation](http://www.infoworld.com/article/2911209/mac-os-x/yosemite-10103-breaks-some-applications-and-https-sites.html).

### UI changes
- Version lists:
  - All version lists now include latest and recommended versions - recommended are pre-selected
  - Java version list now sorts versions based on suitability - best on top
  - Forge version list includes the development branch the version came from
  - Minecraft list marks latest release as 'recommended' and latest snapshot as 'latest', if it is newer than the release
- Mod lists:
  - Are updated and sorted after adding mods
  - Browse buttons now properly open the central mods folder
  - Are no longer watching for updates when the user doesn't look at them
  - Loader mod list now recognizes .litemod files as valid mod files
- Improved wording of instance delete dialog
- Icon themes:
  - Can be changed without restarting
  - Added a workaround for icon themes broken in KDE Plasma 5 (only relevant for custom builds)
- Status icons:
  - Included a 'yellow' one
  - Are clickable and link to [help.mojang.com](https://help.mojang.com/)
  - Refresh when the icon theme does
- Changed default console font to Courier 10pt on Windows
- Description text in the main window status bar now updates when Minecraft version is changed
- Inserted blatant self-promotion (Only Minecraft 1.8 and up)
  - This adds a bit of unobtrusive flavor text to the Minecraft F3 screen
- Log page now has a button to scroll to bottom
- Errors are reported while updating the instance in the Version page
- Fixed typos (forge -> Forge)

### Internals
- Massive internal restructuring (ongoing)
- Downloads now follow redirects
- Minecraft window size is now always at least 1x1 pixel (prevents crash from bad settings)
- Better handling of Forge downloads (obviously invalid/broken files are redownloaded)
- All download tasks now only start 6 downloads, using a queue (fixes issues with assets downloads)
- Fixed bugs related to corrupted settings files (settings and patch order file saves are now atomic)
- Updated zip manipulation library - files inside newly written zip/jar files should have proper access rights and timestamps
- Made Minecraft resource downloads more resilient (throwing away invalid/broken index files)
- Minecraft asset import from old format has been removed
- Generally improved MultiMC logging:
  - More error logging for network tasks
  - Added timestamps relative to application start
- Fixed issue with the application getting stuck in a modal dialog when screenshot uploads fail
- Instance profiles and patches are now loaded lazily (speeds up MultiMC start)
- Groups are saved after copying an instance
- MultiMC launcher part will now exit cleanly when MultiMC crashes or is closed during instance launch


## 0.4.5
- Copies of FTB instances should work again (GH-619)
- Fixed OSX version not including the hotfix number
- If the currectly used java version goes missing, it now triggers auto-detect (GH-608)
- Improved 'refresh' and 'update check' icons of the dark and bright simple icon themes (GH-618)
- Fixed console window hiding - it no longer results in windowless/unusable MultiMC

## 0.4.4
- Other logs larger than 10MB will not load to prevent logs eating the whole available memory
- Translations are now updated independently from MultiMC
- Added new and reworked the old simple icon themes
- LWJGL on OSX should no longer clash with Java 8
- Update to newer Qt version
  - Look and feel updated for latest OSX
- Fixed issues caused by Minecraft inheriting the environment variables from MultiMC
- Minecraft log improvements:
  - Implemented search and pause
  - Automated coloring is updated for log format used by Minecraft 1.7+
  - Added settings for the font used in the console, using sensible defaults for the OS
- Removed MultiMC crash handler, it will be replaced by a better one in the future

## 0.4.3
- Fix for issues with Minecraft version file updates
- Fix for console window related memory leak
- Fix for travis.ci build

## 0.4.2
- Show a warning in the log if a library is missing
- Fixes for relocating instances to other MultiMC installs:
  - Libraries now use full Gradle dependency specifiers
  - Rework of forge installer (forge can reinstall itself using only the information already in the instance)
  - Fixed bugs in rarely used library insertion rules
- Make the global settings dialog into a page dialog
- Check if the Java binary can be found before launch
- Show a warning for paths containing a '!' (Java can't handle that properly)
- Many smaller fixes

## 0.4.1
- Fix LWJGL version list (SourceForge has changed the download API)

## 0.4.0
- Jar support in 1.6+
- Deprecated legacy instances
  - Legacy instances can still be used but not created
  - All Minecraft versions are supported in the new instance format
- All instance editing and settings dialogs were turned into pages
  - The edit instance dialog contains pages relevant to editing and settings
  - The console window contains pages useful when playing the game
- Redone the screenshot management and upload (page)
- Added a way to display and manage log files and crash reports generated by Minecraft (page)
- Added measures to prevent corruption of version files
  - Minecraft version files are no longer part of the instances by default
- Added help for the newly added dialog pages
- Made logs uploaded to paste.ee expire after a month
- Fixed a few bugs related to liteloader and forge (1.7.10 issues)
- Icon themes. Two new themes where added (work in progress)
- Changelog and update channel are now visible in the update dialog
- Several performance improvements to the group view
- Added keyboard navigation to the group view

## 0.3.9
- Workaround for 1.7.10 Forge

## 0.3.8
- Workaround for performance issues with Intel integrated graphics chips

## 0.3.7
- Fixed forge for 1.7.10-pre4 (and any future prereleases)

## 0.3.6
- New server status - now with more color
- Fix for FTB tracking issues
- Fix for translations on OSX not working
- Screenshot dialog should be harder to lose track of when used from the console window
- A crash handler implementation has been added.

## 0.3.5
- More versions are now selectable when changing instance versions
- Fix for Forge/FML changing its mcmod.info metadata format

## 0.3.4
- Show a list of Patreon patrons in credits section of the about dialog
- Make the console window raise itself after minecraft closes
- Add Control/Command+q shortcut to quit from the main window
- Add french translation
- Download and cache FML libs for legacy versions
- Update the OS X icon
- Fix FTB libraries not being used properly

## 0.3.3
- Tweak context menu to prevent accidental clicks
- Fix adding icons to custom icon directories
- Added a Patreon button to the toolbar
- Minecraft authentication tasks now provide better error reports

## 0.3.2
- Fix issues with libraries not getting replaced properly (fixes instance startup for new instances)
- Fix april fools

## 0.3.1
- Fix copying of FTB instances (instance type is changed properly now)
- Customizing FTB pack versions will remove the FTB pack patch file

## 0.3
- Improved instance view
- Overhauled 1.6+ version loading
- Added a patch system for instance modification
  - There is no longer a single custom.json file that overrides version.json
  - Instead there are now "patch" files in <instance>/patches/, one for each main tweaker (forge, liteloader etc.)
  - These patches are applied after version.json in a customisable order,
  - A list of these files is shown in the left most tab in the Edit Mods dialog, where a list of libraries was shown before.
  - custom.json can still be used for overriding everything.
- Offline mode can be used even when online
- Show an "empty" message in version selector dialogs
- Fix FTB paths on windows
- Tooling support
  - JProfiler
  - JVisualVM
  - MCEdit
- Don't assume forge in FTB instances and allow other libraries (liteloader, mcpatcher, etc.) in FTB instances
- Screenshot uploading/managing
- Instance badges
- Some pre/post command stuff (remove the timeout, variable substitution)
- Fix logging when the system language is not en_US
- Setting PermGen to 64 will now omit the java parameter because it is the default
- Fix encoding of escape sequences (tabs and newlines) in config files

## 0.2.1
- Hotfix - move the native library extraction into the onesix launcher part.

## 0.2
- Java memory settings have MB added to the number to make the units obvious.
- Complete rework of the launcher part. No more sensitive information in the process arguments.
- Cached downloads now do not destroy files on failure.
- Mojang service status is now on the MultiMC status bar.
- Java checker is no longer needed/used on instance launch.
- Support for private FTB packs.
- Fixed instance ID issues related to copying FTB packs without changing the instance name.
- Forge versions are better sorted (build numbers above 999 were sorted wrong).
- Fixed crash related to the MultiMC update channel picker in offline mode.
- Started using icon themes for the application icons, fixing many OSX graphical glitches.
- Icon sources have been located, along with icon licenses.
- Update to the German translation.

## 0.1.1
- Hotfix - Changed the issue tracker URL to [GitHub issues](https://github.com/MultiMC/MultiMC5/issues).

## 0.1
- Reworked the version numbering system to support our [new Git workflow](http://nvie.com/posts/a-successful-git-branching-model/).
- Added a tray icon for the console window.
- Fixed instances getting deselected after FTB instances are loaded (or whenever the model is reset).
- Implemented proxy settings.
- Fixed sorting of Java installations in the Java list.
- Jar files are now distributed separately, rather than being extracted from the binary at runtime.
- Added additional information to the about dialog.

## 0.0
- Initial release.
