#include <iostream>
#include "jayson.hpp"
#include "tests.hpp"

int main(int argc, const char* argv[])
{
//	json::tests::run(); return 0;

	// You can generate json data within your code like this:
	json::value number = 12.34;
	json::value string = "string";
	json::value array = { number, string };
	json::value object;
	object("key") = array;
	
	// ...then serialize it to string:
	std::string str = object.serialize();
	
	// ...or write to file:
	object.serialize("test.json");

	// ...and then parse this file and read values:
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

	return 0;
}
