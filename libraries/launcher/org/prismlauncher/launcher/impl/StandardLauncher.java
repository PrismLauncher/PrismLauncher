/* Copyright 2012-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.prismlauncher.launcher.impl;

import org.prismlauncher.utils.Parameters;

public final class StandardLauncher extends AbstractLauncher {

	public StandardLauncher(Parameters params) {
		super(params);
	}

	@Override
    public void launch() throws Throwable {
        // window size, title and state

        // FIXME: there is no good way to maximize the minecraft window from here.
        // the following often breaks linux screen setups
        // mcparams.add("--fullscreen");

        if (!maximize) {
            mcParams.add("--width");
            mcParams.add(Integer.toString(width));
            mcParams.add("--height");
            mcParams.add(Integer.toString(height));
        }

        if (serverAddress != null) {
            mcParams.add("--server");
            mcParams.add(serverAddress);
            mcParams.add("--port");
            mcParams.add(serverPort);
        }

        loadAndInvokeMain();
    }

}
