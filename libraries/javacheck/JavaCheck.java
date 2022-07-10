public final class JavaCheck {

    private static final String[] CHECKED_PROPERTIES = new String[] {
            "os.arch",
            "java.version",
            "java.vendor"
    };

    public static void main(String[] args) {
        int returnCode = 0;

        for (String key : CHECKED_PROPERTIES) {
            String property = System.getProperty(key);

            if (property != null) {
                System.out.println(key + "=" + property);
            } else {
                returnCode = 1;
            }
        }

        System.exit(returnCode);
    }

}
