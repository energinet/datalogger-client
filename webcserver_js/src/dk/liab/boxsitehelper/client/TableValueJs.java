package dk.liab.boxsitehelper.client;

import com.google.gwt.core.client.JavaScriptObject;
import com.google.gwt.core.client.JsDate;

public class TableValueJs extends JavaScriptObject{
	protected TableValueJs(){};	
	public final native String getId() /*-{ return this.id; }-*/; 
	public final native String getValue() /*-{ return this.value; }-*/; 
	public final native JsDate getExeTime() /*-{ return this.exe_time; }-*/;
}