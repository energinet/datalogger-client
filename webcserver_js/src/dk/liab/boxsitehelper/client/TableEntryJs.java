package dk.liab.boxsitehelper.client;

import com.google.gwt.core.client.JavaScriptObject;

public class TableEntryJs extends JavaScriptObject{
	
		protected TableEntryJs(){};	
	
	  public final native String getId() /*-{ return this.id; }-*/; 
	  public final native String getText() /*-{ return this.text; }-*/;
	  public final native boolean getWriteEnabled() /*-{ return this.write; }-*/;
}
