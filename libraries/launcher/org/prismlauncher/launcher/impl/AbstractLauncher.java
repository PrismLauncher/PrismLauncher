// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher
 *
 *  Copyright (C) 2022 icelimetea <fr3shtea@outlook.com>
 *  Copyright (C) 2022 TheKodeToad <TheKodeToad@proton.me>
 *  Copyright (C) 2022 solonovamax <solonovamax@12oclockpoint.com>
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

package org.prismlauncher.launcher.impl;

import org.prismlauncher.exception.ParseException;
import org.prismlauncher.launcher.Launcher;
import org.prismlauncher.utils.Parameters;
import org.prismlauncher.utils.StringUtils;

import java.util.ArrayList;
import java.util.List;

public abstract class AbstractLauncher implements Launcher {

    private static final int DEFAULT_WINDOW_WIDTH = 854;
    private static final int DEFAULT_WINDOW_HEIGHT = 480;

    // parameters, separated from ParamBucket
    protected final List<String> mcParams;

    // secondary parameters
    protected final int width, height;
    protected final boolean maximize;
    protected final String serverAddress, serverPort;

    protected final String mainClassName;

    protected AbstractLauncher(Parameters params) {
        this.mcParams = params.getList("param", new ArrayList<String>());
        this.mainClassName = params.getString("mainClass", "net.minecraft.client.Minecraft");

        this.serverAddress = params.getString("serverAddress", null);
        this.serverPort = params.getString("serverPort", null);

        String windowParams = params.getString("windowParams", null);

        this.maximize = "max".equalsIgnoreCase(windowParams);

        if (windowParams != null && !"max".equalsIgnoreCase(windowParams)) {
            String[] sizePair = StringUtils.splitStringPair('x', windowParams);

            if (sizePair != null) {
                try {
                    this.width = Integer.parseInt(sizePair[0]);
                    this.height = Integer.parseInt(sizePair[1]);
                } catch (NumberFormatException e) {
                    throw new ParseException(String.format("Could not parse window parameters from '%s'", windowParams),
                            e);
                }
            } else {
                throw new ParseException(
                        String.format("Invalid window size parameters '%s'. Format: [height]x[width]", windowParams));
            }
        } else {
            this.width = DEFAULT_WINDOW_WIDTH;
            this.height = DEFAULT_WINDOW_HEIGHT;
        }
    }

}
