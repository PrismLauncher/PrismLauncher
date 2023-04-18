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
#pragma once

#include <QString>
#include <QList>
#include <QMap>
#include <memory>
#include <utility>

enum class OptionType { String, Int, Float, Bool, KeyBind };

template <class T> struct Range {
    T min, max;
};

union UniversalRange {
    Range<float> floatRange;
    Range<int> intRange;
};

union OptionValue {
    float floatValue;
    int intValue;
    bool boolValue;
    // QString stringValue;
};

class GameOption {
   public:

    /// @brief Bool variant
    /// @param defaultValue Default Value
    /// @param description The Description for this Option
    /// @param readOnly whether or not this option should be editable
    explicit GameOption(bool defaultValue, QString description = "", bool readOnly = false)
        : description(std::move(description)), type(OptionType::Bool), readOnly(readOnly) {
        GameOption::defaultValue.boolValue = defaultValue;
    };

    /// @brief String variant
    /// @param defaultValue Default Value
    /// @param description The Description for this Option
    /// @param validValues List of possible options for this field, if empty any input will be possible
    /// @param readOnly whether or not this option should be editable
    explicit GameOption(QString defaultValue = "", QString description = "", QList<QString> validValues = QList<QString>(), bool readOnly = false)
        : description(std::move(description)), type(OptionType::String), readOnly(readOnly), defaultString(std::move(defaultValue)), validValues(std::move(validValues)){};

    /// @brief KeyBind variant
    /// @param defaultValue Default Value
    /// @param description The Description for this Option
    explicit GameOption(QString defaultValue = "", QString description = "")
        : description(std::move(description)), type(OptionType::KeyBind), defaultString(std::move(defaultValue)){};

    /// @brief Float variant
    /// @param defaultValue Default Value
    /// @param description The Description for this Option
    /// @param range if left empty (0,0) no limits are assumed
    /// @param readOnly whether or not this option should be editable
    explicit GameOption(float defaultValue = 0.0f, QString description = "", Range<float> range = Range<float>{ 0.0f, 0.0f }, bool readOnly = false)
        : description(std::move(description)), type(OptionType::Float), readOnly(readOnly), range{ range }, defaultValue{ defaultValue } {};

    /// @brief Int variant
    /// @param defaultValue Default Value 
    /// @param description Description for this Option
    /// @param range if left empty (0,0) no limits are assumed
    /// @param readOnly whether or not this option should be editable
    explicit GameOption(int defaultValue = 0, QString description = "", Range<int> range = Range<int>{ 0, 0 }, bool readOnly = false)
        : description(std::move(description)), type(OptionType::Int), readOnly(readOnly)
    {
        GameOption::range.intRange = range;
        GameOption::defaultValue.intValue = defaultValue;
    };

    QString description;
    OptionType type;
    bool readOnly = false;
    QList<QString> validValues;  // if empty, treat as text input
    //int introducedVersion;
    //int removedVersion;

    int getDefaultInt() const { return defaultValue.intValue; };
    bool getDefaultBool() const { return defaultValue.boolValue; };
    float getDefaultFloat() const { return defaultValue.floatValue; };
    QString getDefaultString();;

    Range<int> getIntRange() const { return range.intRange; };
    Range<float> getFloatRange() const { return range.floatRange; };

   private:
    OptionValue defaultValue = { 0.0f };
    QString defaultString;
    UniversalRange range = { 0.0f, 0.0f };

};

union mouseOrKeyboardButton {
    Qt::Key keyboardKey;
    Qt::MouseButton mouseButton;
};

class KeyBindData {
   public:
    KeyBindData(QString minecraftKeyCode, int glfwCode, QString displayName, Qt::MouseButton mouseButton) : 
        minecraftKeyCode(std::move(minecraftKeyCode)), glfwCode(glfwCode), displayName(std::move(displayName))
    {
        qtKeyCode.mouseButton = mouseButton;
    }

    KeyBindData(QString minecraftKeyCode, int glfwCode, QString displayName, Qt::Key keyboardKey)
        : minecraftKeyCode(std::move(minecraftKeyCode)), glfwCode(glfwCode), displayName(std::move(displayName))
    {
        qtKeyCode.keyboardKey = keyboardKey;
    }


    QString minecraftKeyCode;
    int glfwCode;
    QString displayName;
    mouseOrKeyboardButton qtKeyCode;
};

/// @brief defines constraints, default values and descriptions for known game settings 
class GameOptionsSchema {
   public:
    static QMap<QString, std::shared_ptr<GameOption>>* getKnownOptions();
    static QList<std::shared_ptr<KeyBindData>>* getKeyBindOptions();

   private:
    static QMap<QString, std::shared_ptr<GameOption>> knownOptions;
    static QList<std::shared_ptr<KeyBindData>> keyboardButtons;

    static void populateInternalOptionList();
    static void populateInternalKeyBindList();

    static void addKeyboardBind(const QString& minecraftKeyCode, int glfwCode, const QString& displayName, Qt::Key keyboardKey)
    {
        keyboardButtons.append(std::make_shared<KeyBindData>(minecraftKeyCode, glfwCode, displayName, keyboardKey));
    };
    static void addMouseBind(const QString& minecraftKeyCode, int glfwCode, const QString& displayName, Qt::MouseButton mouseButton)
    {
        keyboardButtons.append(std::make_shared<KeyBindData>(minecraftKeyCode, glfwCode, displayName, mouseButton));
    };

};
