#include <iostream>
#include "jayson.hpp"
#include "tests.hpp"

int main(int argc, const char* argv[])
{
//	json::tests::run(); return 0;

	// You can generate json data within your code like this:
	json::value number = 12.34;
	json::value string = "text";
	json::value array = { 123, number, string };
	json::value object;
	object("key") = array;
	
	// ...then serialize it to string:
	std::string str = object.to_string();
	
	// ...or write to json/bson file:
	object.to_json_file("test.json");
	object.to_bson_file("test.bson");

	// ...and then parse this file and read values:
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

	return 0;
}
