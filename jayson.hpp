#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cstdlib>

namespace json
{

struct serialize_options
{
	bool        pretty_print;
	bool        java_style_braces;
	std::string indent;
	int         number_precision;

	serialize_options(bool pretty_print=true, bool java_style_braces=false, std::string const& indent="  ", int number_precision=2)
	: pretty_print(pretty_print)
	, java_style_braces(java_style_braces)
	, indent(indent)
	, number_precision(number_precision)
	{}
};

enum class type : char
{
	null    = ' ',
	boolean = 'b',
	number  = 'n',
	string  = 's',
	array   = 'a',
	object  = 'o'
};

class value
{
	friend class tests; // FIXME: get rid
	friend std::ostream& operator << (std::ostream&, value const&);

public:

	using keyval_t  = std::pair<std::string, value>;
	using keyvals_t = std::vector<keyval_t>;

	bool parse_string(char const* str) { reader r; return r.parse_string(str, *this, nullptr); }
	bool parse_string(char const* str, std::string& errors) { reader r; return r.parse_string(str, *this, &errors); }
	bool parse_file(char const* filename) { reader r; return r.parse_file(filename, *this, nullptr); }
	bool parse_file(char const* filename, std::string& errors) { reader r; return r.parse_file(filename, *this, &errors); }

	char const* serialize(serialize_options const& options = serialize_options()) const
	{
		thread_local strbuf_t buf;
		writer w(buf);
		w.write(*this, options);
		return buf.data();
	}

	bool serialize(char const* filename, serialize_options const& options = serialize_options()) const
	{
		char const* str = serialize(options);
		std::ofstream ofs(filename);
		if (ofs)
		{
			ofs << str;
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
	using array_t  = std::vector<value>;
	
	class object_t
	{
	private:
	
		// TODO: optimize (2 strings for key)
		using map_t = std::unordered_map<std::string, std::size_t>;
	
		keyvals_t values;
		map_t     map;
		
	public:
		
		void remove(char const* key)
		{
			auto it = map.find(key);
			if (it != map.end())
			{
				std::size_t index = it->second;
				for (auto& pair : map)
				{
					if (pair.second > index) --pair.second;
				}
				values.erase(values.begin() + index);
				map.erase(it);
			}
		}
		
		bool empty() const { return values.empty(); }
		std::size_t size() const { return values.size(); }
		keyval_t const& operator [] (std::size_t index) const { return values[index]; }
		keyvals_t const& get_pairs() const { return values; }

		value const& get_const(std::string const& key) const
		{
			auto it = map.find(key);
			return it != map.end() ? values[it->second].second : value::null();
		}

		value& get(std::string const& key)
		{
			auto it = map.find(key);
			if (it != map.end())
			{
				return values[it->second].second;
			}
			else
			{
				map.emplace(key, values.size());
				values.emplace_back(key, value());
				return values.back().second;
			}
		}
	};

	union
	{
		bool      b;
		double    n;
		string_t* s;
		array_t*  a;
		object_t* o;
	} data;
	
	type type;

public:
	
	static value const& null() { static value val; return val; }
	
	~value()
	{
		if (type == type::string || type == type::array || type == type::object)
		{
			switch (type)
			{
				case type::string: delete data.s; break;
				case type::array:  delete data.a; break;
				case type::object: delete data.o; break;
				case type::null:
				case type::boolean:
				case type::number:;
			}
		}
	}

	// MARK: constructors
	value()                     : value(type::null)    {}
	value(value const& v)       : value(type::null)    { *this = v; }
	value(value&& v) noexcept   : value(type::null)    { std::swap(type, v.type); std::swap(data, v.data); }
	value(bool v)               : value(type::boolean) {  data.b = v; }
	value(char const* v)        : value(type::string)  { *data.s = v; }
	value(std::string const& v) : value(type::string)  { *data.s = v; }
	value(ilist_t const& list)  : value(type::array)   { *data.a = list; }
	value(ilist_t&& list)       : value(type::array)   { *data.a = list; }
	template <typename T> value(T const& t) : value(type::number) { data.n = t; }

	value(enum type t) : type(t)
	{
		switch (type)
		{
			case type::string: data.s = new string_t(); break;
			case type::array:  data.a = new array_t();  break;
			case type::object: data.o = new object_t(); break;
			default:;
		}
	}

	// MARK: assignment operators
	value& operator = (value const& v)
	{
		check_type(v.type);
		switch (type)
		{
			case type::boolean: data.b =  v.data.b; break;
			case type::number:  data.n =  v.data.n; break;
			case type::string: *data.s = *v.data.s; break;
			case type::array:  *data.a = *v.data.a; break;
			case type::object: *data.o = *v.data.o; break;
			case type::null:;
		}
		return *this;
	}

	value& operator = (value&& v)                 { std::swap(type, v.type); std::swap(data, v.data); return *this; }
	value& operator = (ilist_t const& list)       { check_type(type::array);  *data.a = list; return *this; }
	value& operator = (bool b)                    { check_type(type::boolean); data.b = b;    return *this; }
	value& operator = (char const* str)           { check_type(type::string); *data.s = str;  return *this; }
	value& operator = (std::string const& str)    { check_type(type::string); *data.s = str;  return *this; }
	template <typename T> value& operator = (T n) { check_type(type::number);  data.n = n;    return *this; }
	
	// MARK: type checks
	bool is(enum type atype) const { return type == atype; }
	bool is_null()   const { return type == type::null;   }
	bool is_number() const { return type == type::number; }
	bool is_string() const { return type == type::string; }
	bool is_array()  const { return type == type::array;  }
	bool is_object() const { return type == type::object; }

	// MARK: type conversions
	template <typename T> T as() const { return type == type::number ? data.n : 0; }
	template <typename T> operator T () const { return as<T>(); }
	
	// MARK: comparison operators
	bool operator == (char const* s)        const { return (type == type::string && s) ? *data.s == s : false; }
	bool operator == (std::string const& s) const { return  type == type::string       ? *data.s == s : false; }
	
	template <typename T> bool operator == (T const& v) const { return static_cast<T>(*this) == v; }
	template <typename T> bool operator != (T const& v) const { return !(*this == v); }

	std::size_t size() const
	{
		if      (type == type::array)  return data.a->size();
		else if (type == type::object) return data.o->size();
		return 0;
	}

	// MARK: array access
	value& append(value const& v = value())
	{
		check_type(type::array);
		data.a->emplace_back(v);
		return data.a->back();
	}

	value& append(value&& v)
	{
		check_type(type::array);
		data.a->emplace_back(v);
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
	keyvals_t const& get_pairs() const // FIXME: rework
	{
		static keyvals_t empty;
		return type == type::object ? data.o->get_pairs() : empty;
	}
	
	value const& operator () (std::string const& key) const { return type == type::object ? data.o->get_const(key) : null(); }
	value&       operator () (std::string const& key)       { check_type(type::object); return data.o->get(key); }
	
	void remove(char const* key) { if (type == type::object) data.o->remove(key); }
	void remove(std::string const& key) { remove(key.c_str()); }
	
	void remove(std::size_t index)
	{
		if (type == type::array) data.a->erase(data.a->begin() + index);
	}
	
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
		
		void reserve(size_t cap)
		{
			if (m_capacity != cap)
			{
				m_capacity = cap;
				head = (char*)std::realloc(head, m_capacity);
			}
		}
		
		void push_back(char c)
		{
			alloc_if_needed(m_size + 1);
			head[m_size++] = c;
		}

	private:

		char*  head = nullptr;
		size_t m_capacity = 0;
		size_t m_size = 0;
		
		void alloc_if_needed(size_t size)
		{
			if (size < m_capacity) return;
			if (m_capacity == 0) m_capacity = 4096;
			else while (size >= m_capacity) m_capacity *= 2;
			head = (char*)realloc(head, m_capacity);
		}
	};

	// MARK: parser
	class reader
	{
	public:
	
		bool parse_file(char const* filename, value& result, std::string* errors)
		{
			std::ifstream stream(filename);
			if (stream)
			{
				std::stringstream ss;
				ss << stream.rdbuf();
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
				result = value();
				return false;
			}
		}
		
	private:
		
		using fail = std::runtime_error;
		
		char const* source;
		std::size_t line_num;
		
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
				default:  val = read_number(); break;
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
					std::string key = read_string();
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
				
				read_value(val.append());
				skip_whitespaces();
				if (*source == ',') ++source;
			}
			throw fail("unexpected end of array");
		}
		
		double read_number()
		{
			bool negate = false;
			auto c = *source;
			if      (c == '-')             { negate = true;  ++source; }
			else if (c == '+')             { ++source; }
			else if (c >= '0' && c <= '9') { }
			else throw fail("invalid numeric value");
			
			double result = 0;
			while (*source && *source != '.' && *source != 'e' && *source != 'E' && *source >= '0' && *source <= '9')
			{
				result = result * 10 + (*source++ - '0');
			}
			
			if (*source && *source == '.')
			{
				++source;
				double mult = 0.1f;
				while (*source && *source != 'e' && *source != 'E' && *source >= '0' && *source <= '9')
				{
					result += (*source++ - '0') * mult;
					mult *= 0.1f;
				}
			}
			
			if (*source && (*source == 'e' || *source == 'E'))
			{
				++source;
				double exp_mult;
				if      (*source == '-') { exp_mult = 0.1f; ++source; }
				else if (*source == '+') { exp_mult = 10;   ++source; }
				else                     { exp_mult = 10; }
				
				std::size_t exp = 0;
				while (*source && *source >= '0' && *source <= '9')
				{
					exp = exp * 10 + (*source++ - '0');
				}
				
				if (exp > 308) exp = 308;
				while (exp-- != 0) result *= exp_mult;
			}
			
			return negate ? -result : result;
		}
		
		void read_unicode_symbol(std::ostringstream& ss)
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
				ss << static_cast<char>(code);
			}
			else if (code < 0x800)
			{
				ss
				<< static_cast<char>(((code >> 6) & 0x1f) | 0xc0)
				<< static_cast<char>((code & 0x3f) | 0x80);
			}
			else
			{
				ss
				<< static_cast<char>(((code >> 12) & 0x1f) | 0xe0)
				<< static_cast<char>(((code >> 6) & 0x3f) | 0x80)
				<< static_cast<char>((code & 0x3f) | 0x80);
			}
		}
		
		void read_escaped_symbol(std::ostringstream& ss)
		{
			if (*++source)
			{
				switch (*source)
				{
					case '"':  ss << '"';  break;
					case '/':  ss << '/';  break;
					case '\\': ss << '\\'; break;
					case 'b':  ss << '\b'; break;
					case 'f':  ss << '\f'; break;
					case 'n':  ss << '\n'; break;
					case 'r':  ss << '\r'; break;
					case 't':  ss << '\t'; break;
					case 'u':  read_unicode_symbol(ss); break;
					default: throw fail("invalid escaped symbol");
				}
				++source;
			}
			else
			{
				throw fail("unexpected end of escaped symbol");
			}
		}
		
		std::string read_string()
		{
			thread_local std::ostringstream ss; // TODO: optimize?
			ss.str("");
			++source;
			while (*source)
			{
				switch (*source)
				{
				case '\\': read_escaped_symbol(ss); break;
				case '"': ++source; return ss.str();
				default: ss << *source++;
				}
			}
			throw fail("unexpected end of string");
		}
	};

	// MARK: serializer
	class writer
	{
	public:

		writer(strbuf_t& buf) : m_buf(buf) {}

		void write(value const& v, serialize_options const& options = serialize_options())
		{
			m_buf.clear();
			m_options = options;
			m_indents = 0;
			write_value(v);
			m_buf.push_back('\0');
		}

	private:
		
		using fail = std::runtime_error;

		writer(writer const&) = delete;
		writer(writer&&) = delete;
		writer& operator = (writer const&) = delete;
		writer& operator = (writer&&) = delete;

		strbuf_t&         m_buf;
		int               m_indents;
		serialize_options m_options;
		
		char const* number_to_string(double n, int precision)
		{
			thread_local char buf[128];
			if (n == 0) return "0";
			else if (std::isinf(n)) return "inf";
			else if (std::isnan(n)) return "nan";
			else
			{
				char* dst = buf;
				
				if (n < 0)
				{
					*dst++ = '-';
					n = -n;
				}
				
				long p_f = exp(precision * M_LN10);// pow10(precision);
				long integ = floor(n);
				long fract = floor((n - integ) * p_f + 0.5);
				
				if (fract == p_f)
				{
					++integ;
					fract = 0;
				}
				
				if (integ > 0)
				{
					int log_i = log10(n);
					long p_i = exp(log_i * M_LN10);//pow10(log_i);
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
				
				*dst = '\0';
			}
			return buf;
		}
		
		void write_raw_string(char const* str)
		{
			while (*str)
			{
				m_buf.push_back(*str++);
			}
		}

		void write_string(std::string const* str)
		{
			if (!str) return;
			char const* p = str->c_str();
			while (*p)
			{
				if ((*p & 0xc0) == 0xc0)
				{
					int count = (*p & 0xe0) == 0xe0 ? 2 : 1;
					
					uint16_t code = *p & 0x1f;
					for (int i=0; i<count; ++i)
					{
						if (!*++p) fail("invalid unicode symbol");
						code = (code << 6) | (*p & 0x3f);
					}
					
					auto hex_sym = [](uint16_t code, int index)
					{
						char c = (code >> (index * 4)) & 0x0f;
						c = c < 10 ? c + 48 : c + 87;
						return c;
					};
					
					m_buf.push_back('\\');
					m_buf.push_back('u');
					m_buf.push_back(hex_sym(code, 3));
					m_buf.push_back(hex_sym(code, 2));
					m_buf.push_back(hex_sym(code, 1));
					m_buf.push_back(hex_sym(code, 0));
				}
				else
				{
					switch (*p)
					{
					case '"':  m_buf.push_back('\\'); m_buf.push_back('\"'); break;
					case '\\': m_buf.push_back('\\'); m_buf.push_back('\\'); break;
					case '\b': m_buf.push_back('\\'); m_buf.push_back('b'); break;
					case '\f': m_buf.push_back('\\'); m_buf.push_back('f'); break;
					case '\n': m_buf.push_back('\\'); m_buf.push_back('n'); break;
					case '\r': m_buf.push_back('\\'); m_buf.push_back('r'); break;
					case '\t': m_buf.push_back('\\'); m_buf.push_back('t'); break;
					default:   m_buf.push_back(*p); break;
					}
				}
				++p;
			}
		}
		
		void write_array(value const& v)
		{
			m_buf.push_back('[');
			if (!v.data.a->empty())
			{
				++m_indents;
				put_newline();
				
				for (auto const& it : *v.data.a)
				{
					put_indents();
					write_value(it);
					if (&it != &v.data.a->back()) m_buf.push_back(',');
					put_newline();
				}
				
				--m_indents;
				put_indents();
			}
			m_buf.push_back(']');
		}
		
		void write_object(value const& v)
		{
			m_buf.push_back('{');
			if (!v.data.o->empty())
			{
				++m_indents;
				put_newline();
				
				for (std::size_t i=0; i<v.data.o->size(); ++i)
				{
					auto const& it = (*v.data.o)[i];
					auto const& key = it.first;
					auto const& val = it.second;
					
					put_indents();
					m_buf.push_back('"');
					write_raw_string(key.c_str());
					m_buf.push_back('"');
					m_buf.push_back(':');

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

					if (i != v.data.o->size() - 1) m_buf.push_back(',');
					put_newline();
				}

				--m_indents;
				put_indents();
			}
			m_buf.push_back('}');
		}

		void write_value(value const& v)
		{
			switch (v.type)
			{
			case type::null:
				write_raw_string("null");
				break;

			case type::number:
				write_raw_string(number_to_string(v.data.n, m_options.number_precision));
				break;

			case type::boolean:
				write_raw_string(v.data.b ? "true" : "false");
				break;

			case type::string:
				m_buf.push_back('"');
				write_string(v.data.s);
				m_buf.push_back('"');
				break;

			case type::array:
				write_array(v);
				break;

			case type::object:
				write_object(v);
				break;
			}
		}

		void put_space()
		{
			if (m_options.pretty_print) m_buf.push_back(' ');
		}

		void put_newline()
		{
			if (m_options.pretty_print)
			{
				m_buf.push_back('\n');
			}
		}
		
		void put_indents()
		{
			if (m_options.pretty_print)
			{
				for (int i = 0; i < m_indents; ++i) write_raw_string(m_options.indent.c_str());
			}
		}
	};
};

// MARK: type conversion specializations
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

template <> inline bool value::as<bool>() const
{
	switch (type)
	{
	case type::null:    return  false;
	case type::boolean: return  data.b;
	case type::number:  return  data.n != 0;
	case type::string:  return !data.s->empty();
	case type::array:   return !data.a->empty();
	case type::object:  return !data.o->empty();
	}
	return false;
}

// MARK: helpers
inline std::ostream& operator << (std::ostream& ss, value const& val)
{
	switch (val.type)
	{
		case type::null:    ss << "null";                break;
		case type::boolean: ss << val.as<bool>();        break;
		case type::number:  ss << val.as<double>();      break;
		case type::string:  ss << val.as<std::string>(); break;
		case type::array:   ss << "array";               break;
		case type::object:  ss << "object";              break;
	}
	return ss;
}

}
