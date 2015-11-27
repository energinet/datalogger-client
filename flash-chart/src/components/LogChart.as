package components
{
    import mx.charts.AreaChart;
    import mx.charts.DateTimeAxis;
    import mx.charts.chartClasses.Series;
    import mx.charts.series.LineSeries;
    import components.UnitAxis;
    import mx.formatters.DateFormatter;
    import mx.formatters.NumberFormatter;
    import mx.charts.HitData;
    import mx.charts.AxisRenderer;
    import mx.charts.GridLines;

    public class LogChart extends AreaChart
    {

        private var vaxis:UnitAxis;
        private var timeAxis:DateTimeAxis;
        private var axisList:Array = new Array();
        private var axisShow:Array = new Array();
	private var dateForm:DateFormatter = new DateFormatter();
	private var labelTimeForm:DateFormatter = new DateFormatter();
	private var valueForm:NumberFormatter = new NumberFormatter();

        public function LogChart(){
            super();

            timeAxis = new DateTimeAxis();
            
            timeAxis.dataUnits="hours";
            timeAxis.parseFunction=parseTime;
            timeAxis.displayLocalTime = true;
	    timeAxis.disabledDays = [];//Bugfix
	    timeAxis.labelFunction = formatTime; 

	    this.dataTipFunction = labelText;


            this.horizontalAxis = timeAxis;
            
	    vaxis = new UnitAxis("");
	    axisShow  = [ vaxis ];
            this.verticalAxisRenderers = axisShow;


	    dateForm.formatString = "J:NN";
	    labelTimeForm.formatString = "J:NN";

	    valueForm.precision = 2;
	    valueForm.decimalSeparatorTo = ",";
	    valueForm.thousandsSeparatorTo=".";
	    valueForm.useThousandsSeparator=true;
	    valueForm.useNegativeSign=true;


        }

	public function set precision(precision_:Number):void{
	    valueForm.precision = precision_;
	}
        
        public function parseTime(s:String):Date{
            var pxDate:Number = new Number(s);
//            var utcDate:Date = new Date((pxDate)*1000);

            return new Date((pxDate)*1000);
        }

	
	public function formatTime(d:Date, previousValue:Date, axis:DateTimeAxis):String{

	    return dateForm.format(d); 

	}

        
	private function showUnitAxis(axis:UnitAxis):void{
	    
	    for(var m:String in axisShow){
                if(axisShow[m] == axis)
                    return;
            }

	    axisShow.push(axis);
	}

        private function getUnitAxis(unit:String):UnitAxis{
            
            for(var m:String in axisList){
                if(axisList[m].unit == unit)
                    return axisList[m];
            }

            return addUnitAxis(unit);
        }

        private function addUnitAxis(unit:String):UnitAxis{
            var newAxis:UnitAxis =  new UnitAxis(unit);

            axisList.push(newAxis);
	    trace("add newAxis unit:" + unit +" axis count:"+  axisList.length);
            this.verticalAxisRenderers = axisList;

            return newAxis;
        }

        public function setTimeAxis(start:Date , end:Date, setting:String = "day"):void{
            timeAxis.minimum=start;
            timeAxis.maximum=end;
	    
	    switch(setting){
	      default:
	      case "day":
		timeAxis.dataUnits="hours";
  	        dateForm.formatString = "J:NN";
		labelTimeForm.formatString = "J:NN";
		break;
	      case "week":
		timeAxis.dataUnits="days";
		dateForm.formatString = "D.MM";
		labelTimeForm.formatString = "J:NN";
		break;
	      case "month":
		timeAxis.dataUnits="weeks";
		dateForm.formatString = "D.MM";
		labelTimeForm.formatString = "D.MM J:NN";
		break;
	      case "year":
		timeAxis.dataUnits="months";
		dateForm.formatString = "M.YYYY";
		labelTimeForm.formatString = "D.MM";
		break;
	    }
	    
	    timeAxis.direction = "normal";
	    
        }

	public function setUnitAxis(setup:String):void {

	    var axisArray:Array = setup.split(';');

	    for(var i:String in axisArray){
		addUnitAxis(axisArray[i]);
	    }
	    
	}


        public function setSeries(eLogs:Array, unit:String = "W"):void{
            var lineArray:Array = new Array();
            axisShow = new Array();

            for(var m:String in eLogs){
                if(eLogs[m].enabled){
                    var unitAxis:UnitAxis = getUnitAxis(eLogs[m].showUnit);
		    if(eLogs[m].showUnit!=""){
			showUnitAxis(unitAxis);
		    }
		    
		    var line:Series = eLogs[m].getLineSeries(unitAxis);
		      
                    lineArray.push(line);
                }
            }

//	    trace("axisShow count " + this.axisShow.length);
	    
	    if(this.axisShow.length > 0)
		this.verticalAxisRenderers = axisShow;

            this.series =  lineArray;          

	    if(this.backgroundElements[0]){
		this.backgroundElements[0].setStyle("gridDirection", "horizontal");
		this.backgroundElements[0].setStyle("horizontalShowOrigin", "false");
		this.backgroundElements[0].setStyle("verticalShowOrigin", "false");
	    }
        }
        	
     public function labelText(hd:HitData):String {
           var pxDate:Number = new Number(hd.item.time);
           var utcDate:Date = new Date((pxDate)*1000);

         var text:String;
         text = "<b>" + Series(hd.element).displayName + "</b><br>";
         text += labelTimeForm.format(utcDate)+ " : ";
         if(hd.item.text)
             text += hd.item.text;
         else {
             text += valueForm.format(hd.item.val);
	     
         }

         return text;
     }

	

        
    }
}
