package org.prismlauncher.fix.online;

import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.Map;

import org.prismlauncher.utils.Base64;
import org.prismlauncher.utils.JsonParser;

@SuppressWarnings("unchecked")
final class SkinFix {

    static URL redirect(URL address) throws IOException {
        String skinOwner = findSkinOwner(address);
        if (skinOwner != null)
            return convert(skinOwner, "SKIN");

        String capeOwner = findCapeOwner(address);
        if (capeOwner != null)
            return convert(capeOwner, "CAPE");

        return address;
    }

    private static URL convert(String owner, String name) throws IOException {
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
