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
 */

package org.prismlauncher.legacy.fix.online;

import org.prismlauncher.legacy.utils.url.UrlUtils;

import java.io.IOException;
import java.net.MalformedURLException;
import java.net.Proxy;
import java.net.URL;
import java.net.URLConnection;

public final class OnlineModeFix {
    public static URLConnection openConnection(URL address, Proxy proxy) throws IOException {
        // we start with "http://www.minecraft.net/game/joinserver.jsp?user=..."
        if (!(address.getHost().equals("www.minecraft.net") && address.getPath().equals("/game/joinserver.jsp")))
            return null;

        // change it to "https://session.minecraft.net/game/joinserver.jsp?user=..."
        // this seems to be the modern version of the same endpoint...
        // maybe Mojang planned to patch old versions of the game to use it
        // if it ever disappears this should be changed to use sessionserver.mojang.com/session/minecraft/join
        // which of course has a different usage requiring JSON serialisation...
        URL url;
        try {
            url = new URL("https", "session.minecraft.net", address.getPort(), address.getFile());
        } catch (MalformedURLException e) {
            throw new AssertionError("url should be valid", e);
        }

        return UrlUtils.openConnection(url, proxy);
    }
}
