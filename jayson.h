#ifndef JAYSON_H
#define JAYSON_H

// rev. 2

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <fstream>

namespace json
{

class value;

typedef char type_t;

static type_t const type_null    = ' ';
static type_t const type_number  = 'n';
static type_t const type_boolean = 'b';
static type_t const type_string  = 's';
static type_t const type_array   = 'a';
static type_t const type_object  = 'o';

typedef float                               number_t;
typedef bool                                boolean_t;
typedef std::string                         string_t;
typedef std::vector<value>                  array_t;
typedef std::unordered_map<string_t, value> object_t;

class value
{
public:

	value() : m_type(type_null) {}
	value(type_t type) : m_type(type) { create(); }
	value(value const& v) : m_type(type_null) { *this = v; }

	value(number_t v) : m_type(type_number)           { create();  data.n = v; }
	value(boolean_t v) : m_type(type_boolean)         { create();  data.b = v; }
	value(string_t const& v) : m_type(type_string)    { create(); *data.s = v; }
	value(array_t const& v) : m_type(type_array)      { create(); *data.a = v; }
	value(object_t const& v) : m_type(type_object)    { create(); *data.o = v; }

	value(char const* s) : m_type(type_string)        { create(); *data.s = s; }
	value(int n) : m_type(type_number)                { create(); data.n = static_cast<number_t>(n); }
	value(unsigned int n) : m_type(type_number)       { create(); data.n = static_cast<number_t>(n); }
	value(long n) : m_type(type_number)               { create(); data.n = static_cast<number_t>(n); }
	value(unsigned long n) : m_type(type_number)      { create(); data.n = static_cast<number_t>(n); }
	value(long long n) : m_type(type_number)          { create(); data.n = static_cast<number_t>(n); }
	value(unsigned long long n) : m_type(type_number) { create(); data.n = static_cast<number_t>(n); }
	value(double n) : m_type(type_number)             { create(); data.n = static_cast<number_t>(n); }

	~value() { destroy(); }

	value& operator = (value const& v)
	{
		type(v.m_type);
		switch (m_type)
		{
			case type_null:                        break;
			case type_number:  data.n =  v.data.n; break;
			case type_boolean: data.b =  v.data.b; break;
			case type_string: *data.s = *v.data.s; break;
			case type_array:  *data.a = *v.data.a; break;
			case type_object: *data.o = *v.data.o; break;
		}
		return *this;
	}

	inline type_t type() const { return m_type; }

	void type(type_t t)
	{
		if (m_type != t)
		{
			destroy();
			m_type = t;
			create();
		}
	}

	inline bool is_null()    const { return m_type == type_null;    }
	inline bool is_number()  const { return m_type == type_number;  }
	inline bool is_boolean() const { return m_type == type_boolean; }
	inline bool is_string()  const { return m_type == type_string;  }
	inline bool is_array()   const { return m_type == type_array;   }
	inline bool is_object()  const { return m_type == type_object;  }

	inline void             number(number_t val)   { set<number_t>(val); }
	inline number_t&        number()               { return get<number_t>(); }
	inline number_t const&  number() const         { return get_const<number_t>(); }

	inline void             boolean(boolean_t val) { set<boolean_t>(val); }
	inline boolean_t&       boolean()              { return get<boolean_t>(); }
	inline boolean_t const& boolean() const        { return get_const<boolean_t>(); }

	inline void             string(string_t val)   { set<string_t>(val); }
	inline string_t&        string()               { return get<string_t>(); }
	inline string_t const&  string() const         { return get_const<string_t>(); }

	inline void             array(array_t val)     { set<array_t>(val); }
	inline array_t&         array()                { return get<array_t>(); }
	inline array_t const&   array() const          { return get_const<array_t>(); }

	inline void             object(object_t val)   { set<object_t>(val); }
	inline object_t&        object()               { return get<object_t>(); }
	inline object_t const&  object() const         { return get_const<object_t>(); }

private:

	union data_t
	{
		number_t  n;
		boolean_t b;
		string_t* s;
		array_t*  a;
		object_t* o;
	};

	type_t m_type;
	data_t data;
	
	template <typename T> T& ref();
	template <typename T> T const& ref_const() const;
	template <typename T> type_t get_type() const;

	char const* type_name(type_t t) const
	{
		switch (t)
		{
		case type_null:    return "null";
		case type_number:  return "number";
		case type_boolean: return "boolean";
		case type_string:  return "string";
		case type_array:   return "array";
		case type_object:  return "object";
		}
		return "unknown";
	}

	void check_type(type_t t) const
	{
		if (m_type != t)
		{
			std::ostringstream oss;
			oss << "invalid json cast: " << type_name(m_type) << " -> " << type_name(t);
			throw std::runtime_error(oss.str());
		}
	}

	template <typename T> void set(T const& t)
	{
		type(get_type<T>());
		ref<T>() = t;
	}

	template <typename T> T& get()
	{
		check_type(get_type<T>());
		return ref<T>();
	}

	template <typename T> T const& get_const() const
	{
		check_type(get_type<T>());
		return ref_const<T>();
	}

	void destroy()
	{
		switch (m_type)
		{
		case type_string: delete data.s; break;
		case type_array:  delete data.a; break;
		case type_object: delete data.o; break;
		}
	}

	void create()
	{
		switch (m_type)
		{
		case type_string: data.s = new string_t(); break;
		case type_array:  data.a = new array_t();  break;
		case type_object: data.o = new object_t(); break;
		}
	}
};
	
template <> number_t&  value::ref<number_t>()  { return  data.n; }
template <> boolean_t& value::ref<boolean_t>() { return  data.b; }
template <> string_t&  value::ref<string_t>()  { return *data.s; }
template <> array_t&   value::ref<array_t>()   { return *data.a; }
template <> object_t&  value::ref<object_t>()  { return *data.o; }

template <> number_t const&  value::ref_const<number_t>()  const { return  data.n; }
template <> boolean_t const& value::ref_const<boolean_t>() const { return  data.b; }
template <> string_t const&  value::ref_const<string_t>()  const { return *data.s; }
template <> array_t const&   value::ref_const<array_t>()   const { return *data.a; }
template <> object_t const&  value::ref_const<object_t>()  const { return *data.o; }

template <> type_t value::get_type<number_t>()  const { return type_number;  }
template <> type_t value::get_type<boolean_t>() const { return type_boolean; }
template <> type_t value::get_type<string_t>()  const { return type_string;  }
template <> type_t value::get_type<array_t>()   const { return type_array;   }
template <> type_t value::get_type<object_t>()  const { return type_object;  }

class writer
{
public:

	struct options
	{
		bool formatted;
		std::string tab_character;

		options()
		: formatted(true)
		, tab_character("  ")
		{}
	};

	writer(std::ostream& os)
	: m_os(os)
	{
	}

	void write(value& v, options const& options = writer::options())
	{
		m_options = options;
		m_indents = 0;
		write_value(v);
	}

private:

	writer(writer&);
	writer& operator = (writer&);

	std::ostream& m_os;
	int     m_indents;
	options m_options;

	void write_string(std::string const& str)
	{
		for (auto c : str)
		{
			switch (c)
			{
			case '"':  m_os << "\\\""; break;
		//	case '/':  m_os << "\\/";  break;
			case '\\': m_os << "\\\\"; break;
			case '\b': m_os << "\\b";  break;
			case '\f': m_os << "\\f";  break;
			case '\n': m_os << "\\n";  break;
			case '\r': m_os << "\\r";  break;
			case '\t': m_os << "\\t";  break;
		//	case '\u': /* TODO */    break;
			default:
				m_os << c;
				break;
			}
		}
	}

	std::ostream& write_value(value& v)
	{
		switch (v.type())
		{
		case type_null:
			m_os << "null";
			break;

		case type_number:
			m_os << v.number();
			break;

		case type_boolean:
			m_os << (v.boolean() ? "true" : "false");
			break;

		case type_string:
			m_os << "\"";
			write_string(v.string());
			m_os << "\"";
			break;

		case type_array:
			{
				array_t const& a = v.array();
				put_newline();
				m_os << "[";
				++m_indents;
				put_newline();
				std::size_t i = 0;
				for (auto elem : a)
				{
					write_value(elem);
					if (++i != a.size())
					{
						m_os << ",";
						put_space();
					}
				}
				--m_indents;
				put_newline();
				m_os << "]";
			}
			break;

		case type_object:
			{
				put_newline();
				m_os << "{";
				++m_indents;
				put_newline();
				object_t const& o = v.object();
				std::size_t i = 0;
				for (auto it : o)
				{
					m_os << "\"" << it.first << "\"";
					put_space();
					m_os << ':';
					put_space();
					write_value(it.second);
					if (++i != o.size())
					{
						m_os << ",";
						put_newline();
					}
				}
				--m_indents;
				put_newline();
				m_os << "}";
			}
			break;
		}
		return m_os;
	}

	void put_space()
	{
		if (m_options.formatted) m_os << ' ';
	}

	void put_newline()
	{
		if (m_options.formatted)
		{
			m_os << '\n';
			for (int i = 0; i < m_indents; ++i) m_os << m_options.tab_character;
		}
	}
};

inline void to_file(char const* filename, value& v, writer::options const& options = writer::options())
{
	std::ofstream ofs(filename, std::ios::binary);
	writer w(ofs);
	w.write(v, options);
}

inline std::string to_string(value& v, writer::options const& options = writer::options())
{
	std::ostringstream oss;
	writer w(oss);
	w.write(v, options);
	return oss.str();
}

class reader
{
public:

	reader(std::istream& is) : is(is) {}

	bool read(value& result)
	{
		log_str.clear();
		if (is)
		{
			return read_value(result);
		}
		else
		{
			log_str = "json source data is unavailable";
			return false;
		}
	}

	std::string const& get_log() const { return log_str; }

private:

	typedef std::string::const_iterator pos_t;

	std::istream& is;
	std::string   log_str;

	reader(reader&);
	reader& operator = (reader&);

	void fail(char const* why)
	{
		std::streamoff c = is.tellg();
		std::streamoff p = c;
		while (p >= 0)
		{
			is.seekg(-1, std::ios::cur);
			p = is.tellg();
			if (is.peek() == '\n') break;
			log_str.insert(0, 1, is.peek());
		}

		is.seekg(c, std::ios::beg);
		log_str += "<?>";

		//p = is.tellg();
		while (is.peek() != '\n' && !is.eof())
		{
			int u = is.peek();
			if (is.eof()) break;
			if (is.fail()) break;
			log_str += u;
			is.seekg(1, std::ios::cur);
		}

		log_str.insert(0, ": ");
		log_str.insert(0, why);
	}

	inline bool skip_whitespaces()
	{
		while (!is.eof())
		{
			int c = is.peek();
			if (!is.eof() && c != ' ' && c != '\t' && c != '\r' && c != '\n')
			{
				return true;
			}
			else
			{
				is.ignore();
			}
		}
		fail("unexpected end of document");
		return false;
	}

	bool read_value(value& result)
	{
		if (!skip_whitespaces()) return false;
		switch (is.peek())
		{
		case '[': return read_array (result);
		case '{': return read_object(result);
		case '"': return read_string(result);
		case 'n': return read_null  (result);
		case 't': return read_true  (result);
		case 'f': return read_false (result);
		default:  return read_number(result);
		}
	}

	inline bool read_object(value& val)
	{
		val.type(type_object);
		is.ignore();
		while (!is.eof())
		{
			if (!skip_whitespaces()) return false;
			if (is.peek() == '"')
			{
				std::pair<string_t, value> pair;
				read_raw_string(pair.first);

				object_t& o = val.object();
				auto it = o.find(pair.first);
				if (it == o.end())
				{
					it = o.insert(o.end(), pair);
				}

				if (!skip_whitespaces()) return false;
				if (is.get() == ':')
				{
					if (!read_value(it->second)) return false;
					if (!skip_whitespaces()) return false;

					switch (is.get())
					{
					case ',': break;
					case '}': return true;
					default:
						fail("expected a ',' or '}' after key-value pair");
						return false;
					}
				}
				else
				{
					fail("expected ':' after pair key");
					return false;
				}
			}
			else
			{
				fail("expected pair key with quotes");
				return false;
			}
		}
		fail("unexpected end of object");
		return false;
	}

	inline bool read_array(value& val)
	{
		val.type(type_array);
		is.ignore();
		while (!is.eof())
		{
			array_t& arr = val.array();
			arr.push_back(value());
			value& cval = arr.back();
			if (!read_value(cval)) return false;
			if (!skip_whitespaces()) return false;

			switch (is.get())
			{
			case ',': break;
			case ']': return true;
			default:
				fail("expected a ',' or ']' after array element");
				return false;
			}
		}
		fail("unexpected end of array");
		return false;
	}

	inline bool read_null(value& val)
	{
		char c[4];
		is.read(c, 4);
		if (!is.eof() && c[0] == 'n' && c[1] == 'u' && c[2] == 'l' && c[3] == 'l')
		{
			val.type(type_null);
			return true;
		}
		else
		{
			fail("expected a 'null' value");
			return false;
		}
	}

	inline bool read_true(value& val)
	{
		char c[4];
		is.read(c, 4);
		if (!is.eof() && c[0] == 't' && c[1] == 'r' && c[2] == 'u' && c[3] == 'e')
		{
			val.boolean(true);
			return true;
		}
		else
		{
			fail("expected a 'true' value");
			return false;
		}
	}

	inline bool read_false(value& val)
	{
		char c[5];
		is.read(c, 5);
		if (!is.eof() && c[0] == 'f' && c[1] == 'a' && c[2] == 'l' && c[3] == 's' && c[4] == 'e')
		{
			val.boolean(false);
			return true;
		}
		else
		{
			fail("expected a 'false' value");
			return false;
		}
	}

	inline bool read_number(value& val)
	{
		bool negate;
		int c = is.peek();
		if      (c == '-')             { negate = true;  is.ignore(); }
		else if (c == '+')             { negate = false; is.ignore(); }
		else if (c >= '0' && c <= '9') { negate = false; }
		else { fail("invalid number value"); return false; }

		number_t result = 0;
		while (!is.eof() && is.peek() != '.' && is.peek() != 'e' && is.peek() != 'E' && is.peek() >= '0' && is.peek() <= '9')
		{
			result = result * 10 + (is.get() - '0');
		}

		if (!is.eof() && is.peek() == '.')
		{
			is.ignore();
			number_t mult = 0.1f;
			while (!is.eof() && is.peek() != 'e' && is.peek() != 'E' && is.peek() >= '0' && is.peek() <= '9')
			{
				result += (is.get() - '0') * mult;
				mult *= 0.1f;
			}
		}

		if (!is.eof() && (is.peek() == 'e' || is.peek() == 'E'))
		{
			is.ignore();
			number_t exp_mult;
			if      (is.peek() == '-') { exp_mult = 0.1f; is.ignore(); }
			else if (is.peek() == '+') { exp_mult = 10;   is.ignore(); }
			else                       { exp_mult = 10; }

			std::size_t exp = 0;
			while (!is.eof() && is.peek() >= '0' && is.peek() <= '9')
			{
				exp = exp * 10 + (is.get() - '0');
			}

			if (exp > 308) exp = 308;
			while (exp-- != 0) result *= exp_mult;
		}

		val.number(negate ? -result : result);
		return true;
	}

	inline bool read_string(value& val)
	{
		val.type(type_string);
		return read_raw_string(val.string());
	}

	inline bool read_raw_string(std::string& result)
	{
		is.ignore();
		while (!is.eof())
		{
			int c = is.peek();
			if (c == '\\')
			{
				if (!read_escaped_symbol(result)) return false;
			}
			else if (c == '"')
			{
				is.ignore();
				return true;
			}
			else
			{
				result += c;
				is.ignore();
			}
		}

		fail("unexpected end of string");
		return false;
	}

	inline bool read_escaped_symbol(std::string& to)
	{
		is.ignore();
		if (!is.eof())
		{
		//	wchar_t uc;
			switch (is.peek())
			{
			case '"':  to += '"';  break;
			case '/':  to += '/';  break;
			case '\\': to += '\\'; break;
			case 'b': to += '\b';  break;
			case 'f': to += '\f';  break;
			case 'n': to += '\n';  break;
			case 'r': to += '\r';  break;
			case 't': to += '\t';  break;
			case 'u':
			// TODO: unicode support?
			//	if (!read_unicode_char(uc)) return false;
			//	to += uc;
				break;

			default:
				fail("invalid escaped symbol");
				return false;
			}
			is.ignore();
		}
		else
		{
			fail("unexpected end of escaped symbol");
			return false;
		}
		return true;
	}
};

inline bool from_file(char const* filename, value& val)
{
	std::ifstream ifs(filename, std::ios::binary);
	if (ifs)
	{
		reader r(ifs);
		return r.read(val);
	}
	else
	{
		return false;
	}
}

inline bool from_file(char const* filename, value& val, std::string& log)
{
	std::ifstream ifs(filename, std::ios::binary);
	if (ifs)
	{
		reader r(ifs);
		bool b = r.read(val);
		if (!b) log = r.get_log();
		return b;
	}
	else
	{
		log = "file '" + std::string(filename) + "' was not found";
		return false;
	}
}

inline bool from_string(value& result, std::string const& str, std::string& log)
{
	std::stringstream ss;
	ss << str;
	reader r(ss);
	bool b = r.read(result);
	if (!b) log = r.get_log();
	return b;
}

};

#endif
