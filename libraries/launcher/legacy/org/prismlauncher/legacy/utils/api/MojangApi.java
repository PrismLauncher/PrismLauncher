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

package org.prismlauncher.legacy.utils.api;

import org.prismlauncher.legacy.utils.Base64;
import org.prismlauncher.legacy.utils.json.JsonParser;

import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.Map;

/**
 * Basic wrapper for Mojang's Minecraft API.
 */
@SuppressWarnings("unchecked")
public final class MojangApi {
    public static String getUuid(String username) throws IOException {
        try (InputStream in = new URL("https://api.mojang.com/users/profiles/minecraft/" + username).openStream()) {
            Map<String, Object> map = (Map<String, Object>) JsonParser.parse(in);
            return (String) map.get("id");
        }
    }

    public static Texture getTexture(String player, String id) throws IOException {
        Map<String, Object> map = getTextures(player);

        if (map != null) {
            map = (Map<String, Object>) map.get(id);
            if (map == null)
                return null;

            URL url = new URL((String) map.get("url"));
            boolean slim = false;

            if (id.equals("SKIN")) {
                map = (Map<String, Object>) map.get("metadata");
                if (map != null && "slim".equals(map.get("model")))
                    slim = true;
            }

            return new Texture(url, slim);
        }

        return null;
    }

    public static Map<String, Object> getTextures(String player) throws IOException {
        try (InputStream profileIn = new URL("https://sessionserver.mojang.com/session/minecraft/profile/" + player).openStream()) {
            Map<String, Object> profile = (Map<String, Object>) JsonParser.parse(profileIn);

            for (Map<String, Object> property : (Iterable<Map<String, Object>>) profile.get("properties")) {
                if (property.get("name").equals("textures")) {
                    Map<String, Object> result =
                            (Map<String, Object>) JsonParser.parse(new String(Base64.decode((String) property.get("value"))));
                    result = (Map<String, Object>) result.get("textures");

                    return result;
                }
            }

            return null;
        }
    }
}
