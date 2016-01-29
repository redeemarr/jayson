jayson
======

Yet another C++ json library.
It consists of one header file and supposed to be lightweight, easy to integrate and use.

You can generate json data within code like this:
```C++
	json::value number = 12.34;
	json::value string = "string";
	json::value array = { number, string };
```	
...then convert it to string:
```C++
	std::string str = json::to_string(array);
```	
...or write to file:
```C++
	json::to_file("test.json", array);
```
...and then parse this file and read values:
```C++	
	json::value result;
	std::string errors;
	if (json::from_file("test.json", result, &errors))
	{
		double number = result.get(0);
		std::string string = result.get(1);
	}
	else
	{
		std::cout << errors << std::endl;
	}
```
