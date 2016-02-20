#include <iostream>
#include "jayson.h"
#include "tests.h"

int main(int argc, const char * argv[])
{
//	json::tests::run(); return 0;

	// You can generate json data within your code like this:
	json::value number = 12.34;
	json::value string = "string";
	json::value array = { number, string };
	json::value object;
	object("key") = array;
	
	// ...then convert it to string:
	std::string str = json::to_string(object);
	
	// ...or write to file:
	json::to_file("test.json", object);

	// ...and then parse this file and read values:
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
	return 0;
}
