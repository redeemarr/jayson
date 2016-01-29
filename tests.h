#pragma once

#define TEST_LEAKS 0

namespace json
{

class tests
{
public:

	static void run();
	
private:

	void* operator new(size_t size);
	void operator delete(void* ptr) noexcept;
};

}
