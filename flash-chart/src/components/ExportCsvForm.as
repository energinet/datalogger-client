package components
{
    import mx.containers.Form;
    import mx.containers.FormItem;
    import mx.containers.HBox;
    import mx.controls.DateField;
    import mx.controls.Button;
    import mx.controls.ComboBox;
    import mx.containers.TitleWindow;
    import mx.managers.PopUpManager;
    import flash.events.Event;
    import flash.net.FileReference;
    import flash.net.URLRequest;
    import flash.net.URLRequestMethod;
    import flash.net.URLVariables;
    import mx.collections.ArrayCollection;

    public class ExportCsvForm extends TitleWindow
    {
        [Embed(source="../assets/set.png")] 
        private var submitIcon:Class; 

        [Embed(source="../assets/cancel.png")] 
        private var cancelIcon:Class; 


        private var _baseUrl:String;

        private var _startDate:DateField;
        private var _endDate:DateField;
        private var _submit:Button;
        private var _cancel:Button;
        private var _popButton:ComboBox; 
        private var _eTypes:Array;
	

        public var intervals:ArrayCollection = new ArrayCollection(
                [
                  {label:"5 min", data:300}, 
                  {label:"1 time", data:3600}, 
	          {label:"4 timer", data:14400},
  	          {label:"24 timer", data:86400}
		    ]);


        private var fileToDownload:FileReference;

        
        public function ExportCsvForm(){
            super();
           
            var item:FormItem;
            
            _startDate = new DateField();
           
            _endDate   = new DateField();
            _startDate.formatString="DD.MM.YYYY";
            _endDate.formatString="DD.MM.YYYY";
            _startDate.firstDayOfWeek="1";
            _endDate.firstDayOfWeek="1";
	    
            _startDate.yearNavigationEnabled=true;
            _endDate.yearNavigationEnabled=true;
            _submit    = new Button();
            _cancel    = new Button();
            _popButton   = new ComboBox(); 
            _popButton.dataProvider = intervals;
            _popButton.selectedIndex = 2;
            _submit.label = "OK";
            _submit.setStyle("icon", submitIcon); 
            _submit.addEventListener("click",submitButton);            
            _cancel.label = "Cancel";
            _cancel.setStyle("icon" ,cancelIcon);
            _cancel.addEventListener("click",cancelButton);  

            var form:Form = new Form();


            item = new FormItem();
            item.label = "Start";
            item.addChild(_startDate);
            form.addChild(item);
         
            item = new FormItem();
            item.label = "End";
            item.addChild(_endDate);
            form.addChild(item);
            
            item = new FormItem();
            item.label = "Interval";
            item.addChild(_popButton);
            form.addChild(item);

            var box:HBox = new HBox();

            box.addChild(_submit);
            box.addChild(_cancel);

            form.addChild(box);

            this.addChild(form);
            
	    interval = 300;

        }

        public function set baseUrl(baseUrl:String):void{
            _baseUrl = baseUrl;
        }

        public function setETypes(eTypes:Array):void{
            _eTypes = eTypes;
        }

        public function set startDate(date:Date):void{
            _startDate.selectedDate = date;
        }


        public function set endDate(date:Date):void{
            _endDate.selectedDate = date;
        }

	public function set interval(interval_:Number):void{
	    var item:int = 0;
	    for( var i:String in intervals){
		if(interval_ > intervals[i].data)
		    item++;
	    }
	    
	    _popButton.selectedIndex = item;
	    
	}

        private function submitButton(event:Event):void{
         
            DownloadFileExample();
            removeThis();
           
        }

        private function cancelButton(event:Event):void{

            removeThis();
        }

        public function removeThis():void{
         
            PopUpManager.removePopUp(this);
        }

        public function mkVarStr():String{
            var str:String = "";

            str += "timeStart=" +_startDate.selectedDate.time/1000 
            str += "&timeEnd="+_endDate.selectedDate.time/1000;
            str += "&interval=" + _popButton.selectedItem.data;
	    

            if(_eTypes){
		str += "&iid=" + _eTypes[0].iid;
                for(var i:String in _eTypes){
                    if(_eTypes[i].enabled){
                        str +=  "&eid=" + _eTypes[i].eid;
                    }
                }
            }
            return str;
        }


        public function DownloadFileExample():void {
            
            var request:URLRequest = new URLRequest();
            request.url = _baseUrl + "/cgi-bin/csv_export.cgi";
                           

            request.method = URLRequestMethod.GET;
            request.data = new URLVariables(mkVarStr());
            fileToDownload = new FileReference();
            try {
                fileToDownload.addEventListener(Event.COMPLETE, downloadCompleteHandler);
                
                fileToDownload.download(request, "log.csv");
            }
            catch (error:Error){
                trace("Unable to download file.");
            }
        }

        public function downloadCompleteHandler(event:Event):void {
            trace("Download ok");
            
        }




    }

}