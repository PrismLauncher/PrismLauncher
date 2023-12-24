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

package org.prismlauncher.legacy.fix.online;

import org.prismlauncher.legacy.utils.Base64;
import org.prismlauncher.legacy.utils.url.UrlUtils;
import org.prismlauncher.utils.Parameters;
import org.prismlauncher.utils.logging.Log;

import java.net.URL;
import java.net.URLStreamHandler;
import java.net.URLStreamHandlerFactory;

/**
 * Fixes skins by redirecting to other URLs.
 * Thanks to MineOnline for the implementation from which this was inspired!
 * See https://github.com/ahnewark/MineOnline/tree/main/src/main/java/gg/codie/mineonline/protocol.
 *
 * @see {@link Handler}
 * @see {@link UrlUtils}
 */
public final class OnlineFixes implements URLStreamHandlerFactory {
    public static void apply(Parameters params) {
        if (!"true".equals(params.getString("onlineFixes", null)))
            return;

        if (!UrlUtils.isSupported() || !Base64.isSupported()) {
            Log.warning("Cannot access the necessary Java internals for skin fix");
            Log.warning("Turning off online fixes in the settings will silence the warnings");
            return;
        }

        try {
            URL.setURLStreamHandlerFactory(new OnlineFixes());
        } catch (Error e) {
            Log.warning("Cannot apply skin fix: URLStreamHandlerFactory is already set");
            Log.warning("Turning off online fixes in the settings will silence the warnings");
        }
    }

    @Override
    public URLStreamHandler createURLStreamHandler(String protocol) {
        if ("http".equals(protocol))
            return new Handler();

        return null;
    }
}
