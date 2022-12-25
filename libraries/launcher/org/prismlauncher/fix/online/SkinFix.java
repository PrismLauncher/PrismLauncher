package org.prismlauncher.fix.online;

import java.awt.AlphaComposite;
import java.awt.Graphics2D;
import java.awt.image.BufferedImage;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.Proxy;
import java.net.URL;
import java.net.URLConnection;

import javax.imageio.ImageIO;

import org.prismlauncher.utils.api.MojangApi;
import org.prismlauncher.utils.api.Texture;
import org.prismlauncher.utils.url.CustomUrlConnection;
import org.prismlauncher.utils.url.UrlUtils;

final class SkinFix {

    static URLConnection openConnection(URL address, Proxy proxy) throws IOException {
        String skinOwner = findSkinOwner(address);
        if (skinOwner != null)
            // we need to correct the skin
            return getSkinConnection(skinOwner, proxy);

        String capeOwner = findCapeOwner(address);
        if (capeOwner != null) {
            // since we do not need to process the image, open a direct connection bypassing Handler
            Texture texture = MojangApi.getTexture(MojangApi.getUuid(capeOwner), "CAPE");
            if (texture == null)
                return null;

            return UrlUtils.openConnection(texture.getUrl(), proxy);
        }

        return null;
    }

    private static URLConnection getSkinConnection(String owner, Proxy proxy) throws IOException {
        Texture texture = MojangApi.getTexture(MojangApi.getUuid(owner), "SKIN");
        if (texture == null)
            return null;

        URLConnection connection = UrlUtils.openConnection(texture.getUrl(), proxy);
        try (InputStream in = connection.getInputStream()) {
            // thank you craftycodie!
            // this is heavily based on
            // https://github.com/Mojang/LegacyLauncher/pull/33/files#diff-b61023785a9260651ca0a223573ea9acb5be5eec478bff626dafb7abe13ffebaR99
            BufferedImage image = ImageIO.read(in);
            Graphics2D graphics = image.createGraphics();
            graphics.setComposite(AlphaComposite.getInstance(AlphaComposite.SRC_OVER));

            BufferedImage subimage;

            if (image.getHeight() > 32) {
                // flatten second layers
                subimage = image.getSubimage(0, 32, 56, 16);
                graphics.drawImage(subimage, 0, 16, null);
            }

            if (texture.isSlim()) {
                // convert slim to wide
                subimage = image.getSubimage(45, 16, 9, 16);
                graphics.drawImage(subimage, 46, 16, null);

                subimage = image.getSubimage(49, 16, 2, 4);
                graphics.drawImage(subimage, 50, 16, null);

                subimage = image.getSubimage(53, 20, 2, 12);
                graphics.drawImage(subimage, 54, 20, null);
            }

            graphics.dispose();

            // crop the image
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            image = image.getSubimage(0, 0, 64, 32);
            ImageIO.write(image, "png", out);

            return new CustomUrlConnection(out.toByteArray());
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
