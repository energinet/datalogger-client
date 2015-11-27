package dk.liab.boxsitehelper.client;


public class ValueSet{

	private String id, value;
	
	public ValueSet(String id, String value) {
		this.id = id;
		this.value = value;
	}
	
	public String getId(){
		return this.id;
	}
	
	public String getValue(){
		return this.value;
	}
	
}
