// Shape class defined in external file named Shape.as
class shape
{
	var i : Number;
	var j : Number;
	
	function shape(param)
	{
		trace("shape constructor:" + param);
		i = 2;
		j = param;
	}
	
	function drawshape()
	{
		trace("drawshape is called: i=" + i);
	}

	function drawsbox()
	{
		trace("drawsbox is called: i=" + j);
	}
	
}
