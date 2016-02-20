jayson
======

Yet another C++ json library.
It consists of one header file and supposed to be lightweight, easy to integrate and use.

You can generate json data within your code like this:
```C++
	json::value number = 12.34;
	json::value string = "string";
	json::value array = { number, string };
	json::value object;
	object("key") = array;
```	
...then convert it to string:
```C++
	std::string str = json::to_string(object);
```	
...or write to file:
```C++
	json::to_file("test.json", object);
```
...and then parse this file and read values:
```C++	
	json::value result;
	std::string errors;
	if (json::from_file("test.json", result, &errors))
	{
		auto const& array = result("key");
		double number = array[0];
		std::string string = array[1];
		std::cout << number << ", " << string << std::endl;
	}
	else
	{
		std::cout << errors << std::endl;
	}
```

Also included MSVC natvis file for convenient debugging.
This file should be placed in '/Documents/Visual Studio 2013/Visualizers'. After that you'll be able to see nice looking json values in debugger auto/watch panel.

Known limitations and pitfalls:
- weird things may happen due to multiple type-cast operators. be careful
- implicit cast from json::value to std::string doesn't work with MSVC compiler. workaround: explicit cast to 'char const*' first
- highly unoptimized (yet) object/array access
- intended to work with C++14 compilers only
