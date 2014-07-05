#include "jayson.h"
#include <iostream>

void tutorial()
{
	json::object_t obj;
	obj["bool"] = true;
	obj["number_1"] = 1337;
	obj["number_2"] = 1.5;
	obj["string_1"] = "foo";
	
	json::array_t arr;
	arr.push_back(1);
	arr.push_back(12.34);
	arr.push_back("bar");
	obj["array"] = arr;
	
	std::cout << json::to_string(obj) << std::endl;
	
	json::to_file("test.json", obj);
	
	json::value val;
	if (json::from_file("test.json", val))
	{
		std::cout << json::to_string(val) << std::endl;
	}
}

int main(int argc, const char * argv[])
{
	tutorial();
	return 0;
}
