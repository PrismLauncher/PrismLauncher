package org.multimc;/*
 * Copyright 2012-2021 MultiMC Contributors
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

import org.multimc.onesix.OneSixLauncher;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.logging.Level;
import java.util.logging.Logger;

public class EntryPoint
{

    private static final Logger LOGGER = Logger.getLogger("EntryPoint");

    private final ParamBucket params = new ParamBucket();

    private org.multimc.Launcher launcher;

    public static void main(String[] args)
    {
        EntryPoint listener = new EntryPoint();

        int retCode = listener.listen();

        if (retCode != 0)
        {
            LOGGER.info("Exiting with " + retCode);

            System.exit(retCode);
        }
    }

    private Action parseLine(String inData) throws ParseException
    {
        String[] tokens = inData.split("\\s+", 2);

        if (tokens.length == 0)
            throw new ParseException("Unexpected empty string!");

        switch (tokens[0]) {
            case "launch": {
                return Action.Launch;
            }

            case "abort": {
                return Action.Abort;
            }

            case "launcher": {
                if (tokens.length != 2)
                    throw new ParseException("Expected 2 tokens, got " + tokens.length);

                if (tokens[1].equals("onesix")) {
                    launcher = new OneSixLauncher();

                    LOGGER.info("Using onesix launcher.");

                    return Action.Proceed;
                } else {
                    throw new ParseException("Invalid launcher type: " + tokens[1]);
                }
            }

            default: {
                if (tokens.length != 2)
                    throw new ParseException("Error while parsing:" + inData);

                params.add(tokens[0], tokens[1]);

                return Action.Proceed;
            }
        }
    }

    public int listen()
    {
        Action action = Action.Proceed;

        try (BufferedReader reader = new BufferedReader(new InputStreamReader(
                System.in,
                StandardCharsets.UTF_8
        ))) {
            String line;

            while (action == Action.Proceed) {
                if ((line = reader.readLine()) != null) {
                    action = parseLine(line);
                } else {
                    action = Action.Abort;
                }
            }
        } catch (IOException | ParseException e) {
            LOGGER.log(Level.SEVERE, "Launcher ABORT due to exception:", e);

            return 1;
        }

        // Main loop
        if (action == Action.Abort)
        {
            LOGGER.info("Launch aborted by the launcher.");

            return 1;
        }

        if (launcher != null)
        {
            return launcher.launch(params);
        }

        LOGGER.log(Level.SEVERE, "No valid launcher implementation specified.");

        return 1;
    }

    private enum Action {
        Proceed,
        Launch,
        Abort
    }

}
