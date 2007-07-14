// Shape class defined in external file named Shape.as
class shape
{
	var i : String;
	var j : Number;
	
	function shape(param)
	{
		trace("shape constructor:" + param);
		i = param;
		j = param == "one" ? 1 : 2;
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
