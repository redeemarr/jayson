jayson
======

Yet another C++ json library.
It consists of one header file and supposed to be lightweight, easy to integrate and use. Just add jayson.hpp to your project.

You can generate json data within your code like this:
```C++
	json::value number = 12.34;
	json::value string = "string";
	json::value array = { number, string };
	json::value object;
	object("key") = array;
```	
...then serialize it to string:
```C++
	std::string str = object.serialize();
```	
...or write it to a file:
```C++
	object.serialize("test.json");
```
...and then parse this file and read values:
```C++	
	json::value result;
	std::string errors;
	if (result.parse_file("test.json", errors))
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

Known limitations and pitfalls:
- weird things may happen due to multiple type-cast operators. be careful
- implicit cast from json::value to std::string using assignment operator won't work. workaround: explicitly cast like this:
```C++
	str = value.as<std::string>();
```
- intended to work with C++11 compilers only
