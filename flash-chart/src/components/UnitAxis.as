package components
{
    import mx.charts.AxisRenderer;
    import mx.charts.LinearAxis;
    
    public class UnitAxis extends AxisRenderer
    {
        public var unit:String;
	private var lAxis:LinearAxis = new LinearAxis();

        public function UnitAxis(setup:String, title:String = null):void{
            super();

            lAxis = new LinearAxis();

	    var parts:Array = setup.split(':');
	    
	    this.unit = parts[0];

	    if(this.unit == ""){
		lAxis.maximum = 50;
		lAxis.minimum = -0.5;
	    }

	    if(parts[1]){
		var maxmin:Array = parts[1].split(',');
		lAxis.maximum = maxmin[0];
		lAxis.minimum = maxmin[1];
	    } 

	    if(parts[2]){
		var decimals:Number = parts[2];
	    }

            if(title)
                lAxis.title = title;
            else
                lAxis.title = unit;
	    
	    lAxis.displayName = lAxis.title;

            this.placement = "left";
            this.axis = lAxis;
            this.unit = unit;
	    lAxis.baseAtZero = true;
	    lAxis.autoAdjust = true;

        }

	public function set minimum(min:Number):void{
	    this.lAxis.minimum = min;
	}

	public function get minimum():Number{
	    return this.lAxis.minimum;
	}

	public function set maximum(max:Number):void{
	    this.lAxis.maximum = max;
	}

	public function get maximum():Number{
	    return this.lAxis.maximum;
	}
        
    }
}