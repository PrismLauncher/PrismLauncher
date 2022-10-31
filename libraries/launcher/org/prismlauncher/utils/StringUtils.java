package org.prismlauncher.utils;


public final class StringUtils {
    private StringUtils() {
    }
    
    public static String[] splitStringPair(char splitChar, String input) {
        int splitPoint = input.indexOf(splitChar);
        if (splitPoint == -1)
            return null;
        
        return new String[]{ input.substring(0, splitPoint), input.substring(splitPoint + 1) };
    }
}
