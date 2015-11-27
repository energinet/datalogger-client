package dk.liab.boxsitehelper.client;

import com.google.gwt.core.client.JsArray;
import com.google.gwt.http.client.Request;
import com.google.gwt.http.client.RequestBuilder;
import com.google.gwt.http.client.RequestCallback;
import com.google.gwt.http.client.RequestException;
import com.google.gwt.http.client.Response;
import com.google.gwt.user.client.Timer;


public class JsonRpcHandler {

	TableView tableView;
	
	int interval = 2000;
	
	TableEntries entries = new TableEntries();
	
	final Timer timer = new Timer() {
		@Override
		public void run(){
			getData();
		}
	};
	
	JsonRpcHandler(TableEntries entries){
		this.entries = entries;
		timer.schedule(100);
	}
	
	void displayError(String str){
		
	}	
	
	
	void getData()
	{
		if(!entries.hasEntries()){
			this.get("/cgi-bin/cmd_json.cgi?list", false);
		} else {
			this.get("/cgi-bin/cmd_json.cgi?get=" +entries.getValueReq(), true);
		}
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
	  private final native String asJsonArrayEntry(JsArray<TableEntryJs> list) /*-{
	    var a =[2,4,5,6,7,78];
	    for(var i = 0; i < list.length; i++){
	   	   a[i] = list[i].id;
	    }
	   
	   
	     var text = a.toString();

	     return "[" + text + "]";
	  }-*/;
		
	void get(String url , final boolean values){
	 RequestBuilder builder = new RequestBuilder(RequestBuilder.GET, url);

	 try {
	      Request request = builder.sendRequest(null, new RequestCallback() {
	        public void onError(Request request, Throwable exception) {
	          displayError("Couldn't retrieve JSON");
	        }

	        public void onResponseReceived(Request request, Response response) {
	          if (200 == response.getStatusCode()) {

	        	  if(values){
	        		  entries.jsonParseValues(response.getText());
	        		  timer.schedule(interval);
	        	  } else {
	        		  entries.jsonParseEntries(response.getText());
	        		  timer.run();
	        	  }
	          } else {
	            displayError("Couldn't retrieve JSON (" + response.getStatusText()
	                + ")");
      		  timer.schedule(interval*10);

	          }
	        }
	      });
	    } catch (RequestException e) {
	      displayError("Couldn't retrieve JSON");
  		  timer.schedule(interval*10);

	    }
	}
	
	
	public void setValue(ValueSet value){
		this.get("/cgi-bin/cmd_json.cgi?set=[\"" + value.getId() + "\":\"" + value.getValue() + "\"]", true);
	}
}
