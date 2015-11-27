package components
{

    import mx.containers.HBox;
    import mx.controls.DateField;
    import mx.controls.Button;
    import mx.controls.ComboBox;
    import mx.controls.Spacer;
    import flash.events.Event;
    import mx.collections.ArrayCollection;

    public class DateSelect extends HBox
    {

	private var _selectedDate:Date = new Date();
	private var dateField:DateField;
	private var popList:ComboBox; 
	private var _dayViewEnabled:Boolean = true;
	private var nextButton:Button ;
	private var prevButton:Button ;
	private var callback:Function = null;
	private var _interval:String = "day";
	private var lastSelectedDate:Date = new Date();

	public var intervals:ArrayCollection = new ArrayCollection(
	    [ {label:"4 timer"  ,  label_en:"4 hours",  data:"4hour"}, 
	      {label:"Dag"  ,  label_en:"Day",  data:"day"}, 
	      {label:"Uge"  ,  label_en:"Week",  data:"week"}, 
    	      {label:"Måned",  label_en:"Month", data:"month"}, 
              {label:"År"   ,  label_en:"Year",  data:"year"}, 
		    ]);

        public function DateSelect():void{
	    super();
	    
	    this._selectedDate =  new Date();
	    this._selectedDate.setHours(0,0);

	    dateField = new DateField();
            dateField.firstDayOfWeek="1";
	    dateField.yearNavigationEnabled=true;
	    dateField.selectedDate = this._selectedDate; //new Date();
	    dateField.addEventListener("change", dateSelected);
	    dateField.formatString="DD.MM.YYYY";

	    popList = new ComboBox(); 
	    popList.dataProvider = intervals;
	    popList.selectedIndex = 1;
	    popList.addEventListener("close",intervalSelected);      

	    prevButton  = new Button();	    
	    prevButton.label = "<";
	    prevButton.width = 25;
	    prevButton.addEventListener("click",prevSelected);            

	    nextButton  = new Button();	    
	    nextButton.label = ">";
	    nextButton.width = 25;
	    nextButton.addEventListener("click",nextSelected);      

	    var spacerL:Spacer = new Spacer();
	    spacerL.percentWidth=20;

	    var spacerR:Spacer = new Spacer();
	    spacerR.percentWidth=20;


	    this.addChild(prevButton);
	    this.addChild(spacerL);

	    this.addChild(dateField);

	    if(_dayViewEnabled)
		this.addChild(popList);

	    this.addChild(spacerR);
	    this.addChild(nextButton);

        }

	private function set selectedDate(date:Date):void{
	    dateField.selectedDate = date;
	    this._selectedDate = date;
	}

	private function get selectedDate():Date{
	    return this._selectedDate;
	}

	public function get interval():String{
	    return popList.selectedItem.data;
	}

	public function get dateStart():Date{
//	    var start:Date = new Date(dateField.selectedDate);
	    var start:Date = new Date(this.selectedDate);
	   
	    return start;
	}

	public function get dateEnd():Date{
//	    var end:Date = new Date(dateField.selectedDate);
	    var end:Date = new Date(this.selectedDate);

	    //end.setHours(0,0);
	    switch(popList.selectedItem.data){
	      case "4hour":
		end.setTime(end.getTime() + 1000*60*60*4);
		break;
	      case "day":
		end.setDate(end.getDate()+1);
		break;
	      case "week":
		end.setDate(end.getDate()+7);
		break;
	      case "month":
		end.setMonth(end.getMonth()+1);
		break;
	      case "year":
		end.setMonth(end.getMonth()+12);
	    }
	    return end;
	}

	public function setCallback (callback_:Function):void{
	    trace("setcallback");
	    callback = callback_;
	}

	private function update():void{
	    trace("update");
	    if(callback!=null)
		callback();
	    
	}

	private function addTime(toadd:int):void{
	    trace("add " + popList.selectedItem.data + " " + toadd);
//	    var date:Date = new Date(dateField.selectedDate);
	    var date:Date = new Date(this.selectedDate);
	    switch(popList.selectedItem.data){
	      case "4hour":
		date.setTime(date.getTime() + 1000*60*60*4*toadd);
		trace("new date:" + date + "   (" + 1000*60*60*4*toadd + ")");
		break;
	      case "day":
		date.setDate(date.getDate()+ (toadd));
		date.setHours(0,0);
		break;
	      case "week":
		date.setDate(date.getDate()+ (7*toadd));
		date.setHours(0,0);
		break;
	      case "month":
		date.setMonth(date.getMonth()+toadd);
		date.setHours(0,0);
		break;
	      case "year":
		date.setMonth(date.getMonth()+toadd*12);
		date.setHours(0,0);
	    }
	    

	    this.selectedDate = date;
	    lastSelectedDate = date;
	    
	    update();
	}

	private function prevSelected (event:Event):void{
	    trace("prevSelected");
	    addTime(-1);
	}

	private function nextSelected (event:Event):void{
	    trace("nextSelected");
	    addTime(1);
	}
	
        
	private function dateSelected (event:Event):void{
	    trace("dateSelected");
	    this._selectedDate = dateField.selectedDate;
	    lastSelectedDate = this.selectedDate;
	    update();
	}

	private function intervalSelected(event:Event):void{
	    trace("intervalSelected");
	    var selected:Date = new Date(this.selectedDate);
	    
	     switch(popList.selectedItem.data){
	      case "day":
		selected = lastSelectedDate;
		selected.setHours(0,0);
		break;
	      case "week":
		var weekday:Number = lastSelectedDate.getDay()-1;
		if(weekday<0)
		    weekday +=7; // Sunday = -1 --> 6
		var monday:Number = lastSelectedDate.getDate()-weekday;
		selected.setMonth(lastSelectedDate.getMonth(),monday );		
		selected.setHours(0,0);
		break;
	      case "month":
		selected.setMonth(lastSelectedDate.getMonth(), 1);
		selected.setHours(0,0);
		break;
	      case "year":
		selected.setMonth(0, 1);
		selected.setHours(0,0);
		break;
	    }

	     this.selectedDate = selected;

	     _interval = popList.selectedItem.data;

	    update();
	}


    }
}