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

package org.prismlauncher.utils;

import org.prismlauncher.exception.ParameterNotFoundException;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public final class Parameters {

    private final Map<String, List<String>> map = new HashMap<>();

    public void add(String key, String value) {
        List<String> params = map.get(key);

        if (params == null) {
            params = new ArrayList<>();

            map.put(key, params);
        }

        params.add(value);
    }

    public List<String> getList(String key) throws ParameterNotFoundException {
        List<String> params = map.get(key);
    
        if (params == null)
            throw new ParameterNotFoundException(key);
    
        return params;
    }
    
    public List<String> getList(String key, List<String> def) {
        List<String> params = map.get(key);
        
        if (params == null || params.isEmpty())
            return def;
        
        return params;
    }
    
    public String getString(String key) throws ParameterNotFoundException {
        List<String> list = getList(key);
        
        if (list.isEmpty())
            throw new ParameterNotFoundException(key);
        
        return list.get(0);
    }
    
    public String getString(String key, String def) {
        List<String> params = map.get(key);
        
        if (params == null || params.isEmpty())
            return def;
        
        return params.get(0);
    }

}
