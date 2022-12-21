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

package org.prismlauncher.fix.skins;

import java.io.IOException;
import java.io.InputStream;
import java.net.Proxy;
import java.net.URL;
import java.net.URLConnection;
import java.net.URLStreamHandler;
import java.util.Map;

import org.prismlauncher.utils.Base64;
import org.prismlauncher.utils.JsonParser;
import org.prismlauncher.utils.UrlUtils;

@SuppressWarnings("unchecked")
final class Handler extends URLStreamHandler {

    private URL redirect(URL address) throws IOException {
        String skinOwner = findSkinOwner(address);
        if (skinOwner != null)
            return convert(skinOwner, "SKIN");

        String capeOwner = findCapeOwner(address);
        if (capeOwner != null)
            return convert(capeOwner, "CAPE");

        return address;
    }

    @Override
    protected URLConnection openConnection(URL address) throws IOException {
        return openConnection(address, null);
    }

    @Override
    protected URLConnection openConnection(URL address, Proxy proxy) throws IOException {
        address = redirect(address);
        return UrlUtils.openHttpConnection(address, proxy);
    }

    private URL convert(String owner, String name) throws IOException {
        Map<String, Object> textures = getTextures(owner);

        if (textures != null) {
            textures = (Map<String, Object>) textures.get(name);
            if (textures == null)
                return null;

            return new URL((String) textures.get("url"));
        }

        return null;
    }

    private static Map<String, Object> getTextures(String owner) throws IOException {
        try (InputStream in = new URL("https://api.mojang.com/users/profiles/minecraft/" + owner).openStream()) {
            Map<String, Object> map = (Map<String, Object>) JsonParser.parse(in);
            String id = (String) map.get("id");

            try (InputStream profileIn = new URL("https://sessionserver.mojang.com/session/minecraft/profile/" + id)
                    .openStream()) {
                Map<String, Object> profile = (Map<String, Object>) JsonParser.parse(profileIn);

                for (Map<String, Object> property : (Iterable<Map<String, Object>>) profile.get("properties")) {
                    if (property.get("name").equals("textures")) {
                        Map<String, Object> result = (Map<String, Object>) JsonParser
                                .parse(new String(Base64.decode((String) property.get("value"))));
                        result = (Map<String, Object>) result.get("textures");

                        return result;
                    }
                }

                return null;
            }
        }
    }

    private static String findSkinOwner(URL address) {
        switch (address.getHost()) {
            case "www.minecraft.net":
                return stripIfPrefixed(address.getPath(), "/skin/");

            case "s3.amazonaws.com":
            case "skins.minecraft.net":
                return stripIfPrefixed(address.getPath(), "/MinecraftSkins/");
        }

        return null;
    }

    private static String findCapeOwner(URL address) {
        switch (address.getHost()) {
            case "www.minecraft.net":
                return stripIfPrefixed(address.getQuery(), "user=");

            case "s3.amazonaws.com":
            case "skins.minecraft.net":
                return stripIfPrefixed(address.getPath(), "/MinecraftCloaks/");
        }

        return null;
    }

    private static String stripIfPrefixed(String string, String prefix) {
        if (string != null && string.startsWith(prefix)) {
            string = string.substring(prefix.length());

            if (string.endsWith(".png"))
                string = string.substring(0, string.lastIndexOf('.'));

            return string;
        }

        return null;
    }

}
