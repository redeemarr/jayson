#include <sstream>
#include <iostream>
#include "tests.hpp"

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

#include "jayson.hpp"

namespace json
{

void print_padded(std::string const& name, int width)
{
	int spaces = width - (int)name.length();
	if (spaces < 0) spaces = 0;
	std::cout << name;
	for (int i=0; i<spaces; ++i) std::cout << ' ';
}

template <typename T, typename U> void compare_check(T const& a, U const& b)
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

// std::stream workaround
template <typename U> void compare_check(char a, U const& b)          { compare_check(a + 0, b); }
template <typename U> void compare_check(unsigned char a, U const& b) { compare_check(a + 0, b); }

void compare_check(char const* a, std::string* b)
{
	return compare_check(a, *b);
}

void compare_check(std::string const& a, std::string* b)
{
	return compare_check(a, *b);
}

void tests::run()
{
	std::cout << "sizeof(json::value) = " << sizeof(json::value) << " bytes" << std::endl << std::endl;

	std::cout << "**** constructors ****\n";
#define CONSTRUCT(TYPE, VALUE, FIELD) \
{ \
	TYPE var = VALUE; \
	json::value val(var); \
	print_padded(#TYPE, 20); \
	compare_check(var, val.data.FIELD); \
	std::cout << " -> " << json::type_string(val.type) << std::endl; \
}
	CONSTRUCT(char,               -123,     i);
	CONSTRUCT(unsigned char,       123,     i);
	CONSTRUCT(short,              -123,     i);
	CONSTRUCT(unsigned short,      123,     i);
	CONSTRUCT(int,                -123,     i);
	CONSTRUCT(unsigned int,        123,     u);
	CONSTRUCT(long,               -123,     l);
	CONSTRUCT(unsigned long,       123,     u);
	CONSTRUCT(long long,          -123,     l);
	CONSTRUCT(unsigned long long,  123,     u);
	CONSTRUCT(float,              -123.456, d);
	CONSTRUCT(double,             -123.456, d);
	CONSTRUCT(char const*,        "text",   s);
	CONSTRUCT(std::string,        "text",   s);
	CONSTRUCT(bool,               true,     b);
	
	std::cout << "\n***** assignment *****\n";
#define ASSIGN(TYPE, VALUE, FIELD) \
{ \
	TYPE var = VALUE; \
	json::value val; \
	val = var; \
	print_padded(#TYPE, 20); \
	compare_check(var, val.data.FIELD); \
	std::cout << std::endl; \
}
	ASSIGN(short,              -123,     i);
	ASSIGN(unsigned short,      123,     i);
	ASSIGN(int,                -123,     i);
	ASSIGN(unsigned int,        123,     u);
	ASSIGN(long,               -123,     l);
	ASSIGN(unsigned long,       123,     u);
	ASSIGN(long long,          -123,     l);
	ASSIGN(unsigned long long,  123,     u);
	ASSIGN(float,              -123.456, d);
	ASSIGN(double,             -123.456, d);
	ASSIGN(char const*,        "text",   s);
	ASSIGN(std::string,        "text",   s);
	ASSIGN(bool,               true,     b);
	
	std::cout << "\n****** type cast *****\n";
#define TYPE_CAST(TYPE, VALUE, FIELD) \
{ \
	TYPE var = VALUE; \
	json::value val(var); \
	TYPE var2 = val; \
	print_padded(#TYPE, 20); \
	compare_check(var, var2); \
	std::cout << std::endl; \
}
	TYPE_CAST(short,              -123,     i);
	TYPE_CAST(unsigned short,      123,     i);
	TYPE_CAST(int,                -123,     i);
	TYPE_CAST(unsigned int,        123,     u);
	TYPE_CAST(long,               -123,     l);
	TYPE_CAST(unsigned long,       123,     u);
	TYPE_CAST(long long,          -123,     l);
	TYPE_CAST(unsigned long long,  123,     u);
	TYPE_CAST(float,              -123.456, d);
	TYPE_CAST(double,             -123.456, d);
	TYPE_CAST(char const*,        "text",   s);
	TYPE_CAST(std::string,        "text",   s);
	TYPE_CAST(bool,               true,     b);

#if TEST_LEAKS != 0
	std::cout << "\n******* memory *******\n";
	if (g_allocs > 0) std::cout << g_allocs << " leaked objects";
	else std::cout << "no leaks";
	std::cout << ", " << g_total_allocs << " total allocations\n";
#endif

}

}
