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

public class EntryPoint
{

    private final ParamBucket params = new ParamBucket();

    private org.multimc.Launcher launcher;

    public static void main(String[] args)
    {
        EntryPoint listener = new EntryPoint();

        int retCode = listener.listen();

        if (retCode != 0)
        {
            System.out.println("Exiting with " + retCode);
            System.exit(retCode);
        }
    }

    private Action parseLine(String inData) throws ParseException
    {
        String[] pair = inData.split("\\s+", 2);

        if (pair.length == 0)
            throw new ParseException("Unexpected empty string!");

        switch (pair[0]) {
            case "launch": {
                return Action.Launch;
            }

            case "abort": {
                return Action.Abort;
            }

            case "launcher": {
                if (pair.length != 2)
                    throw new ParseException("Expected 2 tokens, got 1!");

                if (pair[1].equals("onesix")) {
                    launcher = new OneSixLauncher();

                    Utils.log("Using onesix launcher.");

                    return Action.Proceed;
                } else {
                    throw new ParseException("Invalid launcher type: " + pair[1]);
                }
            }

            default: {
                if (pair.length != 2)
                    throw new ParseException("Error while parsing:" + pair[0]);

                params.add(pair[0], pair[1]);

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
            Utils.log("Launcher ABORT due to exception:");

            e.printStackTrace();

            return 1;
        }

        // Main loop
        if (action == Action.Abort)
        {
            System.err.println("Launch aborted by the launcher.");
            return 1;
        }

        if (launcher != null)
        {
            return launcher.launch(params);
        }

        System.err.println("No valid launcher implementation specified.");

        return 1;
    }

    private enum Action {
        Proceed,
        Launch,
        Abort
    }

}
