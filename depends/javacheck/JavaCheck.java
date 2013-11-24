import java.lang.Integer;

public class JavaCheck
{
	private static final String key = "os.arch";
	public static void main (String [] args)
	{
		String property = System.getProperty(key);
		System.out.println(key + "=" + property);
		if (property != null)
			System.exit(0);
		System.exit(1);
	}
}
