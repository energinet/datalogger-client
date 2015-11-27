package components
{
    import components.EventLog;
    
    public class EventFilter
    {

	private var unitShow:Array = new Array();
	private var filterUnits:Boolean = false;
	private var enameShow:Array = new Array();
	private var filterEnames:Boolean = false;
	private var groupShow:Array = new Array();
	private var filterGroups:Boolean = false;
	private var showAll:Boolean = false;
	private var showSystem:Boolean = false;

	public function EventFilter(show:String = null){
	    trace("show :" + show);
	    var list:Array = show.split(';'); 
	    for(var i:String in list){
		trace("show list "+i+":" + list[i]);
		fillFiler(list[i]);
	    }

	    trace("filterUnits:" +filterUnits);
	    trace("filterEnames:" +filterEnames);
	    trace("filterGroups:" +filterGroups);
	    
	}


	private function fillFiler(show:String):void{
	    var substr:Array = show.split(':'); 
	    var cmd:String  = substr[0];
	    var param:String = substr[1];
	    
	    switch(substr[0]){
	      case "all":
		showAll = true;
		break;
	      case "unit":
		unitShow = param.split(',');
		filterUnits = true;
		break;
	      case "ename":
		enameShow = param.split(',');
		filterEnames = true;
		break;
	      case "group":
		groupShow = param.split(',');
		filterGroups = true;
		break;
	      default:
		trace("unknown show string:" + show);
		break
	    }
	    
	}

	private function testList(str:String, list:Array):Boolean{
	    for(var i:String in list){
		if(str==list[i])
		    return true;
	    }

	    return false;
	}

	public function testEvent(event:EventLog):Boolean{
	    
	    var show:Boolean = true;

	    if(showAll)
		return true;

	     if(!event.show)
		 return false;

	     if(filterUnits){
		 if(((event.showUnit != "") && !testList(event.unit, unitShow)) && !testList(event.showUnit, unitShow))
		    show = false;
	     }
	    

	    if(filterEnames)
		if(!testList(event.ename, enameShow))
		    show = false;

//	    if(filterGroups)
//		show &= testList(event.ename, unitShow);

	    return show;

	}

	public function Filter(eTypes:Array):Array{
	    var  eTypesRet:Array = new Array();
	    
	    for(var i:String in eTypes){
		if(testEvent(eTypes[i]))
		   eTypesRet.push(eTypes[i]);
	    }

	    return eTypesRet;
	    
	}

	
	public function Units():Array{
	    return unitShow;
	}
	
    }


}