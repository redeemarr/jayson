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
	std::cout << std::endl; \
}
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
#define ASSIGN(TYPE, VALUE, FIELD) \
{ \
	TYPE var = VALUE; \
	json::value val; \
	val = var; \
	print_padded(#TYPE, 20); \
	compare_check(var, val.data.FIELD); \
	std::cout << std::endl; \
}
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
#define TYPE_CAST(TYPE, VALUE, FIELD) \
{ \
	TYPE var = VALUE; \
	json::value val(var); \
	TYPE var2 = val; \
	print_padded(#TYPE, 20); \
	compare_check(var, var2); \
	std::cout << std::endl; \
}
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
	
	std::cout << "\n****** type comparison *****\n";
#define TYPE_COMPARE(TYPE, VALUE, OP) \
{ \
	TYPE var = VALUE; \
	json::value val(var); \
	print_padded(#TYPE, 23); \
	std::cout << ": " << var << " " << #OP << " value(" << var << ") ? " << (val OP var ? "yes" : "no") << "\n"; \
}

#define TYPE_COMPARE_ALL(OP) \
	TYPE_COMPARE(short,              -123, OP); \
	TYPE_COMPARE(unsigned short,      123, OP); \
	TYPE_COMPARE(int,                -123, OP); \
	TYPE_COMPARE(unsigned int,        123, OP); \
	TYPE_COMPARE(long,               -123, OP); \
	TYPE_COMPARE(unsigned long,       123, OP); \
	TYPE_COMPARE(long long,          -123, OP); \
	TYPE_COMPARE(unsigned long long,  123, OP); \
	TYPE_COMPARE(float,              -123.456, OP); \
	TYPE_COMPARE(double,             -123.456, OP); \
	TYPE_COMPARE(char const*,        "text", OP); \
	TYPE_COMPARE(std::string,        "text", OP); \
	TYPE_COMPARE(bool,               true, OP); \
	TYPE_COMPARE(bool,               false, OP); \
	std::cout << "\n";

	TYPE_COMPARE_ALL(==);
	TYPE_COMPARE_ALL(!=);

	std::cout << "\n****** bool comparison *****\n";
#define BOOL_COMPARE(TYPE, VALUE) \
{ \
	TYPE var = VALUE; \
	json::value val(var); \
	print_padded(#TYPE, 23); \
	std::cout << ": value(" << var << ") == true ? " << (val == true ? "yes" : "no") << "\n"; \
}
	print_padded("json::null == true ? ", 23);
	std::cout << ": " << (json::value() == true ? "yes" : "no") << "\n";
	
	print_padded("json::object == true ? ", 23);
	std::cout << ": " << (json::value('o') == true ? "yes" : "no") << "\n";
	
	print_padded("json::array == true ? ", 23);
	std::cout << ": " << (json::value('a') == true ? "yes" : "no") << "\n";
	
	BOOL_COMPARE(int, 0);
	BOOL_COMPARE(int, 1);
	BOOL_COMPARE(float, 0);
	BOOL_COMPARE(float, 1);
	BOOL_COMPARE(char const*, "");
	BOOL_COMPARE(char const*, "abc");
	
	std::cout << "\n******** array *******\n";
	json::value arr;
	arr.append(123);
	arr.append("abc");
	arr.append(true);
	
	std::cout << "0: " << arr[0] << "\n";
	std::cout << "1: " << arr[1] << "\n";
	std::cout << "2: " << arr[2] << "\n";
	
	std::cout << "\n******* object *******\n";
	json::value obj;
	obj("a") = 123;
	obj("b") = "abc";
	obj("c") = true;
	
	std::cout << "a: " << obj("a") << "\n";
	std::cout << "b: " << obj("b") << "\n";
	std::cout << "c: " << obj("c") << "\n";

#if (TEST_LEAKS)
	std::cout << "\n******* memory *******\n";
	if (g_allocs > 0) std::cout << g_allocs << " leaked objects";
	else std::cout << "no leaks";
	std::cout << ", " << g_total_allocs << " total allocations\n";
#endif

}

}
