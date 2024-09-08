// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

package org.prismlauncher.legacy.fix.online;

import org.prismlauncher.legacy.utils.api.MojangApi;
import org.prismlauncher.legacy.utils.api.Texture;
import org.prismlauncher.legacy.utils.url.ByteArrayUrlConnection;
import org.prismlauncher.legacy.utils.url.UrlUtils;

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

final class SkinFix {
    static URLConnection openConnection(URL address, Proxy proxy) throws IOException {
        String skinOwner = findSkinOwner(address);
        if (skinOwner != null)
            // we need to correct the skin
            return getSkinConnection(skinOwner, proxy);

        String capeOwner = findCapeOwner(address);
        if (capeOwner != null) {
            // since we do not need to process the image, open a direct connection bypassing
            // Handler
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
            // thank you ahnewark!
            // this is heavily based on
            // https://github.com/ahnewark/MineOnline/blob/4f4f86f9d051e0a6fd7ff0b95b2a05f7437683d7/src/main/java/gg/codie/mineonline/gui/textures/TextureHelper.java#L17
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
                // convert slim to classic
                subimage = image.getSubimage(45, 16, 9, 16);
                graphics.drawImage(subimage, 46, 16, null);

                subimage = image.getSubimage(49, 16, 2, 4);
                graphics.drawImage(subimage, 50, 16, null);

                subimage = image.getSubimage(53, 20, 2, 12);
                graphics.drawImage(subimage, 54, 20, null);
            }

            graphics.dispose();

            // crop the image - old versions disregard all secondary layers besides the hat
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            image = image.getSubimage(0, 0, 64, 32);
            ImageIO.write(image, "png", out);

            return new ByteArrayUrlConnection(out.toByteArray());
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
                if (!address.getPath().equals("/cloak/get.jsp"))
                    return null;

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
