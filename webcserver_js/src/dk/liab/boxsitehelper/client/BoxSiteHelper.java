package dk.liab.boxsitehelper.client;

import com.google.gwt.core.client.EntryPoint;
import com.google.gwt.event.logical.shared.ValueChangeEvent;
import com.google.gwt.event.logical.shared.ValueChangeHandler;
import com.google.gwt.user.client.ui.RootPanel;


public class BoxSiteHelper  implements EntryPoint{

	TableEntries entries = new TableEntries();
	
	@Override
	public void onModuleLoad() {
		
		TableView view = new TableView(entries);
		if(RootPanel.get("siteHelper")!=null)
			RootPanel.get("siteHelper").add(view);
		
		final JsonRpcHandler rpc = new JsonRpcHandler(entries);
		
		view.addValueChangeHandler(new ValueChangeHandler<ValueSet>(){

			@Override
			public void onValueChange(ValueChangeEvent<ValueSet> event) {
				rpc.setValue(event.getValue());
			}});
		
	}

}
