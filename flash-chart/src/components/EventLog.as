package components
{
    import mx.charts.series.LineSeries;
    import mx.charts.series.PlotSeries;
    import mx.charts.chartClasses.Series;
    import mx.graphics.Stroke;
    import components.RemoteUnit;
    import components.UnitAxis;
    import mx.formatters.DateFormatter;
    import mx.formatters.NumberFormatter;

    public class EventLog
    {

        private var colors:Array = new Array(  0xFFBF00, 0xFF7E00, 0xFF033E, 
                                               0x9966CC, 0xA4C639, 0x0000FF, 
                                               0x008000,  
                                               0x00FFFF, 0x4B5320, 0xDDA0DD,
                                               0xE9D66B, 0xFF9966, 0xFF355E,
                                               0x6D351A, 0x007AFF, 0x3FFF00,
                                               0xFF4F00, 0xE2062C, 0xC2B280  );
        
        public var ename:String;
        public var unit:String;
        public var hname:String;
        public var show:Boolean = true;

        public var mNo:int;

        public var enabled:Boolean = true;
        private var remoteUnit:RemoteUnit

        public var color:Number = new Number();
        //public var curveForm:String = "reverseStep";   
        public var curveForm:String = "line";

        public var eventArray:Array = new Array();

        public var showUnit:String;
	public var showUnitlist:XMLList;
        public var eid:int;
	public var interval:Number = 300;

	private var valueForm:NumberFormatter = new NumberFormatter();
	private var _precision:int = 1;
        private var showList:XML = 
           <root>
	     <unit name="cJ" >
                  <show unit="W" conv="toPrSec" curveform="reverseStep" />
	          <show unit="kW"conv="toPrSec" curveform="reverseStep" scale="0.001" />
                  <show unit="kWh" conv="accu" curveform="line" scale="1/3600000" />
                  <show unit="J" conv="accu" curveform="line" />
              </unit>
              <unit name="_J" >
                  <show unit="W" conv="toPrSec" curveform="reverseStep" />
	          <show unit="kW"conv="toPrSec" curveform="reverseStep" scale="0.001" />
                  <show unit="kWh" conv="accu" curveform="line" scale="1/3600000" />
                  <show unit="J" conv="accu" curveform="line" />
              </unit>
	      <unit name="_m³" >
	          <show unit="m³/h" conv="toPrSec" scale="3600" curveform="reverseStep"  />
	          <show unit="m³" conv="accu" curveform="line" />
		      <show unit="l/min" conv="toPrSec" scale="60000" curveform="reverseStep"  />
	          <show unit="l" conv="accu" scale="1000" curveform="line" />
              </unit>
 	      <unit name="l/min" >
                  <show unit="l/min" conv="none" />
                  <show unit="l/h" scale="60" />
	          <show unit="m³" conv="intg" scale="1/60000" />
                  <show unit="l" conv="intg" scale="1/60"  />
              </unit>
	      <unit name="W" >
                  <show unit="W" curveform="reverseStep" />
	          <show unit="kW" curveform="reverseStep" scale="1/1000" />
                  <show unit="kWh" conv="intg" scale="1/3600000" curveform="line"/>
	      </unit>
	      <unit name="$!" >
	         <show unit=""   conv="event" curveform="plot" precision="0"/>
              </unit>
              <unit name="!$" >
	         <show unit=""   conv="event" curveform="plot" precision="0"/>
              </unit>
              <unit name="$." >
	         <show unit=""  conv="event" curveform="plot" precision="0"/>
              </unit>
              <unit name=".$" >
	         <show unit=""  conv="event" curveform="plot" precision="0"/>
              </unit>
              <unit name="e" >
	         <show unit=""  conv="event" curveform="plot" precision="0" />
              </unit>
	  </root>

	private var showListRep:XML = 
           <root>
              <unit name="_*" >
   	        <show unit=",/s" conv="toPrSec" curveform="reverseStep" />
   	        <show unit="k,/h" conv="toPrSec" curveform="reverseStep" scale="3.6"/>
                <show unit="," conv="accu"  curveform="line" />
                <show unit="k," conv="accu" scale="1/1000"  curveform="line" />
              </unit>
              <unit name="/*" >
	         <show unit=","  curveform="step"  precision="0" />
              </unit>
              <unit name="$*" >
	         <show unit=""  curveform="plot"  precision="0" />
              </unit>
	   </root>


        public function EventLog(xml:XML, mNo:int, remoteUnit:RemoteUnit)
        {
            ename = new String(xml.@name);
            unit  = new String(xml.@unit);
            hname = new String(xml.@description);
            if(xml.@eid)
                eid   = xml.@eid;

            if(xml.@type){
                if(xml.@type == "hide")
                    this.show = false;
            }

	    if(unit==""||unit==null)
		unit = ".$";

            if(eid <= 0)
                eid = -1;
            
            this.mNo = mNo;
            this.color = colors[this.mNo];
            this.remoteUnit = remoteUnit;
            
            if(this.unit == "l")
                curveForm = "line";

	    valueForm.precision = this._precision;
	    valueForm.decimalSeparatorTo = ",";
	    valueForm.thousandsSeparatorTo=".";
	    valueForm.useThousandsSeparator=true;
	    valueForm.useNegativeSign=true;
	    
            this.showUnit = getShowUnit();

	    trace("EventLog '"+ename+"' '"+unit+"'-->'"+this.showUnit+"''");
	    
        }

	public function get precision():int{
	    return this._precision;
	} 

	public function set precision(prec:int):void{
	    this._precision = prec;
	    valueForm.precision = prec;
	} 

        public function get name():String{
            if(hname == "NULL" || hname == "")
                return ename;
            else {
                if(unit&&(unit!="e")&&(showUnit!=""))
                    return hname + " [" + showUnit +"]";
                else
                    return hname;

            }
        }


	public function get iid():String{
	    return remoteUnit.getInstallationId();
	}


	private function getUnitlist():XMLList{
	    for(var u:String in showList.unit){
                if(showList.unit[u].@name == this.unit){
		    this.showUnitlist = showList.unit[u].show;
                    return showList.unit[u].show;
                }
            }

	    for(var i:String in showListRep.unit){
		var xmlunit:String = showListRep.unit[i].@name;

		var slen:int = xmlunit.indexOf('*');
		if(slen == -1)
		    slen = xmlunit.length;
		var sunit:String =  xmlunit.substr(0, slen);
		// trace("try unit " + showList.unit[i].@name + "('" + sunit + "' len " + slen+ ")") ;
		
                if(this.unit.search(sunit)==0){
		    var unitlist:XMLList = new XMLList(showListRep.unit[i].show);
		    var runit:String =  unit.substr(slen);		    
		    for(var n:String in unitlist){
			var tempstr:String = unitlist[n].@unit;
			tempstr = tempstr.replace(",", runit);
			//	trace("replace '" + tempstr + "' '" + unitlist[n].@unit+"'");
			unitlist[n].@unit = tempstr;
		    }
		    
		    this.showUnitlist = unitlist;
                    return unitlist;
                }
            }
	    

	    return null;
	}


        public function setShowUnit(list:Array):Boolean{
            var unitlist:XMLList =  getUnitlist();
	  
            if(unitlist == null){
                return false;
	    }

	    for(var l:String in list){
		for(var i:String in unitlist){
		    //  trace("showunit " +   unitlist[i].@unit +" courve"+ unitlist[i].@curveform);
                    if(unitlist[i].@unit == list[l]){
                        this.showUnit = unitlist[i].@unit;
			if(unitlist[i].@curveform)
			    this.curveForm =  unitlist[i].@curveform;
                        return true;
                    }
                }
            }

            return false;
        }

        private function getShowUnit():String{
            
	    var unitlist:XMLList = getUnitlist();

	    if(unitlist){
		if(unitlist[0].@curveform)
		    this.curveForm =  unitlist[0].@curveform;
		return unitlist[0].@unit;
	    }

	    return this.unit;

        }

            

//////////////////   
//     A return value of -1 indicates that the first parameter, a, precedes the second parameter, b.
//     A return value of 1 indicates that the second parameter, b, precedes the first, a.
//     A return value of 0 indicates that the elements have equal sorting precedence.   
        private function timeOrder(a:Object, b:Object):Number{
            if(a.time < b.time)
                return -1;
            if(a.time > b.time)
                return 1;
            return 0;



        }
        
        private function removeTime(begin:Number, end:Number):void{
            var newArr:Array = new Array();
            
            for(var i:String in eventArray){
                var time:Number = new Number(eventArray[i].time);
                if(time <= begin || time >=end)
                    newArr.push({time:eventArray[i].time, val:eventArray[i].val});
            }

            eventArray = newArr;
        }


        public function putData(eventList:XMLList,begin:Number, end:Number , interval_:Number ):void{

            //  removeTime(begin, end);
            
            this.eventArray = new Array();
	    this.interval   = interval_;

            for(var i:String in eventList){
                var time:Number = eventList[i].@time;
//                var val:Number = eventList[i].@value;
                var val:String = eventList[i].@value;
                if(eid > 0){
                    if(eventList[i].@eid == eid)
                        this.eventArray.push({time:time, val:val});
                } else {                    
                    this.eventArray.push({time:time, val:val});
                }
                
            }


            this.eventArray.sort(timeOrder);

            trace("event log received length "+ this.eventArray.length);
        }

        private function convertToPrSec(eLog:Array, scale:Number = 1):Array{
            
            var convLog:Array = new Array();

            if(eLog.length < 2)
                return convLog;
            
            var i:int = 0;
            var pTime:Number = eLog[i].time;
            var pVal:Number  = /*eLog[i].val*/0;
            
            convLog.push({time:pTime, val:pVal});

            while(i+1 < eLog.length){
                i++;
                if(eLog[i].val == 0)
                    continue;

                var time:Number = eLog[i].time;
                var diff:Number = time -  pTime ;
                var val:Number = (eLog[i].val/diff)*scale;
                // trace("diff: "+ diff + ", watt " + val + " (J:"+ eLog[i].val+")");
                convLog.push({time:time, val:val});
                
                pTime = time;
                pVal = val;
                
            }

            if(eLog[i].val == 0){
                var etime:Number = eLog[i].time;
                var ediff:Number = time -  pTime ;
                var eVal:Number =  (eLog[i-1].val/diff)*scale;
                convLog.push({time:eLog[i].time, val:eVal});
            }

            if(convLog.length > 1){
                convLog[0].val = convLog[1].val;
            }

            return convLog;
            
        }

	private function convertScale(eLog:Array, scale:Number = 1):Array {
	    var convLog:Array = new Array();
	    var i:int = 0;

	    while(i < eLog.length){
                var time:Number = eLog[i].time;
		var scaleVal:Number = 0;		
                scaleVal = eLog[i].val*scale;                
                convLog.push({time:time, val: scaleVal});                
		i++;
            }
	    
	     return convLog;
	}


        private function convertAccumu( eLog:Array, scale:Number = 1):Array {
            var convLog:Array = new Array();

            if(eLog.length < 2)
                return convLog;
            
            var accuVal:Number = 0;
            var i:int = 0;
            var pTime:Number = eLog[i].time;
           
            convLog.push({time:pTime, val:accuVal});

            while(i+1 < eLog.length){
                i++;
                if(eLog[i].val == 0)
                    continue;

                var time:Number = eLog[i].time;
                accuVal += eLog[i].val*scale;                
                convLog.push({time:time, val:accuVal});                
            }
            
            if(eLog[i].val == 0){
                convLog.push({time:eLog[i].time, val:accuVal});
            }

            return convLog;
        }
    
	private function Integration( eLog:Array, scale:Number = 1):Array{
	    var convLog:Array = new Array();
	    trace("Integration scale " + scale);
	    if(eLog.length < 2)
                return convLog;

	    var i:int = 0;
	    var accuVal:Number = 0;
	    var pTime:Number = eLog[i].time;
	    convLog.push({time:pTime, val:accuVal});
	    
	    while(i+1 < eLog.length){
		 i++;
		 var time:Number = eLog[i].time;
		 var diff:Number = (eLog[i].time - pTime);
		 accuVal += eLog[i].val*scale*diff; 
		 convLog.push({time:time, val:accuVal});    
		 pTime = time;
	    }
	    
	    if(eLog[i].val == 0){
                convLog.push({time:eLog[i].time, val:accuVal});
            }

            return convLog;

	}
	

        private function convertFromEvent(to:String, eLog:Array):Array{
            var convLog:Array = new Array();
            var i:int = 0;
            while(i < eLog.length){
                var time:Number = eLog[i].time;
                var val:Number = 5;
                var text:String = valueForm.format(eLog[i].val);
                convLog.push({time:time, val:val, text:text});                
                i++;
            }

            return convLog;
        }

        private function convertUnit(from:String, to:String, eLog:Array):Array{

	    trace("conv from " + from + " to " + to + this.showUnitlist);
	     for(var n:String in this.showUnitlist){
//		 trace("index " + n + " " + this.showUnitlist[n].@unit );
		 if(this.showUnitlist[n].@unit == to){
		     var conv:String = this.showUnitlist[n].@conv;
		     var curveform:String =  this.showUnitlist[n].@curveform;
		     var scaleequa:String = this.showUnitlist[n].@scale;
		     var scale:Number = 1;

		     if(this.showUnitlist[n].@precision)
			 this.precision = this.showUnitlist[n].@precision;

		     if(scaleequa){
			 var scalearr:Array = scaleequa.split('/');
			 scale = scalearr[0];
			 if(scalearr[1])
			     scale /= scalearr[1];
		     }
		     trace("conv='"+conv+"' curveform='" + curveform + "' scale=" + scale )
		     
		     if(curveform)
			 curveForm = curveform; 

		     switch(conv){
		       case 'toPrSec':
			 return convertToPrSec(eLog,scale);
		       case 'accu':
			 return convertAccumu(eLog,scale);
		       case 'intg':
			  return Integration(eLog,scale);
		       case 'event':
			 curveForm = "plot";
			 return convertFromEvent(to, eLog);
		       default:
			 if(scale != 1)
			     return convertScale(eLog,scale);
			 else 
			     return eLog;
			 break;
		     }
		     break;
		 }
	     }

	     return eLog;


        }


        public function getLastTime(begin:Number):Number{
            var lastTime:Number = begin;
            
            for(var i:String in eventArray){
                if(this.eventArray[i].time > lastTime)
                    lastTime = this.eventArray[i].time;
            }

            return lastTime;

        }

   //      public function getFirstTime(end:Number):Number{
//             var lastTime:Number = end;
            
//             for(var i:String in eventArray){
//                 if(this.eventArray[i].time > lastTime)
//                     lastTime = this.eventArray[i].time;
//             }

//             return lastTime;
            
//         }


        public function getData(begin:Number, end:Number):void{
            remoteUnit.getEvent(ename, begin, end);
        }

        public function getLineSeries(unitAxis:UnitAxis):Series {

            var showUnit:String = unitAxis.unit;
	    trace("this.curveForm " +  this.curveForm);
            if( this.curveForm == "plot" || this.eventArray.length <= 1){
		trace("plotting");
                var plot:PlotSeries = new PlotSeries();
                plot.yField = "val";
                plot.xField = "time";
                plot.displayName = this.name;
                plot.verticalAxis = unitAxis.axis;
                plot.setStyle("stroke", new Stroke(this.color, 2, 2));
                plot.setStyle("lineStroke", new Stroke(this.color, 2, 2));
                plot.setStyle("fill", this.color);
                plot.setStyle("radius", 5);
                plot.dataProvider =  convertUnit(this.unit, showUnit, this.eventArray);
                
                return plot;
            } 
            var line:LineSeries = new LineSeries();
            line.yField = "val";
            line.xField = "time";
            line.displayName = this.name;
            line.verticalAxis = unitAxis.axis;
            line.setStyle("stroke", new Stroke(this.color, 2, 2));
            line.setStyle("lineStroke", new Stroke(this.color, 2, 2));
            
            if(this.enabled)
                line.dataProvider =  convertUnit(this.unit, showUnit, this.eventArray);
            
            line.setStyle("form", this.curveForm);
            
            return  line;
        }

    }
}