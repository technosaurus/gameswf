// Circle class defined in external file named Circle.as
class circle extends shape
{
	var i : Number;
	function circle(param)
	{
		super(param);
		i = param;
		trace("circle constructor: " + param);
	}
	function drawshape()
	{
		trace("drawshape from circle is called: i=" + i);
//		super.drawshape();
	}
	function drawcircle()
	{
		drawsbox();
		trace("drawcircle is called: i=" + i);
	}
}
