package dk.liab.boxsitehelper.client;

import java.util.List;

import com.google.gwt.core.client.JsArray;
import com.google.gwt.event.logical.shared.HasValueChangeHandlers;
import com.google.gwt.event.logical.shared.ValueChangeEvent;
import com.google.gwt.event.logical.shared.ValueChangeHandler;
import com.google.gwt.event.shared.GwtEvent;
import com.google.gwt.event.shared.HandlerManager;
import com.google.gwt.event.shared.HandlerRegistration;
import com.google.gwt.user.client.ui.Grid;
import com.google.gwt.user.client.ui.HTML;


public class TableView extends Grid  implements HasValueChangeHandlers<ValueSet>{
	
	TableEntries entries;
	private HandlerManager handlerManager;

	
	public TableView(TableEntries entries) {
		super(2,2);
	    handlerManager = new HandlerManager(this);

		entries.addValueChangeHandler(new ValueChangeHandler<List<TableEntry>>(){

			@Override
			public void onValueChange(ValueChangeEvent<List<TableEntry>> event) {
				setNames(event.getValue());
			}
			
		});
	}

	
	void setNames(List<TableEntry> list){
		
		 DUtils.log("grid updating" );
		
		this.resizeRows(list.size());
		
		for(int i = 0; i < list.size(); i++){
			HTML text = new HTML(list.get(i).getText()+":");
			TableViewEntey value = new TableViewEntey(list.get(i));
			
			value.addValueChangeHandler(new ValueChangeHandler<ValueSet>(){

				@Override
				public void onValueChange(ValueChangeEvent<ValueSet> event) {
					hasSetValue(event.getValue());
				}
				
			});
			
			this.setWidget(i, 0, text);
			this.setWidget(i, 1, value);
			
		}
		
		 DUtils.log("grid updated" );

	}
	
	public void hasSetValue(ValueSet setvalue){
		ValueChangeEvent.fire(this, setvalue);
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
