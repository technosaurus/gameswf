// Shape class defined in external file named Shape.as
class shape
{
	var i : Number;
	var j : Number;
	
	function shape(param)
	{
		trace("shape constructor:" + param);
		i = param-100;
		j = param-100;
	}
	
	function drawshape()
	{
		trace("drawshape is called: i=" + i);
		drawbox();
	}

	function drawbox()
	{
		trace("drawbox is called: i=" + i + ", j=" + j);
	}
	
}
