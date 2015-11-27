package dk.liab.boxsitehelper.client;

import java.util.ArrayList;
import java.util.List;

import com.google.gwt.core.client.JavaScriptObject;
import com.google.gwt.core.client.JsArray;
import com.google.gwt.core.client.JsDate;
import com.google.gwt.event.logical.shared.HasValueChangeHandlers;
import com.google.gwt.event.logical.shared.ValueChangeEvent;
import com.google.gwt.event.logical.shared.ValueChangeHandler;
import com.google.gwt.event.shared.GwtEvent;
import com.google.gwt.event.shared.HandlerManager;
import com.google.gwt.event.shared.HandlerRegistration;

public class TableEntries  implements HasValueChangeHandlers<List<TableEntry>>{

	private HandlerManager handlerManager;
	
	List<TableEntry> entries = new ArrayList<TableEntry>();
	
	TableEntries(){
	    handlerManager = new HandlerManager(this);
	}
	

	
   /**
	* Convert the string of JSON into JavaScript object.
	*/
    private final native JsArray<TableEntryJs> asArrayOfTableEntry(String json) /*-{
	   return eval(json);
	}-*/;

    /**
 	* Convert the string of JSON into JavaScript object.
 	*/
     private final native JsArray<TableValueJs> asArrayOfTableValue(String json) /*-{
 	   return eval(json);
 	}-*/;
	
	public void jsonParseEntries(String str){
		JsArray<TableEntryJs> jsentries = asArrayOfTableEntry(str);
		entries = new ArrayList<TableEntry>();;
		for(int i = 0; i < jsentries.length(); i++){
			TableEntryJs jsEntry = jsentries.get(i);
			entries.add(new TableEntry(jsEntry.getId(), jsEntry.getText(), jsEntry.getWriteEnabled()));
		}
		ValueChangeEvent.fire(this, this.entries);

	}
	
	public TableEntry getEntey(String id){
		for(int i = 0; i < entries.size(); i++){
			if(entries.get(i).getId()==id)
				return entries.get(i);
		}
		return null;
	}
	
	public void jsonParseValues(String str){
		JsArray<TableValueJs> jsvalues = asArrayOfTableValue(str);

		for(int i = 0; i < jsvalues.length(); i++){
			TableValueJs jsValue = jsvalues.get(i);
			TableEntry entey = getEntey(jsValue.getId());
			if(entey==null)
				continue;
			entey.updateValue(jsValue.getValue(), jsValue.getExeTime());
			
		}		
		
	}

	public boolean hasEntries(){
		return entries.size() > 0;
	}
	
	public String getValueReq(){
		String str = "[";
		
		for(int i = 0; i < entries.size(); i++){
			if(i > 0)
				str += ",";
			str += "\"" + entries.get(i).getId() + "\"";
				
		}
		
		str += "]";
		
		return str;
	}
	
	@Override
	public void fireEvent(GwtEvent<?> event) {
		handlerManager.fireEvent(event);				
	}


	@Override
	public HandlerRegistration addValueChangeHandler(
			ValueChangeHandler<List<TableEntry>> handler) {
        return handlerManager.addHandler(ValueChangeEvent.getType(), handler);
	}
}
