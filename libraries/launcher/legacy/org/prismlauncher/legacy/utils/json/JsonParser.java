// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 TheKodeToad <TheKodeToad@proton.me>
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
 *  Linking this library statically or dynamically with other modules is
 *  making a combined work based on this library. Thus, the terms and
 *  conditions of the GNU General Public License cover the whole
 *  combination.
 *
 *  As a special exception, the copyright holders of this library give
 *  you permission to link this library with independent modules to
 *  produce an executable, regardless of the license terms of these
 *  independent modules, and to copy and distribute the resulting
 *  executable under terms of your choice, provided that you also meet,
 *  for each linked independent module, the terms and conditions of the
 *  license of that module. An independent module is a module which is
 *  not derived from or based on this library. If you modify this
 *  library, you may extend this exception to your version of the
 *  library, but you are not obliged to do so. If you do not wish to do
 *  so, delete this exception statement from your version.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

package org.prismlauncher.legacy.utils.json;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.StringReader;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A lightweight portable JSON parser used instead of GSON since it is not
 * available in a lot of versions.
 */
public final class JsonParser {
    private final Reader in;
    private char[] buffer;
    private int pos, length;

    public static Object parse(String in) throws IOException {
        return parse(new StringReader(in));
    }

    public static Object parse(InputStream in) throws IOException {
        return parse(new InputStreamReader(in, StandardCharsets.UTF_8));
    }

    public static Object parse(Reader in) throws IOException {
        return new JsonParser(in).readSingleValue();
    }

    private JsonParser(Reader in) throws IOException {
        this.in = in;
        pos = length = 0;
        read();
    }

    private int character() {
        if (length == -1)
            return -1;

        return buffer[pos];
    }

    private int read() throws IOException {
        if (length == -1)
            return -1;

        if (buffer == null || pos++ == length - 1) {
            pos = 0;
            buffer = new char[8192];
            length = in.read(buffer);
        }

        return character();
    }

    private void assertCharacter(char character) throws JsonParseException {
        if (character() != character)
            throw new JsonParseException(
                    "Expected '" + character + "' but got " + (character() != -1 ? ("'" + (char) character() + "'") : "EOF"));
    }

    private void assertNoEOF(String expected) throws JsonParseException {
        if (character() == -1)
            throw new JsonParseException("Expected " + expected + " but got EOF");
    }

    private void skipWhitespace() throws IOException {
        while (isWhitespace()) read();
    }

    private boolean isWhitespace() {
        return character() == ' ' || character() == '\n' || character() == '\r' || character() == '\t';
    }

    private Object readSingleValue() throws IOException {
        skipWhitespace();
        Object result = readValue();

        if (!(result instanceof Double))
            read();

        skipWhitespace();

        if (character() != -1)
            throw new JsonParseException("Found trailing non-whitespace characters");

        return result;
    }

    private Object readValue() throws IOException {
        assertNoEOF("a value");

        int character = character();

        switch (character) {
            case '{':
                return readObject();

            case '[':
                return readArray();

            case '"':
                return readString();

            case 't':
            case 'f':
                // probably boolean
                Boolean bool = readBoolean();
                if (bool != null)
                    return bool;

                break;

            case 'n':
                // probably null
                if (readNull())
                    return null;

                break;
        }

        if (character == '-' || isDigit())
            // probably a number
            return readNumber();

        throw new JsonParseException("Expected a JSON value but got '" + (char) character + "'");
    }

    private Map<String, Object> readObject() throws IOException {
        assertCharacter('{');
        Map<String, Object> obj = new HashMap<>();
        boolean comma = false;

        read();
        skipWhitespace();

        while (character() != '}') {
            if (comma) {
                assertCharacter(',');
                read();
                skipWhitespace();
            }

            String key = readString();
            read();
            skipWhitespace();
            assertCharacter(':');
            read();
            skipWhitespace();

            Object value = readValue();
            obj.put(key, value);

            if (!(value instanceof Double))
                read();

            skipWhitespace();
            comma = true;
        }

        return obj;
    }

    private List<Object> readArray() throws IOException {
        assertCharacter('[');
        List<Object> array = new ArrayList<>();
        boolean comma = false;

        read();
        skipWhitespace();

        while (character() != ']') {
            if (comma) {
                assertCharacter(',');
                read();
                skipWhitespace();
            }

            Object value = readValue();
            array.add(value);

            if (!(value instanceof Double))
                read();

            skipWhitespace();
            comma = true;
        }

        return array;
    }

    private String readString() throws IOException {
        assertCharacter('"');

        StringBuilder result = new StringBuilder();

        while (read() != '"') {
            int character = character();

            if (character >= '\u0000' && character <= '\u001F')
                throw new JsonParseException("Found unescaped control character within string");

            switch (character) {
                case -1:
                    throw new JsonParseException("Expected '\"' but got EOF");

                case 0x7F:
                    if (read() == '"') {
                        return result.toString();
                    }
                    continue;

                case '\\':
                    int seq = read();

                    switch (seq) {
                        case -1:
                            throw new JsonParseException("Expected an escape sequence but got EOF");

                        case '\\':
                            break;

                        case '/':
                        case '\"':
                            character = seq;
                            break;

                        case 'b':
                            character = '\b';
                            break;

                        case 'f':
                            character = '\f';
                            break;

                        case 'n':
                            character = '\n';
                            break;

                        case 'r':
                            character = '\r';
                            break;

                        case 't':
                            character = '\t';
                            break;

                        case 'u':
                            // char array to allow allocation in advance.
                            char[] digits = new char[4];

                            for (int index = 0; index < digits.length; index++) {
                                character = read();
                                if (index == 0 && character() == '-') {
                                    throw new JsonParseException("Hex sequence may not be negative");
                                } else if (character() == -1) {
                                    throw new JsonParseException("Expected a hex sequence but got EOF");
                                }
                                digits[index] = (char) character;
                            }

                            String digitsString = new String(digits);

                            try {
                                character = Integer.parseInt(digitsString, 16);
                            } catch (NumberFormatException e) {
                                throw new JsonParseException("Could not parse hex sequence \"" + digitsString + "\"");
                            }

                            break;
                        default:
                            throw new JsonParseException("Invalid escape sequence: \\" + (char) seq);
                    }
                    break;
            }

            result.append((char) character);
        }

        return result.toString();
    }

    private boolean isDigit() {
        return character() >= '0' && character() <= '9';
    }

    private Double readNumber() throws IOException {
        StringBuilder result = new StringBuilder();

        if (character() == '-') {
            result.append((char) character());
            read();
        }

        if (character() == '0') {
            result.append((char) character());
            read();

            if (isDigit())
                throw new JsonParseException("Found superfluous leading zero");
        } else if (!isDigit())
            throw new JsonParseException("Expected digits");

        while (character() != -1 && isDigit()) {
            result.append((char) character());
            read();
        }

        if (character() == '.') {
            result.append('.');

            read();
            assertNoEOF("digits");

            if (!isDigit())
                throw new JsonParseException("Expected digits after decimal point");

            while (character() != -1 && isDigit()) {
                result.append((char) character());
                read();
            }
        }

        if (character() == 'e' || character() == 'E') {
            result.append('E');

            read();
            assertNoEOF("digits");

            if (character() == '+' || character() == '-') {
                result.append((char) character());
                read();
            }

            if (!(character() == '+' || character() == '-' || isDigit()))
                throw new JsonParseException("Expected exponent digits");

            while (character() != -1 && isDigit()) {
                result.append((char) character());
                read();
            }
        }

        String resultStr = result.toString();

        try {
            return Double.parseDouble(resultStr);
        } catch (NumberFormatException e) {
            throw new JsonParseException("Failed to parse number '" + resultStr + "'");
        }
    }

    private Boolean readBoolean() throws IOException {
        if (character() == 't') {
            if (read() == 'r' && read() == 'u' && read() == 'e') {
                return true;
            }
        } else if (character() == 'f' && read() == 'a' && read() == 'l' && read() == 's' && read() == 'e') {
            return false;
        }

        return null;
    }

    private boolean readNull() throws IOException {
        return character() == 'n' && read() == 'u' && read() == 'l' && read() == 'l';
    }
}
