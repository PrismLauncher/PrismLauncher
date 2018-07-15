/*
 * Copyright 2012-2018 MultiMC Contributors
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

public class ParamBucket
{
    public void add(String key, String value)
    {
        List<String> coll = null;
        if(!m_params.containsKey(key))
        {
            coll = new ArrayList<String>();
            m_params.put(key, coll);
        }
        else
        {
            coll = m_params.get(key);
        }
        coll.add(value);
    }

    public List<String> all(String key) throws NotFoundException
    {
        if(!m_params.containsKey(key))
            throw new NotFoundException();
        return m_params.get(key);
    }

    public List<String> allSafe(String key, List<String> def)
    {
        if(!m_params.containsKey(key) || m_params.get(key).size() < 1)
        {
            return def;
        }
        return m_params.get(key);
    }

    public List<String> allSafe(String key)
    {
        return allSafe(key, new ArrayList<String>());
    }

    public String first(String key) throws NotFoundException
    {
        List<String> list = all(key);
        if(list.size() < 1)
        {
            throw new NotFoundException();
        }
        return list.get(0);
    }

    public String firstSafe(String key, String def)
    {
        if(!m_params.containsKey(key) || m_params.get(key).size() < 1)
        {
            return def;
        }
        return m_params.get(key).get(0);
    }

    public String firstSafe(String key)
    {
        return firstSafe(key, "");
    }

    private HashMap<String, List<String>> m_params = new HashMap<String, List<String>>();
}
