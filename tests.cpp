#include <iostream>
#include "tests.h"

#if (TEST_LEAKS)

static long g_allocs = 0;
static long g_total_allocs = 0;

void* operator new(size_t size)
{
	++g_allocs;
	++g_total_allocs;
	return malloc(size);
}

void operator delete(void* ptr) noexcept
{
	--g_allocs;
	free(ptr);
}

#endif

#include "jayson.h"

namespace json
{

void print_padded(std::string const& name, int width)
{
	int spaces = width - (int)name.length();
	if (spaces < 0) spaces = 0;
	std::cout << name;
	for (int i=0; i<spaces; ++i) std::cout << ' ';
}

template <typename T> void compare_check(T const& a, T const& b)
{
	std::ostringstream oss;
	oss << a;
	std::string sa = oss.str();
	oss.str("");
	oss << b;
	std::string sb = oss.str();
	if (sa == sb)
	{
		std::cout << "ok";
	}
	else
	{
		std::cout << "failed: " << sa << " / " << sb;
	}
}

#define CONSTRUCT(TYPE, VALUE, FIELD) \
{ \
	TYPE var = VALUE; \
	json::value val(var); \
	print_padded(#TYPE, 20); \
	compare_check<TYPE>(var, val.data.FIELD); \
	std::cout << std::endl; \
}

#define ASSIGN(TYPE, VALUE, FIELD) \
{ \
	TYPE var = VALUE; \
	json::value val; \
	val = var; \
	print_padded(#TYPE, 20); \
	compare_check<TYPE>(var, val.data.FIELD); \
	std::cout << std::endl; \
}

#define TYPE_CAST(TYPE, VALUE, FIELD) \
{ \
	TYPE var = VALUE; \
	json::value val(var); \
	TYPE var2 = val; \
	print_padded(#TYPE, 20); \
	compare_check<TYPE>(var, var2); \
	std::cout << std::endl; \
}

void tests::run()
{
	{
	std::cout << "**** constructors ****\n";
	CONSTRUCT(short,              -123,     n);
	CONSTRUCT(unsigned short,      123,     n);
	CONSTRUCT(int,                -123,     n);
	CONSTRUCT(unsigned int,        123,     n);
	CONSTRUCT(long,               -123,     n);
	CONSTRUCT(unsigned long,       123,     n);
	CONSTRUCT(long long,          -123,     n);
	CONSTRUCT(unsigned long long,  123,     n);
	CONSTRUCT(float,              -123.456, n);
	CONSTRUCT(double,             -123.456, n);
	CONSTRUCT(char const*,        "text",   s);
	CONSTRUCT(std::string,        "text",   s);
	CONSTRUCT(bool,               true,     b);
	
	std::cout << "\n***** assignment *****\n";
	ASSIGN(short,              -123,     n);
	ASSIGN(unsigned short,      123,     n);
	ASSIGN(int,                -123,     n);
	ASSIGN(unsigned int,        123,     n);
	ASSIGN(long,               -123,     n);
	ASSIGN(unsigned long,       123,     n);
	ASSIGN(long long,          -123,     n);
	ASSIGN(unsigned long long,  123,     n);
	ASSIGN(float,              -123.456, n);
	ASSIGN(double,             -123.456, n);
	ASSIGN(char const*,        "text",   s);
	ASSIGN(std::string,        "text",   s);
	ASSIGN(bool,               true,     b);
	
	std::cout << "\n****** type cast *****\n";
	TYPE_CAST(short,              -123,     n);
	TYPE_CAST(unsigned short,      123,     n);
	TYPE_CAST(int,                -123,     n);
	TYPE_CAST(unsigned int,        123,     n);
	TYPE_CAST(long,               -123,     n);
	TYPE_CAST(unsigned long,       123,     n);
	TYPE_CAST(long long,          -123,     n);
	TYPE_CAST(unsigned long long,  123,     n);
	TYPE_CAST(float,              -123.456, n);
	TYPE_CAST(double,             -123.456, n);
	TYPE_CAST(char const*,        "text",   s);
	TYPE_CAST(std::string,        "text",   s);
	TYPE_CAST(bool,               true,     b);
	
	std::cout << "\n******** array *******\n";
	json::value arr;
	arr.add(123);
	arr.add("abc");
	arr.add(true);
	
	std::cout << "0: " << arr.get(0).data.n << "\n";
	std::cout << "1: " << arr.get(1).data.s << "\n";
	std::cout << "2: " << arr.get(2).data.b << "\n";
	
	std::cout << "\n******* object *******\n";
	json::value obj;
	obj["a"] = 123;
	obj["b"] = "abc";
	obj["c"] = true;
	
	std::cout << "a: " << obj["a"].data.n << "\n";
	std::cout << "b: " << obj["b"].data.s << "\n";
	std::cout << "c: " << obj["c"].data.b << "\n";
	}

#if (TEST_LEAKS)
	std::cout << "\n******* memory *******\n";
	if (g_allocs > 0) std::cout << g_allocs << " leaked objects";
	else std::cout << "no leaks";
	std::cout << ", " << g_total_allocs << " total allocations\n";
#endif
}

}
