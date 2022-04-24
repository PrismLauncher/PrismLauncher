/*
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

package org.multimc;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class ParamBucket
{

    private final Map<String, List<String>> paramsMap = new HashMap<>();

    public void add(String key, String value)
    {
        paramsMap.computeIfAbsent(key, k -> new ArrayList<>())
                .add(value);
    }

    public List<String> all(String key) throws NotFoundException
    {
        List<String> params = paramsMap.get(key);

        if (params == null)
            throw new NotFoundException();

        return params;
    }

    public List<String> allSafe(String key, List<String> def)
    {
        List<String> params = paramsMap.get(key);

        if (params == null || params.isEmpty())
            return def;

        return params;
    }

    public List<String> allSafe(String key)
    {
        return allSafe(key, new ArrayList<>());
    }

    public String first(String key) throws NotFoundException
    {
        List<String> list = all(key);

        if (list.isEmpty())
            throw new NotFoundException();

        return list.get(0);
    }

    public String firstSafe(String key, String def)
    {
        List<String> params = paramsMap.get(key);

        if (params == null || params.isEmpty())
            return def;

        return params.get(0);
    }

    public String firstSafe(String key)
    {
        return firstSafe(key, "");
    }

}
