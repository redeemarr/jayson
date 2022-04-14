#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <cmath>

namespace json
{

using fail    = std::runtime_error;
using bytes_t = std::vector<char>;

struct serialize_options
{
	bool        pretty_print      = true;
	bool        java_style_braces = false;
	bool        utf8_escaping     = true;
	std::string indent            = "  ";
	int         number_precision  = 2;
};

enum class type : char
{
	null     = ' ',
	boolean  = 'b',
	n_double = 'f',
	n_int32  = 'i',
	n_int64  = 'l',
	n_uint64 = 'u',
	string   = 's',
	binary   = 'x',
	array    = 'a',
	object   = 'o'
};

inline char const* type_string(type t)
{
	switch (t)
	{
		case type::null:     return "null";
		case type::boolean:  return "boolean";
		case type::n_double: return "double";
		case type::n_int32:  return "int32";
		case type::n_int64:  return "int64";
		case type::n_uint64: return "uint64";
		case type::string:   return "string";
		case type::binary:   return "binary";
		case type::array:    return "array";
		case type::object:   return "object";
		default:             return "<unknown>";
	}
}

class value
{
friend class tests;
public:

	using obj_map_t   = std::unordered_map<std::string, value>;
	using obj_order_t = std::vector<obj_map_t::iterator>;
	using array_t     = std::vector<value>;
	
	bool from_string(char const* str, std::string* errors = nullptr)
	{
		json_reader r;
		return r.parse_string(str, *this, errors);
	}

	char const* to_string(serialize_options const& options = serialize_options()) const
	{
		thread_local strbuf_t buf;
		json_writer w(buf);
		w.write(*this, options);
		return buf.data();
	}
	
	bool from_json_file(char const* filename, std::string* errors = nullptr)
	{
		json_reader r;
		return r.parse_file(filename, *this, errors);
	};

	bool to_json_file(char const* filename, serialize_options const& options = serialize_options()) const
	{
		std::ofstream ofs(filename);
		if (ofs)
		{
			ofs << to_string(options);
			return true;
		}
		else
		{
			return false;
		}
		return false;
	}

	bool from_bytes(bytes_t const& data, std::string* errors = nullptr)
	{
		bson_reader r;
		return r.parse_data(data, *this, errors);
	}
	
	bytes_t to_bytes() const
	{
		bson_writer w;
		w.write_value(nullptr, *this);
		return w.data;
	}

	bool from_bson_file(char const* filename, std::string* errors = nullptr)
	{
		bson_reader r;
		return r.parse_file(filename, *this, errors);
	}

	bool to_bson_file(char const* filename) const
	{
		std::ofstream ofs(filename);
		if (ofs)
		{
			bytes_t data = to_bytes();
			ofs.write(data.data(), data.size());
			return true;
		}
		else
		{
			return false;
		}
		return false;
	}

private:

	using ilist_t  = std::initializer_list<value>;
	using string_t = std::string;
	
	class object_t
	{
	public:
		
		void remove(char const* key)
		{
			auto it = map.find(key);
			if (it != map.end())
			{
				order.erase(std::find(order.begin(), order.end(), it));
				map.erase(it);
			}
		}
		
		bool empty() const { return map.empty(); }
		std::size_t size() const { return map.size(); }
		obj_order_t const& get_order() const { return order; }
		
		bool has_key(std::string const& key) const
		{
			return map.find(key) != map.end();
		}

		value const& get_const(std::string const& key) const
		{
			auto it = map.find(key);
			return it != map.end() ? it->second : value::null();
		}

		value& get(std::string const& key)
		{
			auto it = map.find(key);
			if (it != map.end())
			{
				return it->second;
			}
			else
			{
				auto at = map.emplace(key, value()).first;
				order.push_back(at);
				return at->second;
			}
		}
		
	private:
	
		obj_order_t order;
		obj_map_t   map;
	};

	union
	{
		bool      b;
		double    d;
		int32_t   i;
		int64_t   l;
		uint64_t  u;
		string_t* s;
		array_t*  a;
		object_t* o;
		bytes_t*  x;
	} data;
	
	type type;

public:
	
	static value const& null() { static value val; return val; }
	
	~value()
	{
		switch (type)
		{
			case type::string: delete data.s; break;
			case type::array:  delete data.a; break;
			case type::object: delete data.o; break;
			case type::binary: delete data.x; break;
			default:;
		}
	}

	// MARK: constructors
	value()                     : value(type::null)     {}
	value(value const& v)       : value(type::null)     { *this = v; }
	value(value&& v) noexcept   : value(type::null)     { std::swap(type, v.type); std::swap(data, v.data); }
	value(char const* v)        : value(type::string)   { *data.s = v; }
	value(std::string const& v) : value(type::string)   { *data.s = v; }
	value(bytes_t const& v)     : value(type::binary)   { *data.x = v; }
	value(ilist_t const& list)  : value(type::array)    { *data.a = list; }
	value(ilist_t&& list)       : value(type::array)    { *data.a = list; }

	template <typename T, typename std::enable_if<std::is_same<T, bool>::value, bool>::type = true>                                                               value(T v) : value(type::boolean)  { data.b = v; } // bool
	template <typename T, typename std::enable_if<std::is_integral<T>::value && sizeof(T) < sizeof(int32_t) && !std::is_same<T, bool>::value, bool>::type = true> value(T v) : value(type::n_int32)  { data.i = v; } // ints less than sizeof(int32) -> int32
	template <typename T, typename std::enable_if<std::is_integral<T>::value && sizeof(T) == sizeof(int32_t) && std::is_signed<T>::value, bool>::type = true>     value(T v) : value(type::n_int32)  { data.i = v; } // int32
	template <typename T, typename std::enable_if<std::is_integral<T>::value && sizeof(T) == sizeof(int32_t) && std::is_unsigned<T>::value, bool>::type = true>   value(T v) : value(type::n_uint64) { data.u = v; } // uint32 -> uint64
	template <typename T, typename std::enable_if<std::is_integral<T>::value && sizeof(T) == sizeof(int64_t) && std::is_signed<T>::value, bool>::type = true>     value(T v) : value(type::n_int64)  { data.l = v; } // int64
	template <typename T, typename std::enable_if<std::is_integral<T>::value && sizeof(T) == sizeof(int64_t) && std::is_unsigned<T>::value, bool>::type = true>   value(T v) : value(type::n_uint64) { data.u = v; } // uint64
	template <typename T, typename std::enable_if<std::is_floating_point<T>::value && sizeof(T) <= sizeof(double), bool>::type = true>                            value(T v) : value(type::n_double) { data.d = v; } // float/double

	value(enum type t) : type(t)
	{
		switch (type)
		{
			case type::string: data.s = new string_t(); break;
			case type::array:  data.a = new array_t();  break;
			case type::object: data.o = new object_t(); break;
			case type::binary: data.x = new bytes_t();  break;
			default:;
		}
	}

	// MARK: assignment operators
	value& operator = (value const& v)
	{
		check_type(v.type);
		switch (type)
		{
			case type::boolean : data.b  =  v.data.b; break;
			case type::string  : *data.s = *v.data.s; break;
			case type::array   : *data.a = *v.data.a; break;
			case type::object  : *data.o = *v.data.o; break;
			case type::binary  : *data.x = *v.data.x; break;
			case type::n_double: data.d  =  v.data.d; break;
			case type::n_int32 : data.i  =  v.data.i; break;
			case type::n_int64 : data.l  =  v.data.l; break;
			case type::n_uint64: data.u  =  v.data.u; break;
			case type::null:;
		}
		return *this;
	}
	
	value& operator = (value&& v) { std::swap(type, v.type); std::swap(data, v.data); return *this; }
	
	enum type get_type() const { return type; }
	
	// MARK: type checks
	bool is(enum type atype) const { return type == atype;  }
	bool is_null()   const { return type == type::null;     }
	bool is_bool()   const { return type == type::boolean;  }
	bool is_number() const { return type == type::n_double || type == type::n_int32 || type == type::n_int64 || type == type::n_uint64;  }
	bool is_double() const { return type == type::n_double; }
	bool is_int32()  const { return type == type::n_int32;  }
	bool is_int64()  const { return type == type::n_int64;  }
	bool is_uint64() const { return type == type::n_uint64; }
	bool is_string() const { return type == type::string;   }
	bool is_array()  const { return type == type::array;    }
	bool is_object() const { return type == type::object;   }
	bool is_binary() const { return type == type::binary;   }

	// MARK: type conversions
	template <typename T> operator T () const { return as<T>(); }
	template <typename T> T as() const // default behavior is number, explicit specializations for other types
	{
		switch (type)
		{
		case type::n_double: return data.d;
		case type::n_int32:  return data.i;
		case type::n_int64:  return data.l;
		case type::n_uint64: return data.u;
		default: return 0;
		}
	}

	std::size_t size() const
	{
		if      (type == type::array)  return data.a->size();
		else if (type == type::object) return data.o->size();
		return 0;
	}

	// MARK: array access
	array_t const& array() const
	{
		static array_t empty;
		return type == type::array ? *data.a : empty;
	}
	
	value& append(value const& v = value())
	{
		check_type(type::array);
		data.a->emplace_back(v);
		return data.a->back();
	}

	value& append(value&& v)
	{
		check_type(type::array);
		data.a->emplace_back(std::move(v));
		return data.a->back();
	}

	value const& operator [] (std::size_t index) const
	{
		if (type == type::array && data.a && index < data.a->size()) return (*data.a)[index];
		else return null();
	}

	value& operator [] (std::size_t index)
	{
		check_type(type::array);
		if (index >= data.a->size()) data.a->resize(index + 1);
		return (*data.a)[index];
	}
	
	// MARK: object access
	obj_order_t const& object() const
	{
		static obj_order_t empty;
		return type == type::object ? data.o->get_order() : empty;
	}
	
	bool has_key(std::string const& key) const
	{
		return type == type::object ? data.o->has_key(key) : false;
	}
	
	value const& operator () (std::string const& key) const { return type == type::object ? data.o->get_const(key) : null(); }
	value&       operator () (std::string const& key)       { check_type(type::object); return data.o->get(key); }
	
	void remove_key(char const* key) { if (type == type::object) data.o->remove(key); }
	void remove_key(std::string const& key) { remove_key(key.c_str()); }
	
private:
	
	void check_type(enum type t)
	{
		if (type != t)
		{
			this->~value();
			new (this) value(t);
		}
	}

	class strbuf_t
	{
	public:

		~strbuf_t() { std::free(head); }

		void clear() { m_size = 0; }
		size_t capacity() const { return m_capacity; }
		char const* data() const { return head; }

		strbuf_t& operator << (char c)
		{
			resize(m_size + 1);
			head[m_size++] = c;
			return *this;
		}
		
		void reserve(size_t cap)
		{
			if (m_capacity != cap)
			{
				m_capacity = cap;
				head = (char*)std::realloc(head, m_capacity);
			}
		}
		
		void write(char const* str, size_t len)
		{
			resize(m_size + len);
			memcpy(&head[m_size], str, len);
			m_size += len;
		}
		
		void write(std::string const& str)
		{
			write(str.c_str(), str.length());
		}

	private:

		char*  head = nullptr;
		size_t m_capacity = 0;
		size_t m_size = 0;
		
		void resize(size_t size)
		{
			if (size < m_capacity) return;
			if (m_capacity == 0) m_capacity = 4096;
			else while (size >= m_capacity) m_capacity *= 2;
			head = (char*)realloc(head, m_capacity);
		}
	};
	
#pragma mark -

	// MARK: json parser
	class json_reader
	{
	public:
	
		bool parse_file(char const* filename, value& result, std::string* errors)
		{
			std::ifstream file(filename);
			if (file)
			{
				std::stringstream ss;
				ss << file.rdbuf();
				return parse_string(ss.str().data(), result, errors);
			}
			else
			{
				if (errors) *errors = "failed to load file '" + std::string(filename) + "'";
				return false;
			}
		}
		
		bool parse_string(char const* string, value& result, std::string* errors)
		{
			line_num = 1;
			if (string)
			{
				source = string;
				try
				{
					read_value(result);
					return true;
				}
				catch (std::exception const &ex)
				{
					if (errors)
					{
						std::ostringstream oss;
						oss << ex.what() << " at line " << line_num;
						*errors = oss.str();
					}
					return false;
				}
			}
			else
			{
				if (errors) *errors = "no data";
				result = value();
				return false;
			}
		}
		
	private:
		
		char const* source;
		std::size_t line_num;
		strbuf_t    strbuf;
		
		void skip_whitespaces()
		{
			while (*source)
			{
				if (*source == '\n') ++line_num;
				if (*source != ' ' && *source != '\t' && *source != '\r' && *source != '\n') return;
				else ++source;
			}
			throw fail("unexpected end of document");
		}
		
		void read_value(value& val)
		{
			skip_whitespaces();
			switch (*source)
			{
				case '[': read_array (val); break;
				case '{': read_object(val); break;
				case '"': val = read_string(); break;
				case 'n': skip_check("null");  val = value(); break;
				case 't': skip_check("true");  val = true;    break;
				case 'f': skip_check("false"); val = false;   break;
				default:  read_number(val); break;
			}
		}
		
		void skip_check(char const* str)
		{
			for (char const* p = str; *p; ++p)
			{
				if (*source++ != *p)
				{
					std::string err = "expected ";
					err += str;
					throw fail(err.c_str());
				}
			}
		}
		
		void read_object(value& val)
		{
			++source;
			val = value(type::object);
			while (*source)
			{
				skip_whitespaces();
				
				if (*source == '}')
				{
					++source;
					return;
				}
				
				if (*source == '"')
				{
					char const* key = read_string();
					skip_whitespaces();
					if (*source++ == ':')
					{
						skip_whitespaces();
						read_value(val(key));
						skip_whitespaces();
						if (*source == ',') ++source;
					}
					else
					{
						throw fail("expected ':' after pair key");
					}
				}
				else
				{
					throw fail("expected quoted pair key");
				}
			}
			throw fail("unexpected end of object");
		}

		void read_array(value& val)
		{
			++source;
			val = value(type::array);
			while (*source)
			{
				skip_whitespaces();
				if (*source == ']')
				{
					++source;
					return;
				}
				
				val.data.a->emplace_back();
				read_value(val.data.a->back());
				skip_whitespaces();
				if (*source == ',') ++source;
			}
			throw fail("unexpected end of array");
		}
		
		void read_number(value& val)
		{
			bool is_float = false;
			double sign = 1.0;
			char c = *source;
			if      (c == '-')             { sign = -1.0; ++source; }
			else if (c == '+')             { ++source; }
			else if (c >= '0' && c <= '9') { }
			else throw fail("invalid numeric value");

			int shift = 0;
			double result = 0;
			while (*source >= '0' && *source <= '9')
			{
				result = result * 10.0 + (*source++ - '0');
			}

			if (*source == '.')
			{
				is_float = true;
				++source;
				while (*source >= '0' && *source <= '9')
				{
					result = result * 10.0 + (*source++ - '0');
					--shift;
				}
			}

			if (*source == 'e' || *source == 'E')
			{
				is_float = true;
				++source;
				int exp_sign = 1;
				if      (*source == '-') { ++source; exp_sign = -1; }
				else if (*source == '+') { ++source; }
				
				int exponent = 0;
				while (*source >= '0' && *source <= '9')
				{
					exponent = exponent * 10 + (*source++ - '0');
				}
				
				shift += exponent * exp_sign;
			}
			
			if      (shift < 0) while (shift++ != 0) result *= 0.1;
			else if (shift > 0) while (shift-- != 0) result *= 10.0;
			
			if (is_float)
			{
				val = result * sign;
			}
			else
			{
				if (result < 0x7FFFFFFF)
				{
					val = static_cast<int>(result * sign);
				}
				else
				{
					if (sign < 0)
					{
						val = static_cast<int64_t>(result * sign);
					}
					else
					{
						val = static_cast<uint64_t>(result);
					}
				}
			}
		}
		
		void read_unicode_symbol(strbuf_t& sb)
		{
			uint16_t code = 0;
			for (int i=0; i<4; ++i)
			{
				uint16_t sym = *++source;
				if (!sym) throw fail("unexpected end of unicode symbol");
				if      (sym >= '0' && sym <= '9') sym -= 48;
				else if (sym >= 'a' && sym <= 'f') sym -= 87;
				else if (sym >= 'A' && sym <= 'F') sym -= 55;
				else throw fail("invalid hex value");
				code = (code << 4) | sym;
			}
			
			if (code < 0x80)
			{
				sb << static_cast<char>(code);
			}
			else if (code < 0x800)
			{
				sb
				<< static_cast<char>(((code >> 6) & 0x1f) | 0xc0)
				<< static_cast<char>((code & 0x3f) | 0x80);
			}
			else
			{
				sb
				<< static_cast<char>(((code >> 12) & 0x1f) | 0xe0)
				<< static_cast<char>(((code >> 6) & 0x3f) | 0x80)
				<< static_cast<char>((code & 0x3f) | 0x80);
			}
		}
		
		void read_escaped_symbol(strbuf_t& sb)
		{
			if (*++source)
			{
				switch (*source)
				{
					case '"':  sb << '"';  break;
					case '/':  sb << '/';  break;
					case '\\': sb << '\\'; break;
					case 'b':  sb << '\b'; break;
					case 'f':  sb << '\f'; break;
					case 'n':  sb << '\n'; break;
					case 'r':  sb << '\r'; break;
					case 't':  sb << '\t'; break;
					case 'u':  read_unicode_symbol(sb); break;
					default: throw fail("invalid escaped symbol");
				}
				++source;
			}
			else
			{
				throw fail("unexpected end of escaped symbol");
			}
		}
		
		char const* read_string()
		{
			strbuf.clear();
			++source;
			while (*source)
			{
				char const* end = source;
				while (*end != '\\' && *end != '"' && *end != '\0') ++end;
				
				if (source != end)
				{
					strbuf.write(source, end - source);
					source = end;
				}
				
				if (*source == '\\')
				{
					read_escaped_symbol(strbuf);
				}
				else if (*source == '"')
				{
					++source;
					strbuf << '\0';
					return strbuf.data();
				}
			}
			throw fail("unexpected end of string");
		}
	};

#pragma mark -

	// MARK: json serializer
	class json_writer
	{
	public:

		json_writer(strbuf_t& buf) : m_buf(buf) {}

		void write(value const& v, serialize_options const& options = serialize_options())
		{
			m_buf.clear();
			m_options = options;
			m_indents = 0;
			write_value(v);
			m_buf << '\0';
		}

	private:

		json_writer(json_writer const&) = delete;
		json_writer(json_writer&&) = delete;
		json_writer& operator = (json_writer const&) = delete;
		json_writer& operator = (json_writer&&) = delete;

		strbuf_t&         m_buf;
		int               m_indents;
		serialize_options m_options;
		
		void write_float(double n, int precision)
		{
			double const ln10 = 2.30258509299404568402;

			if (n == 0) m_buf << '0';
			else if (std::isinf(n) || std::isnan(n)) m_buf.write("null", 4);
			else
			{
				thread_local char buf[128];
				char* dst = buf;
				
				if (n < 0)
				{
					*dst++ = '-';
					n = -n;
				}
				
				long p_f = exp(precision * ln10);// pow10(precision);
				long integ = floor(n);
				long fract = floor((n - integ) * p_f + 0.5);
				
				if (fract == p_f)
				{
					++integ;
					fract = 0;
				}
				
				if (integ > 0)
				{
					int log_i = log10(integ);
					long p_i = exp(log_i * ln10);//pow10(log_i);
					for (int i=0; i<=log_i; ++i)
					{
						long dig = integ / p_i;
						integ -= dig * p_i;
						*dst++ = dig + 48;
						p_i /= 10;
					}
				}
				else
				{
					*dst++ = '0';
				}
				
				if (fract > 0)
				{
					*dst++ = '.';
					for (int i=0; i<precision; ++i)
					{
						p_f /= 10;
						if (fract == 0) break;
						long dig = fract / p_f;
						fract -= dig * p_f;
						*dst++ = dig + 48;
					}
				}
				
				m_buf.write(buf, dst - buf);
			}
		}
		
		template <typename T> void write_integer(T n)
		{
			thread_local char buf[32] = { 0 };

			bool neg = false;
			if (n < 0)
			{
				neg = true;
				n = -n;
			}
			
			char* end = buf + sizeof(buf) - 1;
			char* ptr = end;
			
			do
			{
				*ptr-- = '0' + n % 10;
				n = n / 10;
			} while (n != 0);
			
			if (neg) *ptr-- = '-';
			
			m_buf.write(ptr + 1, end - ptr);
		}

		void write_string(char const* str)
		{
			while (*str)
			{
				if (m_options.utf8_escaping && (*str & 0xc0) == 0xc0)
				{
					int count = (*str & 0xe0) == 0xe0 ? 2 : 1;
					
					uint16_t code = *str & 0x1f;
					for (int i=0; i<count; ++i)
					{
						if (!*++str) fail("invalid unicode symbol");
						code = (code << 6) | (*str & 0x3f);
					}
					
					auto hex_sym = [](uint16_t code, int index)
					{
						char c = (code >> (index * 4)) & 0x0f;
						c = c < 10 ? c + 48 : c + 87;
						return c;
					};
					
					char chars[6] = { '\\', 'u' };
					chars[2] = hex_sym(code, 3);
					chars[3] = hex_sym(code, 2);
					chars[4] = hex_sym(code, 1);
					chars[5] = hex_sym(code, 0);
					m_buf.write(chars, 6);
				}
				else
				{
					switch (*str)
					{
					case  '"': m_buf << '\\' << '\"'; break;
					case '\\': m_buf << '\\' << '\\'; break;
					case '\b': m_buf << '\\' << 'b';  break;
					case '\f': m_buf << '\\' << 'f';  break;
					case '\n': m_buf << '\\' << 'n';  break;
					case '\r': m_buf << '\\' << 'r';  break;
					case '\t': m_buf << '\\' << 't';  break;
					default:   m_buf << *str;         break;
					}
				}
				++str;
			}
		}
		
		void write_array(value const& v)
		{
			m_buf << '[';
			if (!v.data.a->empty())
			{
				++m_indents;
				put_newline();
				
				for (auto const& it : *v.data.a)
				{
					put_indents();
					write_value(it);
					if (&it != &v.data.a->back()) m_buf << ',';
					put_newline();
				}
				
				--m_indents;
				put_indents();
			}
			m_buf << ']';
		}
		
		void write_object(value const& v)
		{
			m_buf << '{';
			if (!v.data.o->empty())
			{
				++m_indents;
				put_newline();
				
				size_t index = 0;
				for (auto const& it : v.data.o->get_order())
				{
					auto const& key = it->first;
					auto const& val = it->second;
					
					put_indents();
					m_buf << '"';
					write_string(key.c_str());
					m_buf << '"' << ':';

					if (val.size() > 0)
					{
						if (m_options.java_style_braces)
						{
							put_space();
						}
						else
						{
							put_newline();
							put_indents();
						}
					}
					else
					{
						put_space();
					}

					write_value(val);
					
					if (index++ != v.data.o->size() - 1) m_buf << ',';

					put_newline();
				}

				--m_indents;
				put_indents();
			}
			m_buf << '}';
		}

		void write_value(value const& v)
		{
			switch (v.type)
			{
			case type::null:
				m_buf.write("null", 4);
				break;
				
			case type::n_double:
				write_float(v.data.d, m_options.number_precision);
				break;
				
			case type::n_int32:
				write_integer(v.data.i);
				break;
				
			case type::n_int64:
				write_integer(v.data.l);
				break;
				
			case type::n_uint64:
				write_integer(v.data.u);
				break;

			case type::boolean:
				v.data.b ? m_buf.write("true", 4) : m_buf.write("false", 5);
				break;

			case type::string:
				m_buf << '"';
				if (v.data.s) write_string(v.data.s->c_str());
				m_buf << '"';
				break;

			case type::array:
				write_array(v);
				break;

			case type::object:
				write_object(v);
				break;
				
			case type::binary:
				write_string("<binary>");
				break;
			}
		}

		void put_space()
		{
			if (m_options.pretty_print) m_buf << ' ';
		}

		void put_newline()
		{
			if (m_options.pretty_print) m_buf << '\n';
		}
		
		void put_indents()
		{
			if (m_options.pretty_print)
			{
				for (int i = 0; i < m_indents; ++i) m_buf.write(m_options.indent);
			}
		}
	};
	
#pragma mark -
	
	enum bson_t
	{
		bson_double   = 0x01,
		bson_string   = 0x02,
		bson_document = 0x03,
		bson_array    = 0x04,
		bson_binary   = 0x05,
		// Deprecated   0x06 - Undefined (value)
		//              0x07 - ObjectId
		bson_bool     = 0x08,
		bson_utc_time = 0x09,
		bson_null     = 0x0a,
		//              0x0b - regex (cstring, cstring)
		// Deprecated   0x0c - DBPointer
		//              0x0d - JavaScript code
		// Deprecated   0x0e - Symbol
		// Deprecated   0x0f - JavaScript code w/ scope
		bson_int32    = 0x10,
		bson_uint64   = 0x11,
		bson_int64    = 0x12,
		//              0x13 - 	128-bit decimal floating point
		//              0xff - 	Min key
		//              0x7f - 	Max key
	};
	
	// MARK: bson parser
	class bson_reader
	{
	public:
	
		bool parse_file(char const* filename, value& result, std::string* errors)
		{
			std::ifstream file(filename, std::ios::binary | std::ios::ate);
			if (file)
			{
				thread_local bytes_t data;
				data.resize(file.tellg());
				file.seekg(0, std::ios::beg);
				file.read(data.data(), data.size());
				return parse_data(data, result, errors);
			}
			else
			{
				if (errors) *errors = "failed to load file '" + std::string(filename) + "'";
				return false;
			}
		}
		
		bool parse_data(bytes_t const& data, value& result, std::string* errors)
		{
			if (!data.empty())
			{
				ptr = data.data();
				end = ptr + data.size();
				result = value(type::object);
				try
				{
					read_document(result);
				}
				catch (std::exception const& ex)
				{
					if (errors) *errors = ex.what();
					return false;
				}
				return true;
			}
			else
			{
				if (errors) *errors = "no data";
				result = value();
				return false;
			}
		}
		
	private:
		
		char const* ptr;
		char const* end;
		
		struct pair_t
		{
			uint8_t type;
			char const* key;
		};
		
		bool read_pair(pair_t& pair)
		{
			pair.type = read<uint8_t>();
			if (pair.type == 0) return false;
			pair.key = fetch_string();
			return true;
		}
		
		void read_value(uint8_t type, value& tmp)
		{
			switch (type)
			{
				case bson_double:   tmp = read<double>();    break;
				case bson_int32:    tmp = read<int32_t>();   break;
				case bson_int64:
				case bson_utc_time: tmp = read<int64_t>();   break;
				case bson_uint64:   tmp = read<uint64_t>();  break;
				case bson_bool:     tmp = read<uint8_t>() > 0 ? true : false; break;
				case bson_null:     tmp = value(type::null); break;
				
				case bson_string:
					(void)read<int32_t>(); // string length
					tmp = fetch_string();
					break;
				
				case bson_document:
					tmp = value(type::object);
					read_document(tmp);
					break;
				
				case bson_array:
					tmp = value(type::array);
					read_array(tmp);
					break;
				
				case bson_binary:
					read_binary(tmp);
					break;
				
				default: throw fail("unsupported bson type id: " + std::to_string(type)); break;
			}
		}
		
		void read_binary(value& val)
		{
			val = value(type::binary);
			size_t size = read<int32_t>();
			auto& data = *val.data.x;
			data.resize(size);
			(void)read<uint8_t>(); // subtype
			check_end(size);
			memcpy(data.data(), ptr, data.size());
			ptr += size;
		}
		
		void read_document(value& val)
		{
			auto size = read<uint32_t>(); (void)size;
			pair_t pair;
			while (read_pair(pair))
			{
				read_value(pair.type, val(pair.key));
			}
		}
		
		void read_array(value& val)
		{
			auto size = read<uint32_t>(); (void)size;
			pair_t pair;
			while (read_pair(pair))
			{
				val.data.a->emplace_back();
				read_value(pair.type, val.data.a->back());
			}
		}
		
		template <typename T> T read()
		{
			check_end(sizeof(T));
			T* t = (T*)ptr;
			ptr += sizeof(T);
			return *t;
		}
		
		char const* fetch_string()
		{
			size_t len = strlen(ptr);
			check_end(len);
			char const* str = ptr;
			ptr += len + 1;
			return str;
		}
		
		void check_end(size_t pos)
		{
			if (ptr + pos > end) throw fail("end of data reached");
		}
	};

#pragma mark -

	// MARK: bson serializer
	struct bson_writer
	{
		bytes_t data;
		
		void write_value(char const* key, value const& val)
		{
			switch (val.type)
			{
				case type::object:
				{
					if (key)
					{
						write<uint8_t>(bson_document);
						write_string(key);
					}
					size_t beg = data.size();
					write<uint32_t>(0); // len
					for (auto const& it : val.data.o->get_order())
					{
						write_value(it->first.c_str(), it->second);
					}
					write<uint8_t>(0x00);
					uint32_t len = static_cast<uint32_t>(data.size() - beg);
					write_at(beg, len);
					break;
				}
				
				case type::array:
				{
					write<uint8_t>(bson_array);
					write_string(key);
					size_t beg = data.size();
					write<uint32_t>(0); // len
					for (size_t i=0; i<val.size(); ++i)
					{
						write_value(std::to_string(i).c_str(), val[i]);
					}
					write<uint8_t>(0x00);
					uint32_t len = data.size() - beg;
					write_at(beg, len);
					break;
				}
				
				case type::n_double:
					write<uint8_t>(bson_double);
					write_string(key);
					write(&val.data.d, sizeof(double));
					break;
				
				case type::n_int32:
					write<uint8_t>(bson_int32);
					write_string(key);
					write(&val.data.i, sizeof(int32_t));
					break;
				
				case type::n_int64:
					write<uint8_t>(bson_int64);
					write_string(key);
					write(&val.data.l, sizeof(int32_t));
					break;
				
				case type::n_uint64:
					write<uint8_t>(bson_uint64);
					write_string(key);
					write(&val.data.u, sizeof(int32_t));
					break;
				
				case type::boolean:
					write<uint8_t>(bson_bool);
					write_string(key);
					write<uint8_t>(val.data.b ? 1 : 0);
					break;
				
				case type::string:
					write<uint8_t>(bson_string);
					write_string(key);
					write<uint32_t>(val.data.s->length() + 1);
					write_string(val.data.s->c_str());
					break;
				
				case type::binary:
					write<uint8_t>(bson_binary);
					write_string(key);
					write<int32_t>(val.data.x->size());
					write<uint8_t>(0x00); // subtype
					write(val.data.x->data(), val.data.x->size());
					break;
				
				case type::null:
					write<uint8_t>(bson_null);
					break;
				
				default:
					throw fail(std::string("unsupported type ") + type_string(val.type));
					break;
			}
		}
		
		void write(void const* ptr, size_t size)
		{
			size_t prev = data.size();
			data.resize(data.size() + size);
			memcpy(data.data() + prev, ptr, size);
		}
		
		template <typename T> void write(T const& t) { write(&t, sizeof(T)); }
		void write_string(char const* str) { write(str, strlen(str) + 1); }
		void write_at(size_t at, uint32_t val) { *((uint32_t*)(data.data() + at)) = val; }
	};
};

// MARK: type conversion specializations
template <> inline bool value::as<bool>() const
{
	return type == type::boolean ? data.b : false;
}

template <> inline char const* value::as<char const*>() const
{
	return type == type::string ? data.s->c_str() : "";
}

template <> inline std::string value::as<std::string>() const
{
	return as<char const*>();
}

template <> inline std::string const& value::as<std::string const&>() const
{
	static std::string empty;
	return type == type::string ? *data.s : empty;
}

template <> inline bytes_t const& value::as<bytes_t const&>() const
{
	static bytes_t empty;
	return type == type::binary ? *data.x : empty;
}

template <> inline bytes_t value::as<bytes_t>() const
{
	return as<bytes_t const&>();
}

}
