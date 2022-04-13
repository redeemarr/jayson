jayson
======

Yet another C++ json library.
It consists of one header file and supposed to be fast, lightweight, easy to integrate and use. Just add jayson.hpp to your project and include it.

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
	std::string str = object.to_string();
```	
...or write to json/bson file:
```C++
	object.to_json_file("test.json");
	object.to_bson_file("test.bson");
```
...and then parse this file and read values:
```C++	
	json::value result;
	std::string errors;
	if (result.from_json_file("test.json", &errors))
	{
		auto const& array   = result("key");
		int         n_int   = array[0];
		double      n_float = array[1];
		std::string string  = array[2];
		std::cout << n_int << ", " << n_float << ", " << string << std::endl;
	}
	else
	{
		std::cout << errors << std::endl;
	}
```

Known limitations and pitfalls:
- some exotic bson types are not supported
- implicit cast from json::value to std::string using assignment operator won't work. workaround: explicitly cast like this:
```C++
	str = value.as<std::string>();
```
- intended to work with C++11 compilers only
