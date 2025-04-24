x = y = 1;
{
	x = 2; 
	a = 3;
	function f (z) {
		x = 4;
		y = 6;
		{
			local z = 7;
			function () { return y; }
		}
	}
}