package dk.liab.boxsitehelper.client;

import java.util.Date;

import com.google.gwt.core.client.JsDate;
import com.google.gwt.event.logical.shared.HasValueChangeHandlers;
import com.google.gwt.event.logical.shared.ValueChangeEvent;
import com.google.gwt.event.logical.shared.ValueChangeHandler;
import com.google.gwt.event.shared.GwtEvent;
import com.google.gwt.event.shared.HandlerManager;
import com.google.gwt.event.shared.HandlerRegistration;

public class TableEntry implements HasValueChangeHandlers<String>{

	
	public enum UpdateType { Server, User }; 
	
	private HandlerManager handlerManager;
	
	private String id;
	private String text;
	private String value;
	private JsDate timeExec;
	private boolean writeEnabled;
	
	TableEntry(String id, String text, boolean writeEnabled){
	    handlerManager = new HandlerManager(this);
	    this.id = id;
	    this.text = text;
	    this.writeEnabled = writeEnabled;
	}

	public void updateValue(String value, JsDate timeExec){
		this.value = value;
		this.timeExec = timeExec;
		ValueChangeEvent.fire(this, value);
	}
	
	public String getId(){
		return id;
	}

	public String getText(){
		return id;
	}
	
	public boolean isWritable(){
		return writeEnabled;
	}

	
	@Override
	public void fireEvent(GwtEvent<?> event) {
		handlerManager.fireEvent(event);		
	}


	@Override
	public HandlerRegistration addValueChangeHandler(
			ValueChangeHandler<String> handler) {
        return handlerManager.addHandler(ValueChangeEvent.getType(), handler);
	}
	
	
}
