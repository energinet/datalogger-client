package components
{
//    import mx.collections.ArrayCollection;
    import mx.charts.LinearAxis;
//    import mx.charts.chartClasses.Series;
//    import mx.charts.series.LineSeries;
    
    public class UnitAxis extends LinearAxis
    {
        public function UnitAxis(unit:String, title:String = null):void{
            super();
            if(title)
                this.title = title;
            else
                this.title = unit;
        }
        
    }
}