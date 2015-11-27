package dk.liab.boxsitehelper.client;

import java.util.List;

import com.google.gwt.core.client.Scheduler;
import com.google.gwt.core.client.Scheduler.ScheduledCommand;
import com.google.gwt.event.dom.client.ClickEvent;
import com.google.gwt.event.dom.client.ClickHandler;
import com.google.gwt.event.dom.client.KeyCodes;
import com.google.gwt.event.dom.client.KeyUpEvent;
import com.google.gwt.event.dom.client.KeyUpHandler;
import com.google.gwt.event.logical.shared.HasValueChangeHandlers;
import com.google.gwt.event.logical.shared.ValueChangeEvent;
import com.google.gwt.event.logical.shared.ValueChangeHandler;
import com.google.gwt.event.shared.GwtEvent;
import com.google.gwt.event.shared.HandlerManager;
import com.google.gwt.event.shared.HandlerRegistration;
import com.google.gwt.user.client.Timer;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.FocusListener;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.TextBox;
import com.google.gwt.user.client.ui.ValueBoxBase.TextAlignment;
import com.google.gwt.user.client.ui.Widget;

public class TableViewEntey extends HorizontalPanel implements HasValueChangeHandlers<ValueSet>{

	private HandlerManager handlerManager;
	
	TextBox text = new TextBox();
	Button button = new Button();
	TableEntry entry;
	
	boolean inFocus = false;
	
	final Timer timer = new Timer() {
		@Override
		public void run(){
			endEdit();
		}
	};
	
	@SuppressWarnings("deprecation")
	public TableViewEntey(TableEntry entry){
		super();
		
	    handlerManager = new HandlerManager(this);
		
		this.add(text);
		this.add(button);
		
		text.setEnabled(entry.isWritable());
		text.setAlignment(TextAlignment.RIGHT);
		text.setWidth("70px");
		
		button.setVisible(false);
		button.setText("set");
		
		this.entry = entry;
		
		entry.addValueChangeHandler(new ValueChangeHandler<String>(){

			@Override
			public void onValueChange(ValueChangeEvent<String> event) {
				onChange(event.getValue());				
			}});
		
		text.addFocusListener(new FocusListener(){

			@Override
			public void onFocus(Widget sender) {
				startEdit();
			}

			@Override
			public void onLostFocus(Widget sender) {
//				timer.schedule(100);
//				endEdit();
				Scheduler.get().scheduleDeferred(new ScheduledCommand(){

					@Override
					public void execute() {
						endEdit();						
					}});

			}
			
		});
		
		
		text.addKeyUpHandler(new KeyUpHandler(){

			@Override
			public void onKeyUp(KeyUpEvent event) {
				
				switch(event.getNativeKeyCode()){
					case KeyCodes.KEY_ESCAPE:
						button.setFocus(true);
						endEdit();
						break;
					case KeyCodes.KEY_ENTER:
						setValue(text.getText());

						endEdit();
						break;
					default:
						startEdit();
						break;	
				}

			}});
		
			button.addClickHandler(new ClickHandler(){

			@Override
			public void onClick(ClickEvent event) {
				setValue(text.getText());
				endEdit();
			}});
	}
	
	
	void setValue(String value){
		ValueChangeEvent.fire(this, new ValueSet(entry.getId(), value));
	}
	
	
	void startEdit(){
		button.setVisible(true);
		timer.cancel();
		if(inFocus == false){
		Scheduler.get().scheduleDeferred(new ScheduledCommand(){

			@Override
			public void execute() {
				text.selectAll();
				
			}});
		}
		
		inFocus = true;
		button.setTabIndex(-1);
	}
	
	void endEdit(){
		button.setVisible(false);
		inFocus = false;
	}
	
	
	void onChange(String value){
		if(!inFocus)
			text.setText(value);
	}
	
	@Override
	public void fireEvent(GwtEvent<?> event) {
		handlerManager.fireEvent(event);				
	}


	@Override
	public HandlerRegistration addValueChangeHandler(
			ValueChangeHandler<ValueSet> handler) {
        return handlerManager.addHandler(ValueChangeEvent.getType(), handler);
	}
	
	
	
	
}
