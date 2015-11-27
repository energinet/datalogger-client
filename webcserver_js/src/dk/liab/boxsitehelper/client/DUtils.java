package dk.liab.boxsitehelper.client;

public class DUtils {
	public static native void warn(String message) /*-{
	 console.warn(message);
	}-*/;
	public static native void log(String message) /*-{
	 console.log(message);
	}-*/;
	public static native void error(String message) /*-{
	 console.error(message);
	}-*/;
}
