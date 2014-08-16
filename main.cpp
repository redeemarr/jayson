#include <iostream>
#include "jayson.h"

int main(int argc, const char * argv[])
{
	json::object_t obj;
	obj["bool"] = true;
	obj["number_1"] = 1337;
	obj["number_2"] = 1.5f;
	obj["string_1"] = "foo";
	
	json::array_t arr;
	arr.push_back(1);
	arr.push_back(12.34f);
	arr.push_back("bar");
	obj["array"] = arr;
	obj["object"] = obj;
	
	std::cout << json::to_string(obj) << std::endl;
	
	json::to_file("test.json", obj);
	
	json::value val;
	if (json::from_file("test.json", val))
	{
		std::cout << json::to_string(val) << std::endl;
	}
	
	return 0;
}
