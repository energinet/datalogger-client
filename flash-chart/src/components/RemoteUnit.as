package components
{
    import mx.collections.SortField;
    import mx.collections.Sort;
    import mx.rpc.soap.mxml.WebService;
//    import mx.rpc.soap.Operation;
    import mx.rpc.events.FaultEvent;
    import mx.rpc.events.ResultEvent;
    import mx.controls.TextArea; 
    import mx.controls.ProgressBar;
    import flash.utils.Timer;
    import flash.events.TimerEvent;
    import flash.display.Sprite;
    
    public class RemoteUnit
    {

        private var eTypeCallback:Array = new Array();
        private var eLogsCallback:Array = new Array();
        private var eLogsWaitCount:int = 0;
        private var totalWaitCount:int = 0;
        private var totalWaitReceivedCount:int = 0;
        private var totalWaitTime:int = 0;
        private var totalWaitTimeMax:int = 15;
        private var totalWaitText:String = "";
        private var wsdlfile:String = "rpc_server_flash.wsdl";
        private var installation_id:String = "1";
        public  var show:String = "";

        private var progressBar:ProgressBar = null;

        private var rpcRemote:WebService;

        public var eTypes:Array = new Array();

        //private var _unit:String = "W";
	private var _unit:Array = new Array();

	private var showFilter:EventFilter; 

	private var _interval:Number = 300;

        public function RemoteUnit(wsdl:String = null)
        {
            this.progressBar = progressBar;
            if(wsdl)
                this.wsdlfile = wsdl;

            setUpRcpRemote(this.wsdlfile);
            StartTimer();
        }
        
        public function setInstallationId(iid:String):void{
            this.installation_id = iid;
        }

	
	public function getInstallationId():String{
	    return this.installation_id;
	}


        public function setProgressBar(progressBar:ProgressBar):void{
            this.progressBar = progressBar;
//            setUpRcpRemote();
        }

        private function setUpRcpRemote(wsdl:String):void{
            rpcRemote = new WebService();
            rpcRemote.wsdl=wsdl;
            rpcRemote.addEventListener("fault", onRcpFault);
	    
	    trace("loading WSDL:"+wsdl);

            rpcRemote.loadWSDL();

            rpcRemote.getETypes.addEventListener(FaultEvent.FAULT, onRcpFault, false, 0, true);
            rpcRemote.getETypes.addEventListener(ResultEvent.RESULT, getETypesResult, false, 0, true);
            rpcRemote.getETypes.resultFormat="e4x";

            rpcRemote.getEvents.addEventListener(FaultEvent.FAULT, onRcpFault, false, 0, true);
            rpcRemote.getEvents.addEventListener(ResultEvent.RESULT, getEventsResult, false, 0, true);
            rpcRemote.getEvents.resultFormat="e4x";


        }
        
        public function onRcpFault(event:FaultEvent):void{
            var title:String = event.type + " (" + event.fault.faultCode + ")";
            var text:String = event.fault.faultString;
            setStatus(title +" : " + text, "red");
            resetTotalWait();
        }

        public function setStatus(text:String, color:String):void{
            trace("Set status: " + text + " (color:"+ color +")");
        }

        private function getETypesResult(event:ResultEvent):void{
            setStatus("OK: Event types received", "green");
            updateTotalWait(-1, "event types");
            var eTypeList:XMLList;

            eTypes = new Array(/*eTypeList.length*/);
            
            if(event.result.result.item)
                eTypeList = event.result.result.item;
            else
                return; 
            
            var i:int = 0;
            for(var s:String in eTypeList){
                var eType:EventLog = new EventLog(eTypeList[s],i++, this);
                eType.setShowUnit(_unit );
                eTypes.push(eType);
            }
            
            for( var c:String in eTypeCallback){
                eTypeCallback[c]();
            }

            eTypeCallback = new Array();

        }

        private function getEventLog(ename:String):EventLog{
            
            for(var i:String in eTypes){
                if(eTypes[i].ename == ename)
                    return eTypes[i];
            }

            return null;
        }

        private function getEventsResult(event:ResultEvent):void{
            
            var ename:String = event.result.name;
            var doUpdate:Boolean = false;
            eLogsWaitCount--;

	    if(event.result.interval)
		_interval = event.result.interval;

            doUpdate = updateTotalWait(-1, "event log");
            setStatus("OK: Eventlog reveived for " + ename + "(waiting" + eLogsWaitCount + ")", "green");
            
            var eLog:EventLog = getEventLog(ename);

            if(eLog){
                trace("parsing events for " + eLog.ename);
                eLog.putData(event.result.events.item, event.result.begin, event.result.end, event.result.interval);
                if(eLogsWaitCount <= 0){
                    for( var c:String in eLogsCallback)
                        eLogsCallback[c]();
                    eLogsCallback = new Array();
                }            }else{
                for(var i:String in eTypes){
                    eTypes[i].putData(event.result.events.item, event.result.begin, event.result.end, event.result.interval);
                }
            }

            if(doUpdate){
                for( var n:String in eLogsCallback)
                    eLogsCallback[n]();
                eLogsCallback = new Array();
            }
            

        }

        private function resetTotalWait():void{
            totalWaitCount = 0;
            if(totalWaitTime > totalWaitTimeMax)
                totalWaitTimeMax = totalWaitTime;
            totalWaitTime = 0;

            totalWaitReceivedCount = 0;
            if(progressBar)
                progressBar.visible=false;

            trace("progressBar reset " + totalWaitCount + " , " + totalWaitReceivedCount);
        }

        private function updateTotalWait(value:int, text:String):Boolean{

            trace("updateTotalWait " + totalWaitCount + " , " + totalWaitReceivedCount);
            totalWaitText = text;
            if(totalWaitReceivedCount < 0 ||  totalWaitCount < 0){
                resetTotalWait();
                return true;
            }

            if(value < 0)
                totalWaitReceivedCount -= value;
            else if (value > 0)
                totalWaitCount += value;

            if(totalWaitCount <= totalWaitReceivedCount){
                resetTotalWait();
                return true;
            }
            
            
            if(progressBar){
            //     if(totalWaitCount == 1)
//                      progressBar.indeterminate=true;
//                 else
                     progressBar.indeterminate=false;

                progressBar.visible=true;
                progressBar.label="receiving "+text+"..." ;
                progressBar.setProgress(totalWaitReceivedCount+1, totalWaitCount);
                trace("progressBar " + totalWaitCount + " , " + totalWaitReceivedCount);
            }
            
            return false;
        }

        private function timerHandler(event:TimerEvent):void {

            
            if(progressBar &&  totalWaitCount>0){
                totalWaitTime++;
                progressBar.indeterminate=false;

                progressBar.visible=true;
                progressBar.label="receiving "+totalWaitText+"..." ;
                progressBar.setProgress(/*totalWaitReceivedCount+1*/totalWaitTime%totalWaitTimeMax, /*totalWaitCount*/totalWaitTimeMax);
                trace("progressBar " + totalWaitCount + " , " + totalWaitReceivedCount);
            }
        }

        public function StartTimer():void {
            var myTimer:Timer = new Timer(1000, 0);
            myTimer.addEventListener("timer", timerHandler);
            myTimer.start();
        }
        


        public function getETypes(callback:Function):void{
            updateTotalWait(1, "Event Types");
            eTypeCallback.push(callback);  
            rpcRemote.getETypes(installation_id);
        }

        public function setUnit(unit:String):void{
            this.unit = unit.split(',');
//	     for(var i:String in eTypes){
//		 eTypes[i].setShowUnit( _unit );
//            }
        }

	public function set unit(unit:Array):void{
	    trace("set units:" + unit.join(','));
            this._unit = unit;
	     for(var i:String in eTypes){
		 eTypes[i].setShowUnit( _unit );
            }
        }

        public function getUnit():String{
            return this._unit.join(',');
            if(eTypes.length > 0)
                return eTypes[0].unit;

            return "-";
        }

        public function rmoDate(date:Date):Number{
            
            return date.time/1000;
            
        }

	public function setShow(list:String):void{
	    trace("set show filter: " + list);
	    this.showFilter = new EventFilter(list);
	    this.unit = this.showFilter.Units();
	    
	}

	public function getEventLogs():Array {
             
             var eTypesRet:Array = new Array();
             
             if(this.showFilter)
                 return this.showFilter.Filter(eTypes);

	     for(var i:String in eTypes){
		 if(eTypes[i].show)
		     eTypesRet.push(eTypes[i]);
	     }
	     return eTypesRet;

        }

	public function get interval ():Number{

	    return _interval;
	}
	

	private function getPosixDate(date:Date):Number {
	    return date.time/1000;
	}
	

        public function getEvents(timeBegin:Date, timeEnd:Date, callback:Function):void{
            setStatus("Wait: receiving event logs", "yellow");
//	    trace("rem unit get events from 
	    var begin:Number = getPosixDate(timeBegin);
	    var end:Number = getPosixDate(timeEnd);
    
            eLogsCallback.push(callback);  
            resetTotalWait();
            
            if(eTypes.length == 0)
                return;
            
            if(eTypes[0].eid == -1){
                updateTotalWait(eTypes.length, "event logs");
                eLogsWaitCount++
            
                for(var i:String in eTypes){
                    trace("ename:"+eTypes[i].ename);
                    eLogsWaitCount++;
                    eTypes[i].getData(begin, end);
                    //rpcRemote.getEvents(eTypes[i].ename, begin, end);
                
                }
                
            } else {      
                updateTotalWait(/*eTypes.length*/1, "event logs.");
		var enames:String = "";
		if(this.showFilter){
		    var earray:Array = this.showFilter.Filter(eTypes);
		    for(var a:String in earray){
			enames += earray[a].ename + ",";
		    }
		} else {
		    for(var e:String in eTypes){
			if(eTypes[e].show)
			    enames += eTypes[e].ename + ",";
		    }
		}
		trace("get enames " + enames);
                this.getEvent(enames, begin, end);
		eLogsWaitCount++;
            }
	}

	public function getEvent(ename:String, begin:Number, end:Number):void{
            rpcRemote.getEvents(installation_id, ename, begin, end);
            
        }

       


    }
}	