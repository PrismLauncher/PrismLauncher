// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Tayou <tayou@gmx.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "GameOptionsSchema.h"
#include <QObject>
#include <memory>

QMap<QString, std::shared_ptr<GameOption>> GameOptionsSchema::knownOptions;
QList<std::shared_ptr<KeyBindData>> GameOptionsSchema::keyboardButtons;

QMap<QString, std::shared_ptr<GameOption>>* GameOptionsSchema::getKnownOptions()
{
    if (knownOptions.isEmpty()) {
        populateInternalOptionList();
    }

    return &knownOptions;
}

QList<std::shared_ptr<KeyBindData>>* GameOptionsSchema::getKeyBindOptions()
{
    if (keyboardButtons.isEmpty()) {
        populateInternalKeyBindList();
    }

    return &keyboardButtons;
}

/// @brief this data is mostly copied from https://minecraft.fandom.com/wiki/Options.txt
/// the order of options here should be the same order as in the file
void GameOptionsSchema::populateInternalOptionList()
{
    // clang-format off
    
    // data version
    knownOptions["version"]                 = std::make_shared<GameOption>(0, "Data version of the client version this file was last saved in; used for upgrading default settings. (numeric)", true);

    // general options
    knownOptions["autoJump"]                = std::make_shared<GameOption>(false, "Whether auto-jump is enabled");
    knownOptions["autoSuggestions"]         = std::make_shared<GameOption>(true,  "True if brigadier's command suggestion UI should always be shown, instead of just when pressing tab");
    knownOptions["chatColors"]              = std::make_shared<GameOption>(true,  "Whether colored chat is allowed");
    knownOptions["chatLinks"]               = std::make_shared<GameOption>(true,  "Whether links show as links or just text in the chat");
    knownOptions["chatLinksPrompt"]         = std::make_shared<GameOption>(true,  "Whether clicking on links in chat needs confirmation before opening them");
    knownOptions["enableVsync"]             = std::make_shared<GameOption>(true,  "Whether v-sync (vertical synchronization) is enabled");
    knownOptions["entityShadows"]           = std::make_shared<GameOption>(true,  "Whether to display entity shadows");
    knownOptions["forceUnicodeFont"]        = std::make_shared<GameOption>(false, "Whether Unicode font should be used");
    knownOptions["discrete_mouse_scroll"]   = std::make_shared<GameOption>(false, "Ignores scrolling set by operating system");
    knownOptions["invertYMouse"]            = std::make_shared<GameOption>(false, "Whether mouse is inverted or not");
    knownOptions["realmsNotifications"]     = std::make_shared<GameOption>(true,  "Whether Realms invites are alerted on the main menu");
    knownOptions["reducedDebugInfo"]        = std::make_shared<GameOption>(false, "Whether to show reduced information on the Debug screen");
    knownOptions["showSubtitles"]           = std::make_shared<GameOption>(false, "If subtitles are shown");
    knownOptions["directionalAudio"]        = std::make_shared<GameOption>(false, "If Directional Audio is enabled");
    knownOptions["touchscreen"]             = std::make_shared<GameOption>(false, "Whether touchscreen controls are used");
    knownOptions["fullscreen"]              = std::make_shared<GameOption>(false, "Whether the game attempts to go fullscreen at startup");
    knownOptions["bobView"]                 = std::make_shared<GameOption>(true,  "Whether or not the camera bobs up and down as the player walks");
    knownOptions["toggleCrouch"]            = std::make_shared<GameOption>(false, "Whether the sneak key must be pressed or held to activate sneaking");
    knownOptions["toggleSprint"]            = std::make_shared<GameOption>(false, "Whether the sprint key must be pressed or held to activate sprinting");
    knownOptions["darkMojangStudiosBackground"] = std::make_shared<GameOption>(false, "Whether the Mojang Studios loading screen will appear monochrome");
    knownOptions["hideLightningFlashes"]    = std::make_shared<GameOption>(false, "Hide lightning flashes (visual effect)");

    knownOptions["mouseSensitivity"]        = std::make_shared<GameOption>(0.5,   "How much a mouse movement changes the position of the camera", Range<float>{ 0.0, 1.0 });
    // FOV: The in-game value is counted in degrees, however, the options.txt isn't. The value is converted into degrees with the following formula: degrees = 40 * value + 70
    knownOptions["fov"]                     = std::make_shared<GameOption>(0.0,   "How large the field of view is", Range<float>{ -1.0, 1.0 });
    knownOptions["screenEffectScale"]       = std::make_shared<GameOption>(1.0,   "Distortion Effects (how intense the effects of Nausea and nether portals are)", Range<float>{ 0.0, 1.0 });
    knownOptions["fovEffectScale"]          = std::make_shared<GameOption>(1.0,   "FOV Effects (how much the field of view changes when sprinting, having Speed or Slowness etc.)", Range<float>{ 0.0, 1.0 });
    knownOptions["darknessEffectScale"]     = std::make_shared<GameOption>(1.0,   "Darkness Pulsing (how much the Darkness effect pulses)", Range<float>{ 0.0, 1.0 });
    knownOptions["gamma"]                   = std::make_shared<GameOption>(0.5,   "Brightness", Range<float>{ 0.0, 1.0 });
    knownOptions["renderDistance"]          = std::make_shared<GameOption>(12,    "The render distance in the chunk radius from the player - Note: The Maximum and default in vanilla is 32 and 12, 16 and 8 on 32bit machines", Range<int>{ 2, 64 });
    knownOptions["simulationDistance"]      = std::make_shared<GameOption>(12,    "The simulation distance in the chunk radius from the player - Note: The Maximum and default in vanilla is 32 and 12, 16 and 8 on 32bit machines", Range<int>{ 5, 32 });
    knownOptions["entityDistanceScaling"]   = std::make_shared<GameOption>(1.0,   "The maximum distance from the player that entities render", Range<float>{ 0.5, 5.0 });
    knownOptions["guiScale"]                = std::make_shared<GameOption>(0,     "Size of interfaces - Note: 0 (Auto) or 1+ for size. Upper limit based on window resolution");
    knownOptions["particles"]               = std::make_shared<GameOption>(0,     "Amount of particles (such as rain, potion effects, etc.)", Range<int>{ 0, 2 });
    knownOptions["maxFps"]                  = std::make_shared<GameOption>(120,   "The maximum framerate", Range<int>{ 10, 260 });
    knownOptions["difficulty‌"]              = std::make_shared<GameOption>(2,     "Has no effect after 1.7.2", Range<int>{ 0, 3 });
    knownOptions["graphicsMode"]            = std::make_shared<GameOption>(1,     "Whether Fast (less detailed), Fancy (more detailed), or Fabulous! (most detailed) graphics are turned on", Range<int>{ 0, 2 });
    knownOptions["ao"]                      = std::make_shared<GameOption>(true,  "Smooth lighting (Ambient Occlusion)");
    knownOptions["prioritizeChunkUpdates"]  = std::make_shared<GameOption>(0,     "Chunk section update strategy", Range<int>{ 0, 2 });
    knownOptions["biomeBlendRadius"]        = std::make_shared<GameOption>(2,     "Radius for which biome blending should happen", Range<int>{ 0, 7 });
    knownOptions["renderClouds"]            = std::make_shared<GameOption>("true", "Whether to display clouds", QStringList{ "true", "false", "fast" });

    knownOptions["resourcePacks"]           = std::make_shared<GameOption>("[]",  "A list of active resource packs", QStringList(), false);
    knownOptions["incompatibleResourcePacks"] = std::make_shared<GameOption>("[]", "A list of active resource packs that are marked as incompatible with the current Minecraft version.", QStringList(), false);

    knownOptions["lastServer"]              = std::make_shared<GameOption>("",    "Address of last server used with Direct Connection");
    knownOptions["lang"]                    = std::make_shared<GameOption>("en_us", "Language to be used");
    knownOptions["soundDevice"]             = std::make_shared<GameOption>("",    "Sound device to be used");
    knownOptions["chatVisibility"]          = std::make_shared<GameOption>(0,     "What is seen in chat", Range<int>{ 0, 2 });
    knownOptions["chatOpacity"]             = std::make_shared<GameOption>(1.0,   "Opacity of the chat", Range<float>{ 0, 1 });
    knownOptions["chatLineSpacing"]         = std::make_shared<GameOption>(0.0,   "Spacing between text in chat", Range<float>{ 0, 1 });
    knownOptions["textBackgroundOpacity"]   = std::make_shared<GameOption>(0.5,   "Opacity of text background", Range<float>{ 0, 1 });
    knownOptions["backgroundForChatOnly"]   = std::make_shared<GameOption>(true,  "Toggles if the background is only in chat or if it's everywhere");
    knownOptions["hideServerAddress"]       = std::make_shared<GameOption>(false, "Has no effect in modern versions");
    knownOptions["advancedItemTooltips"]    = std::make_shared<GameOption>(false, "Whether hovering over items in the inventory shows its ID and durability; toggled by pressing F3 + H");
    knownOptions["pauseOnLostFocus"]        = std::make_shared<GameOption>(true,  "Whether switching out of Minecraft without pressing Esc or opening an in-game interface automatically pauses the game; toggled by pressing F3 + P");
    knownOptions["overrideWidth"]           = std::make_shared<GameOption>(0,     "Width to open Minecraft with in pixels (0 means default to the Minecraft settings); no in-game control");
    knownOptions["overrideHeight"]          = std::make_shared<GameOption>(0,     "Height to open Minecraft with in pixels (0 means default to the Minecraft settings); no in-game control");
    knownOptions["heldItemTooltips"]        = std::make_shared<GameOption>(true,  "Whether switching between items shows the name of the item; no in-game control");
    knownOptions["chatHeightFocused"]       = std::make_shared<GameOption>(1.0,   "How tall the chat span is", Range<float>{ 0, 1 });
    knownOptions["chatDelay"]               = std::make_shared<GameOption>(0.0,   "How much delay there is between text", Range<float>{ 0, 6 });
    knownOptions["chatHeightUnfocused"]     = std::make_shared<GameOption>(0.4375, "How tall the maximum chat span is, when the chat button is not pressed", Range<float>{ 0, 1 });
    knownOptions["chatScale"]               = std::make_shared<GameOption>(1.0,   "The scale/size of the text in the chat", Range<float>{ 0, 1 });
    knownOptions["chatWidth"]               = std::make_shared<GameOption>(1.0,   "The span width of the chat", Range<float>{ 0, 1 });
    knownOptions["mipmapLevels"]            = std::make_shared<GameOption>(4,     "Amount by which distant textures are smoothed", Range<int>{0, 4});
    knownOptions["useNativeTransport"]      = std::make_shared<GameOption>(true,  "Whether to use a Netty EpollSocketChannel for connections to servers instead of a NioSocketChannel (only applies if EPOLL is available on the user's system)");
    knownOptions["mainHand"]                = std::make_shared<GameOption>("right", "Whether the main hand appears as left or right", QStringList{ "left", "right" });
    knownOptions["attackIndicator"]         = std::make_shared<GameOption>(1,     "When hitting, how the attack indicator is shown on screen", Range<int>{ 0, 2 });
    knownOptions["narrator"]                = std::make_shared<GameOption>(0,     "Setting of the Narrator", Range<int>{ 0, 3 });
    knownOptions["tutorialStep"]            = std::make_shared<GameOption>("movement", "Next stage of tutorial hints to display",
             QStringList{ "movement", "find_tree", "punch_tree", "open_inventory", "craft_planks", "none" });
    knownOptions["mouseWheelSensitivity"]   = std::make_shared<GameOption>(1.0f,  "Allows making the mouse wheel more sensitive (see MC-123773)", Range<float>{ 1.0f, 10.0f });
    knownOptions["rawMouseInput"]           = std::make_shared<GameOption>(true,  "Ignores acceleration set by the operating system");
    knownOptions["glDebugVerbosity"]        = std::make_shared<GameOption>(1,     "LWJGL log info level (only on some machines)", Range<int>{ 0, 4 });
    knownOptions["skipMultiplayerWarning"]  = std::make_shared<GameOption>(true,  "Whether to skip the legal disclaimer when entering the multiplayer screen");
    knownOptions["skipRealms32bitWarning"]  = std::make_shared<GameOption>(true,  "Whether to skip the 32-bit environment warning when entering the Realms screen");
    knownOptions["hideMatchedNames"]        = std::make_shared<GameOption>(true,  "Some servers send chat messages in non-standard formats. With this option on, the game will attempt to apply chat hiding anyway by matching the text in messages.");
    knownOptions["joinedFirstServer"]       = std::make_shared<GameOption>(true,  "Whether the player has joined a server before. If false, the Social Interactions tutorial hint will appear when joining a server.");
    knownOptions["hideBundleTutorial"]      = std::make_shared<GameOption>(true,  "Whether the player has seen the bundle tutorial hint when trying to use a bundle.");
    knownOptions["syncChunkWrites"]         = std::make_shared<GameOption>(true,  "Whether to open region files in synchronous mode");
    knownOptions["showAutosaveIndicator"]   = std::make_shared<GameOption>(true,  "Whether to show autosave indicator on the right-bottom of the screen");
    knownOptions["allowServerListing"]      = std::make_shared<GameOption>(true,  "Whether to allow player's ID to be shown in the player list shown on the multiplayer screen");

    // Keys Binds
    knownOptions["key_key.attack"]              = std::make_shared<GameOption>("key.mouse.left",      "Attack control");
    knownOptions["key_key.use"]                 = std::make_shared<GameOption>("key.mouse.right",     "Use Item control");
    knownOptions["key_key.forward"]             = std::make_shared<GameOption>("key.keyboard.w",      "Forward control ");
    knownOptions["key_key.left"]                = std::make_shared<GameOption>("key.keyboard.a",      "Left control");
    knownOptions["key_key.back"]                = std::make_shared<GameOption>("key.keyboard.s",      "Back control");
    knownOptions["key_key.right"]               = std::make_shared<GameOption>("key.keyboard.d",      "Right control");
    knownOptions["key_key.jump"]                = std::make_shared<GameOption>("key.keyboard.space",  "Jump control");
    knownOptions["key_key.sneak"]               = std::make_shared<GameOption>("key.keyboard.left.shift", "Sneak control");
    knownOptions["key_key.sprint"]              = std::make_shared<GameOption>("key.keyboard.left.control", "Sprint control ");
    knownOptions["key_key.drop"]                = std::make_shared<GameOption>("key.keyboard.q",      "Drop control ");
    knownOptions["key_key.inventory"]           = std::make_shared<GameOption>("key.keyboard.e",      "Inventory control");
    knownOptions["key_key.chat"]                = std::make_shared<GameOption>("key.keyboard.t",      "Chat control");
    knownOptions["key_key.playerlist"]          = std::make_shared<GameOption>("key.keyboard.tab",    "List Players control");
    knownOptions["key_key.pickItem"]            = std::make_shared<GameOption>("key.mouse.middle",    "Pick Block control");
    knownOptions["key_key.command"]             = std::make_shared<GameOption>("key.keyboard.slash",  "Command control");
    knownOptions["key_key.socialInteractions"]  = std::make_shared<GameOption>("key.keyboard.p",      "Social Interaction control");
    knownOptions["key_key.screenshot"]          = std::make_shared<GameOption>("key.keyboard.f2",     "Screenshot control");
    knownOptions["key_key.togglePerspective"]   = std::make_shared<GameOption>("key.keyboard.f5",     "Perspective control");
    knownOptions["key_key.smoothCamera"]        = std::make_shared<GameOption>("key.keyboard.unknown", "Mouse Smoothing control");
    knownOptions["key_key.fullscreen"]          = std::make_shared<GameOption>("key.keyboard.f11",    "Fullscreen control");
    knownOptions["key_key.spectatorOutlines"]   = std::make_shared<GameOption>("key.keyboard.unknown", "Visibility of player outlines in Spectator Mode control");
    knownOptions["key_key.swapOffhand"]         = std::make_shared<GameOption>("key.keyboard.f",      "Swapping of items between both hands control");
    knownOptions["key_key.saveToolbarActivator"] = std::make_shared<GameOption>("key.keyboard.c",     "Save current toolbar to a slot (in Creative Mode)");
    knownOptions["key_key.loadToolbarActivator"] = std::make_shared<GameOption>("key.keyboard.x",     "Load toolbar from a slot (in Creative Mode)");
    knownOptions["key_key.advancements"]        = std::make_shared<GameOption>("key.keyboard.l",      "Open the Advancements screen");
    knownOptions["key_key.hotbar.1"]            = std::make_shared<GameOption>("key.keyboard.1",      "Hotbar Slot 1 control");
    knownOptions["key_key.hotbar.2"]            = std::make_shared<GameOption>("key.keyboard.2",      "Hotbar Slot 2 control");
    knownOptions["key_key.hotbar.3"]            = std::make_shared<GameOption>("key.keyboard.3",      "Hotbar Slot 3 control");
    knownOptions["key_key.hotbar.4"]            = std::make_shared<GameOption>("key.keyboard.4",      "Hotbar Slot 4 control");
    knownOptions["key_key.hotbar.5"]            = std::make_shared<GameOption>("key.keyboard.5",      "Hotbar Slot 5 control");
    knownOptions["key_key.hotbar.6"]            = std::make_shared<GameOption>("key.keyboard.6",      "Hotbar Slot 6 control");
    knownOptions["key_key.hotbar.7"]            = std::make_shared<GameOption>("key.keyboard.7",      "Hotbar Slot 7 control");
    knownOptions["key_key.hotbar.8"]            = std::make_shared<GameOption>("key.keyboard.8",      "Hotbar Slot 8 control");
    knownOptions["key_key.hotbar.9"]            = std::make_shared<GameOption>("key.keyboard.9",      "Hotbar Slot 9 control");

    // Sound
    knownOptions["soundCategory_master"]    = std::make_shared<GameOption>(1.0f,  "The volume of all sounds", Range<float>{ 0, 1 });
    knownOptions["soundCategory_music"]     = std::make_shared<GameOption>(1.0f,  "The volume of gameplay music", Range<float>{ 0, 1 });
    knownOptions["soundCategory_record"]    = std::make_shared<GameOption>(1.0f,  "The volume of music/sounds from Jukeboxes and Note Blocks", Range<float>{ 0, 1 });
    knownOptions["soundCategory_weather"]   = std::make_shared<GameOption>(1.0f,  "The volume of rain and thunder", Range<float>{ 0, 1 });
    knownOptions["soundCategory_block"]     = std::make_shared<GameOption>(1.0f,  "The volume of blocks", Range<float>{ 0, 1 });
    knownOptions["soundCategory_hostile"]   = std::make_shared<GameOption>(1.0f,  "The volume of hostile and neutral mobs", Range<float>{ 0, 1 });
    knownOptions["soundCategory_neutral"]   = std::make_shared<GameOption>(1.0f,  "The volume of passive mobs", Range<float>{ 0, 1 });
    knownOptions["soundCategory_player"]    = std::make_shared<GameOption>(1.0f,  "The volume of players", Range<float>{ 0, 1 });
    knownOptions["soundCategory_ambient"]   = std::make_shared<GameOption>(1.0f,  "The volume of cave sounds and fireworks", Range<float>{ 0, 1 });
    knownOptions["soundCategory_voice"]     = std::make_shared<GameOption>(1.0f,  "The volume of voices", Range<float>{ 0, 1 });

    // Model Parts
    knownOptions["modelPart_cape"]              = std::make_shared<GameOption>(true,  "Whether the cape is shown");
    knownOptions["modelPart_jacket"]            = std::make_shared<GameOption>(true,  "Whether the \"Jacket\" skin layer is shown");
    knownOptions["modelPart_left_sleeve"]       = std::make_shared<GameOption>(true,  "Whether the \"Left Sleeve\" skin layer is shown");
    knownOptions["modelPart_right_sleeve"]      = std::make_shared<GameOption>(true,  "Whether the \"Right Sleeve\" skin layer is shown");
    knownOptions["modelPart_left_pants_leg"]    = std::make_shared<GameOption>(true,  "Whether the \"Left Pants Leg\" skin layer is shown");
    knownOptions["modelPart_right_pants_leg"]   = std::make_shared<GameOption>(true,  "Whether the \"Right Pants Leg\" skin layer is shown");
    knownOptions["modelPart_hat"]               = std::make_shared<GameOption>(true,  "Whether the \"Hat\" skin layer is shown");

    // Misc
    knownOptions["fullscreenResolution"]        = std::make_shared<GameOption>("", "Changes the resolution of the game when in fullscreen mode. The only values allowed are the values supported by the user's monitor, shown when changing the screen resolution in the operating system settings. Setting this option to a value not supported by the monitor resets the option to \"Current\". When set to \"Current\", this option is absent from options.txt. ");

    // Not vanilla - from mods or modloaders
    knownOptions["quilt_available_resourcepacks"] = std::make_shared<GameOption>("[]", "Quilt Internal Available Resource Packs", QStringList(), false);

}

/// @brief this data is mostly copied from https://minecraft.fandom.com/wiki/Key_codes
void GameOptionsSchema::populateInternalKeyBindList() {
    // TODO: add numeric (GLFW) key codes

    // Legacy Keycodes are from minecraft versions prior to 1.13 and use a numeric code loosely based on old LWJGL Key Codes
    //             #-----------------------#-------------------#----------------------------------#-------------------------#
    //             |minecraft key code     | old LWJGL Code    |Display String                    | Qt Key Code             |
    //             #-----------------------#-------------------#----------------------------------#-------------------------#
    
    // Unboud
    addKeyboardBind("key.keyboard.unknown",             0, QObject::tr("Not bound"), Qt::Key_unknown );  // 4th value is Qt key code

    // Mouse - legacy keybindings only go up to 16 for mice, represented as: -100 + LWJGL Code
    addMouseBind(   "key.mouse.left",                   -100,   QObject::tr("Left Mouse Button"),   Qt::MouseButton::LeftButton );
    addMouseBind(   "key.mouse.right",                  -99,    QObject::tr("Right Mouse Button"),  Qt::MouseButton::RightButton );
    addMouseBind(   "key.mouse.middle",                 -98,    QObject::tr("Middle Mouse Button"), Qt::MouseButton::MiddleButton );
    addMouseBind(   "key.mouse.4",                      -97,    QObject::tr("Mouse Button 4"),      Qt::MouseButton::BackButton );
    addMouseBind(   "key.mouse.5",                      -96,    QObject::tr("Mouse Button 5"),      Qt::MouseButton::ExtraButton1 );
    addMouseBind(   "key.mouse.7",                      -95,    QObject::tr("Mouse Button 7"),      Qt::MouseButton::ExtraButton3 );
    addMouseBind(   "key.mouse.8",                      -94,    QObject::tr("Mouse Button 8"),      Qt::MouseButton::ExtraButton4 );
    addMouseBind(   "key.mouse.9",                      -93,    QObject::tr("Mouse Button 9"),      Qt::MouseButton::ExtraButton5 );
    addMouseBind(   "key.mouse.6",                      -92,    QObject::tr("Mouse Button 6"),      Qt::MouseButton::ExtraButton2 );
    addMouseBind(   "key.mouse.10",                     -91,    QObject::tr("Mouse Button 10"),     Qt::MouseButton::ExtraButton6 );
    addMouseBind(   "key.mouse.11",                     -90,    QObject::tr("Mouse Button 11"),     Qt::MouseButton::ExtraButton7 );
    addMouseBind(   "key.mouse.12",                     -89,    QObject::tr("Mouse Button 12"),     Qt::MouseButton::ExtraButton8 );
    addMouseBind(   "key.mouse.13",                     -88,    QObject::tr("Mouse Button 13"),     Qt::MouseButton::ExtraButton9 );
    addMouseBind(   "key.mouse.14",                     -87,    QObject::tr("Mouse Button 14"),     Qt::MouseButton::ExtraButton10 );
    addMouseBind(   "key.mouse.15",                     -86,    QObject::tr("Mouse Button 15"),     Qt::MouseButton::ExtraButton11 );
    addMouseBind(   "key.mouse.16",                     -85,    QObject::tr("Mouse Button 16"),     Qt::MouseButton::ExtraButton12 );
    addMouseBind(   "key.mouse.17",                     0,      QObject::tr("Mouse Button 17"),     Qt::MouseButton::ExtraButton13 );
    addMouseBind(   "key.mouse.18",                     0,      QObject::tr("Mouse Button 18"),     Qt::MouseButton::ExtraButton14 );
    addMouseBind(   "key.mouse.19",                     0,      QObject::tr("Mouse Button 19"),     Qt::MouseButton::ExtraButton15 );
    addMouseBind(   "key.mouse.20",                     0,      QObject::tr("Mouse Button 20"),     Qt::MouseButton::ExtraButton16 );
    addMouseBind(   "key.mouse.21",                     0,      QObject::tr("Mouse Button 21"),     Qt::MouseButton::ExtraButton17 );
    addMouseBind(   "key.mouse.22",                     0,      QObject::tr("Mouse Button 22"),     Qt::MouseButton::ExtraButton18 );
    addMouseBind(   "key.mouse.23",                     0,      QObject::tr("Mouse Button 23"),     Qt::MouseButton::ExtraButton19 );
    addMouseBind(   "key.mouse.24",                     0,      QObject::tr("Mouse Button 24"),     Qt::MouseButton::ExtraButton20 );
    addMouseBind(   "key.mouse.25",                     0,      QObject::tr("Mouse Button 25"),     Qt::MouseButton::ExtraButton21 );
    addMouseBind(   "key.mouse.26",                     0,      QObject::tr("Mouse Button 26"),     Qt::MouseButton::ExtraButton22 );
    addMouseBind(   "key.mouse.27",                     0,      QObject::tr("Mouse Button 27"),     Qt::MouseButton::ExtraButton23 );
    addMouseBind(   "key.mouse.28",                     0,      QObject::tr("Mouse Button 28"),     Qt::MouseButton::ExtraButton24 );
    // not sure if it makes sense to go this far, but it's how far Qt can count.
    
    // Number Row
    addKeyboardBind("key.keyboard.0",                   11,     QObject::tr("0"),                   Qt::Key::Key_0 );
    addKeyboardBind("key.keyboard.1",                   2,      QObject::tr("1"),                   Qt::Key::Key_1 );
    addKeyboardBind("key.keyboard.2",                   3,      QObject::tr("2"),                   Qt::Key::Key_2 );
    addKeyboardBind("key.keyboard.3",                   4,      QObject::tr("3"),                   Qt::Key::Key_3 );
    addKeyboardBind("key.keyboard.4",                   5,      QObject::tr("4"),                   Qt::Key::Key_4 );
    addKeyboardBind("key.keyboard.5",                   6,      QObject::tr("5"),                   Qt::Key::Key_5 );
    addKeyboardBind("key.keyboard.6",                   7,      QObject::tr("6"),                   Qt::Key::Key_6 );
    addKeyboardBind("key.keyboard.7",                   8,      QObject::tr("7"),                   Qt::Key::Key_7 );
    addKeyboardBind("key.keyboard.8",                   9,      QObject::tr("8"),                   Qt::Key::Key_8 );
    addKeyboardBind("key.keyboard.9",                   10,     QObject::tr("9"),                   Qt::Key::Key_9 );

    // Letters
    addKeyboardBind("key.keyboard.a",                   30,     QObject::tr("a"),                   Qt::Key::Key_A );
    addKeyboardBind("key.keyboard.b",                   48,     QObject::tr("b"),                   Qt::Key::Key_B );
    addKeyboardBind("key.keyboard.c",                   46,     QObject::tr("c"),                   Qt::Key::Key_C );
    addKeyboardBind("key.keyboard.d",                   32,     QObject::tr("d"),                   Qt::Key::Key_D );
    addKeyboardBind("key.keyboard.e",                   18,     QObject::tr("e"),                   Qt::Key::Key_E );
    addKeyboardBind("key.keyboard.f",                   33,     QObject::tr("f"),                   Qt::Key::Key_F );
    addKeyboardBind("key.keyboard.g",                   34,     QObject::tr("g"),                   Qt::Key::Key_G );
    addKeyboardBind("key.keyboard.h",                   35,     QObject::tr("h"),                   Qt::Key::Key_H );
    addKeyboardBind("key.keyboard.i",                   23,     QObject::tr("i"),                   Qt::Key::Key_I );
    addKeyboardBind("key.keyboard.j",                   36,     QObject::tr("j"),                   Qt::Key::Key_J );
    addKeyboardBind("key.keyboard.k",                   37,     QObject::tr("k"),                   Qt::Key::Key_K );
    addKeyboardBind("key.keyboard.l",                   38,     QObject::tr("l"),                   Qt::Key::Key_L );
    addKeyboardBind("key.keyboard.m",                   50,     QObject::tr("m"),                   Qt::Key::Key_M );
    addKeyboardBind("key.keyboard.n",                   49,     QObject::tr("n"),                   Qt::Key::Key_N );
    addKeyboardBind("key.keyboard.o",                   24,     QObject::tr("o"),                   Qt::Key::Key_O );
    addKeyboardBind("key.keyboard.p",                   25,     QObject::tr("p"),                   Qt::Key::Key_P );
    addKeyboardBind("key.keyboard.q",                   16,     QObject::tr("q"),                   Qt::Key::Key_Q );
    addKeyboardBind("key.keyboard.r",                   19,     QObject::tr("r"),                   Qt::Key::Key_R );
    addKeyboardBind("key.keyboard.s",                   31,     QObject::tr("s"),                   Qt::Key::Key_S );
    addKeyboardBind("key.keyboard.t",                   20,     QObject::tr("t"),                   Qt::Key::Key_T );
    addKeyboardBind("key.keyboard.u",                   22,     QObject::tr("u"),                   Qt::Key::Key_U );
    addKeyboardBind("key.keyboard.v",                   47,     QObject::tr("v"),                   Qt::Key::Key_V );
    addKeyboardBind("key.keyboard.w",                   17,     QObject::tr("w"),                   Qt::Key::Key_W );
    addKeyboardBind("key.keyboard.x",                   45,     QObject::tr("x"),                   Qt::Key::Key_X );
    addKeyboardBind("key.keyboard.y",                   21,     QObject::tr("y"),                   Qt::Key::Key_Y );
    addKeyboardBind("key.keyboard.z",                   44,     QObject::tr("z"),                   Qt::Key::Key_Z );

    // F Keys - legacy keybinds have only 15 F keys
    addKeyboardBind("key.keyboard.f1",                  59,     QObject::tr("F1"),                  Qt::Key::Key_F1 );
    addKeyboardBind("key.keyboard.f2",                  60,     QObject::tr("F2"),                  Qt::Key::Key_F2 );
    addKeyboardBind("key.keyboard.f3",                  61,     QObject::tr("F3"),                  Qt::Key::Key_F3 );
    addKeyboardBind("key.keyboard.f4",                  62,     QObject::tr("F4"),                  Qt::Key::Key_F4 );
    addKeyboardBind("key.keyboard.f5",                  63,     QObject::tr("F5"),                  Qt::Key::Key_F5 );
    addKeyboardBind("key.keyboard.f6",                  64,     QObject::tr("F6"),                  Qt::Key::Key_F6 );
    addKeyboardBind("key.keyboard.f7",                  65,     QObject::tr("F7"),                  Qt::Key::Key_F7 );
    addKeyboardBind("key.keyboard.f8",                  66,     QObject::tr("F8"),                  Qt::Key::Key_F8 );
    addKeyboardBind("key.keyboard.f9",                  67,     QObject::tr("F9"),                  Qt::Key::Key_F9 );
    addKeyboardBind("key.keyboard.f10",                 68,     QObject::tr("F10"),                 Qt::Key::Key_F10 );
    addKeyboardBind("key.keyboard.f11",                 87,     QObject::tr("F11"),                 Qt::Key::Key_F11 );
    addKeyboardBind("key.keyboard.f12",                 88,     QObject::tr("F12"),                 Qt::Key::Key_F12 );
    addKeyboardBind("key.keyboard.f13",                 100,    QObject::tr("F13"),                 Qt::Key::Key_F13 );
    addKeyboardBind("key.keyboard.f14",                 101,    QObject::tr("F14"),                 Qt::Key::Key_F14 );
    addKeyboardBind("key.keyboard.f15",                 102,    QObject::tr("F15"),                 Qt::Key::Key_F15 );
    addKeyboardBind("key.keyboard.f16",                 0,      QObject::tr("F16"),                 Qt::Key::Key_F16 );
    addKeyboardBind("key.keyboard.f17",                 0,      QObject::tr("F17"),                 Qt::Key::Key_F17 );
    addKeyboardBind("key.keyboard.f18",                 0,      QObject::tr("F18"),                 Qt::Key::Key_F18 );
    addKeyboardBind("key.keyboard.f19",                 0,      QObject::tr("F19"),                 Qt::Key::Key_F19 );
    addKeyboardBind("key.keyboard.f20",                 0,      QObject::tr("F20"),                 Qt::Key::Key_F20 );
    addKeyboardBind("key.keyboard.f21",                 0,      QObject::tr("F21"),                 Qt::Key::Key_F21 );
    addKeyboardBind("key.keyboard.f22",                 0,      QObject::tr("F22"),                 Qt::Key::Key_F22 );
    addKeyboardBind("key.keyboard.f23",                 0,      QObject::tr("F23"),                 Qt::Key::Key_F23 );
    addKeyboardBind("key.keyboard.f24",                 0,      QObject::tr("F24"),                 Qt::Key::Key_F24 );
    addKeyboardBind("key.keyboard.f25",                 0,      QObject::tr("F25"),                 Qt::Key::Key_F25 );

    // Numblock
    addKeyboardBind("key.keyboard.num.lock",            69,     QObject::tr("Num Lock"),            Qt::Key::Key_NumLock );
    addKeyboardBind("key.keyboard.keypad.0",            82,     QObject::tr("Keypad 0"),            Qt::Key::Key_0 );
    addKeyboardBind("key.keyboard.keypad.1",            79,     QObject::tr("Keypad 1"),            Qt::Key::Key_0 );
    addKeyboardBind("key.keyboard.keypad.2",            80,     QObject::tr("Keypad 2"),            Qt::Key::Key_0 );
    addKeyboardBind("key.keyboard.keypad.3",            81,     QObject::tr("Keypad 3"),            Qt::Key::Key_0 );
    addKeyboardBind("key.keyboard.keypad.4",            75,     QObject::tr("Keypad 4"),            Qt::Key::Key_0 );
    addKeyboardBind("key.keyboard.keypad.5",            76,     QObject::tr("Keypad 5"),            Qt::Key::Key_0 );
    addKeyboardBind("key.keyboard.keypad.6",            77,     QObject::tr("Keypad 6"),            Qt::Key::Key_0 );
    addKeyboardBind("key.keyboard.keypad.7",            71,     QObject::tr("Keypad 7"),            Qt::Key::Key_0 );
    addKeyboardBind("key.keyboard.keypad.8",            72,     QObject::tr("Keypad 8"),            Qt::Key::Key_0 );
    addKeyboardBind("key.keyboard.keypad.9",            73,     QObject::tr("Keypad 9"),            Qt::Key::Key_0 );
    addKeyboardBind("key.keyboard.keypad.add",          78,     QObject::tr("Keypad +"),            Qt::Key_unknown );      // fix maybe?
    addKeyboardBind("key.keyboard.keypad.decimal",      83,     QObject::tr("Keypad Decimal"),      Qt::Key_unknown );      // fix maybe?
    addKeyboardBind("key.keyboard.keypad.enter",        156,    QObject::tr("Keypad Enter"),        Qt::Key_unknown );      // fix maybe?
    addKeyboardBind("key.keyboard.keypad.equal",        141,    QObject::tr("Keypad ="),            Qt::Key_unknown );      // fix maybe?
    addKeyboardBind("key.keyboard.keypad.multiply",     55,     QObject::tr("Keypad *"),            Qt::Key_unknown );      // fix maybe?
    addKeyboardBind("key.keyboard.keypad.divide",       181,    QObject::tr("Keypad /"),            Qt::Key_unknown );      // fix maybe?
    addKeyboardBind("key.keyboard.keypad.subtract",     74,     QObject::tr("Keypad -"),            Qt::Key_unknown );      // fix maybe?

    // Arrow Keys
    addKeyboardBind("key.keyboard.down",                208,    QObject::tr("Down Arrow"),          Qt::Key::Key_Down );
    addKeyboardBind("key.keyboard.left",                203,    QObject::tr("Left Arrow"),          Qt::Key::Key_Left );
    addKeyboardBind("key.keyboard.right",               205,    QObject::tr("Right Arrow"),         Qt::Key::Key_Right );
    addKeyboardBind("key.keyboard.up",                  200,    QObject::tr("Up Arrow"),            Qt::Key::Key_Up );

    // Other
    addKeyboardBind("key.keyboard.apostrophe",          40,     QObject::tr("'"),                   Qt::Key::Key_Apostrophe );
    addKeyboardBind("key.keyboard.backslash",           43,     QObject::tr("\\"),                  Qt::Key::Key_Backslash );
    addKeyboardBind("key.keyboard.comma",               51,     QObject::tr(","),                   Qt::Key::Key_Comma );
    addKeyboardBind("key.keyboard.equal",               13,     QObject::tr("="),                   Qt::Key::Key_Equal );
    addKeyboardBind("key.keyboard.grave.accent",        41,     QObject::tr("`"),                   Qt::Key::Key_Dead_Grave );
    addKeyboardBind("key.keyboard.left.bracket",        26,     QObject::tr("["),                   Qt::Key::Key_BracketLeft );
    addKeyboardBind("key.keyboard.minus",               12,     QObject::tr("-"),                   Qt::Key::Key_Minus );
    addKeyboardBind("key.keyboard.period",              52,     QObject::tr("."),                   Qt::Key::Key_Period );
    addKeyboardBind("key.keyboard.right.bracket",       27,     QObject::tr("]"),                   Qt::Key::Key_BracketRight );
    addKeyboardBind("key.keyboard.semicolon",           39,     QObject::tr(";"),                   Qt::Key::Key_Semicolon );
    addKeyboardBind("key.keyboard.slash",               53,     QObject::tr("/"),                   Qt::Key::Key_Slash );

    addKeyboardBind("key.keyboard.space",               57,     QObject::tr("Space"),               Qt::Key::Key_Space );
    addKeyboardBind("key.keyboard.tab",                 15,     QObject::tr("Tab"),                 Qt::Key::Key_Tab );
    addKeyboardBind("key.keyboard.left.alt",            56,     QObject::tr("Left Alt"),            Qt::Key::Key_Alt );
    addKeyboardBind("key.keyboard.left.control",        29,     QObject::tr("Left Control"),        Qt::Key::Key_Control );
    addKeyboardBind("key.keyboard.left.shift",          42,     QObject::tr("Left Shift"),          Qt::Key::Key_Shift );
    addKeyboardBind("key.keyboard.left.win",            219,    QObject::tr("Left Win"),            Qt::Key::Key_Meta );
    addKeyboardBind("key.keyboard.right.alt",           184,    QObject::tr("Right Alt"),           Qt::Key::Key_AltGr );
    addKeyboardBind("key.keyboard.right.control",       157,    QObject::tr("Right Control"),       Qt::Key_unknown );      // fix maybe?
    addKeyboardBind("key.keyboard.right.shift",         54,     QObject::tr("Right Shift"),         Qt::Key_unknown );      // fix maybe?
    addKeyboardBind("key.keyboard.right.win",           220,    QObject::tr("Right Win"),           Qt::Key_unknown );      // fix maybe?
    addKeyboardBind("key.keyboard.enter",               28,     QObject::tr("Enter"),               Qt::Key::Key_Enter );
    addKeyboardBind("key.keyboard.escape",              1,      QObject::tr("Escape"),              Qt::Key::Key_Escape );
    addKeyboardBind("key.keyboard.backspace",           14,     QObject::tr("Backspace"),           Qt::Key::Key_Backspace );
    addKeyboardBind("key.keyboard.delete",              211,    QObject::tr("Delete"),              Qt::Key::Key_Delete );
    addKeyboardBind("key.keyboard.end",                 207,    QObject::tr("End"),                 Qt::Key::Key_End );
    addKeyboardBind("key.keyboard.home",                199,    QObject::tr("Home"),                Qt::Key::Key_Home );
    addKeyboardBind("key.keyboard.insert",              210,    QObject::tr("Insert"),              Qt::Key::Key_Insert );
    addKeyboardBind("key.keyboard.page.down",           209,    QObject::tr("Page Down"),           Qt::Key::Key_PageDown );
    addKeyboardBind("key.keyboard.page.up",             201,    QObject::tr("Page Up"),             Qt::Key::Key_PageUp );
    addKeyboardBind("key.keyboard.caps.lock",           58,     QObject::tr("Caps Lock"),           Qt::Key::Key_CapsLock );
    addKeyboardBind("key.keyboard.pause",               197,    QObject::tr("Pause"),               Qt::Key::Key_Pause );
    addKeyboardBind("key.keyboard.scroll.lock",         70,     QObject::tr("Scroll Lock"),         Qt::Key::Key_ScrollLock );
    addKeyboardBind("key.keyboard.menu",                0,      QObject::tr("Menu"),                Qt::Key::Key_Menu );    // not legacy bind?
    addKeyboardBind("key.keyboard.print.screen",        0,      QObject::tr("Print Screen"),        Qt::Key::Key_Print );   // not legacy bind?
    addKeyboardBind("key.keyboard.world.1",             0,      QObject::tr("World 1"),             Qt::Key_unknown );      // fix maybe? not legacy bind?
    addKeyboardBind("key.keyboard.world.2",             0,      QObject::tr("World 2"),             Qt::Key_unknown );      // fix maybe? not legacy bind?
    // addKeyboardBind( "scancode.###",                    0,      QObject::tr("scancode.###")              );              // no description, no translation, no mapping?

    // clang-format on
};
QString GameOption::getDefaultString()
{ return defaultString; }
