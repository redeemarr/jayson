#include <iostream>
#include "jayson.h"
#include "tests.h"

int main(int argc, const char * argv[])
{
//	json::tests::run();

	// You can generate json data within code like this:
	json::value number = 12.34;
	json::value string = "string";
	json::value array = { number, string };
	
	// ...then convert it to string:
	std::string str = json::to_string(array);
	
	// ...or write to file:
	json::to_file("test.json", array);

	// ...and then parse this file and read values:
	json::value result;
	std::string errors;
	if (json::from_file("test.json", result, &errors))
	{
		double number = result.get(0);
		std::string string = result.get(1);
		std::cout << number << ", " << string << std::endl;
	}
	else
	{
		std::cout << errors << std::endl;
	}
	return 0;
}
