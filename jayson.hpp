#pragma once

#ifndef _MSC_VER
#define NOEXCEPT noexcept
#else
#define NOEXCEPT
#endif

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <iomanip>

namespace json
{

struct serialize_options
{
	bool        pretty_print;
	std::string tab_character;

	serialize_options(bool pretty_print=true, std::string const& tab_character="  ")
	: pretty_print(pretty_print)
	, tab_character(tab_character)
	{}
};

class value
{
	friend class tests; // FIXME: get rid
	friend std::ostream& operator << (std::ostream&, value const&);

public:

	bool parse_string(char const* str) { reader r; return r.parse_string(str, *this, nullptr); }
	bool parse_string(char const* str, std::string& errors) { reader r; return r.parse_string(str, *this, &errors); }
	bool parse_file(char const* filename) { reader r; return r.parse_file(filename, *this, nullptr); }
	bool parse_file(char const* filename, std::string& errors) { reader r; return r.parse_file(filename, *this, &errors); }

	std::string serialize(serialize_options const& options = serialize_options()) const
	{
		std::ostringstream oss;
		writer w(oss);
		w.write(*this, options);
		return oss.str();
	}

	bool serialize(char const* filename, serialize_options const& options = serialize_options()) const
	{
		std::ofstream ofs(filename, std::ios::binary);
		if (ofs)
		{
			writer w(ofs);
			w.write(*this, options);
			return true;
		}
		else
		{
			return false;
		}
	}

private:

	typedef std::initializer_list<value> ilist_t;
	typedef std::string string_t;
	typedef std::vector<value> array_t;

	enum type_t
	{
		type_null   = ' ',
		type_bool   = 'b',
		type_number = 'n',
		type_string = 's',
		type_array  = 'a',
		type_object = 'o'
	};
	
	class object_t
	{
	private:
	
		// TODO: optimize (2 strings for key)
		typedef std::pair<std::string, value> pair_t;
		typedef std::vector<pair_t> values_t;
		typedef std::unordered_map<std::string, std::size_t> map_t;
	
		values_t values;
		map_t    map;
		
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
		
		pair_t const& at(std::size_t index) const
		{
			return values[index];
		}

		value const& get(std::string const& key) const
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
	
	type_t type;

public:
	
	static value const& null() { static value val; return val; }
	
	~value()
	{
		if (type == type_string || type == type_array || type == type_object)
		{
			switch (type)
			{
				case type_string: delete data.s; break;
				case type_array:  delete data.a; break;
				case type_object: delete data.o; break;
				case type_null:
				case type_bool:
				case type_number:;
			}
		}
	}

	// MARK: constructors
	value()                     : value(type_null)   {}
	value(value const& v)       : value(type_null)   { *this = v; }
	value(value&& v) NOEXCEPT   : value(type_null)   { std::swap(type, v.type), std::swap(data, v.data); }
	value(bool v)               : value(type_bool)   {  data.b = v; }
	value(char const* v)        : value(type_string) { *data.s = v; }
	value(std::string const& v) : value(type_string) { *data.s = v; }
	value(ilist_t const& list)  : value(type_array)  { *data.a = list; }
	value(ilist_t&& list)       : value(type_array)  { *data.a = list; }
	template <typename T> value(T const& t) : value(type_number) { data.n = t; }

	// MARK: assignment operators
	value& operator = (value const& v)
	{
		check_type(v.type);
		switch (type)
		{
			case type_bool:    data.b =  v.data.b; break;
			case type_number:  data.n =  v.data.n; break;
			case type_string: *data.s = *v.data.s; break;
			case type_array:  *data.a = *v.data.a; break;
			case type_object: *data.o = *v.data.o; break;
			case type_null:;
		}
		return *this;
	}

	value& operator = (value&& v)                 { std::swap(type, v.type); std::swap(data, v.data); return *this; }
	value& operator = (ilist_t const& list)       { check_type(type_array);  *data.a = list; return *this; }
	value& operator = (bool b)                    { check_type(type_bool);    data.b = b;    return *this; }
	value& operator = (char const* str)           { check_type(type_string); *data.s = str;  return *this; }
	value& operator = (std::string const& str)    { check_type(type_string); *data.s = str;  return *this; }
	template <typename T> value& operator = (T n) { check_type(type_number);  data.n = n;    return *this; }

	// MARK: type conversions
	template <typename T> T as() const { return type == type_number ? data.n : 0; }
	template <typename T> operator T () const { return as<T>(); }
	
	// MARK: comparison operators
	bool operator == (char const* s)        const { return (type == type_string && s) ? *data.s == s : false; }
	bool operator == (std::string const& s) const { return  type == type_string       ? *data.s == s : false; }
	
	template <typename T> bool operator == (T const& v) const { return static_cast<T>(*this) == v; }
	template <typename T> bool operator != (T const& v) const { return !(*this == v); }

	std::size_t size() const
	{
		if      (type == type_array)  return data.a->size();
		else if (type == type_object) return data.o->size();
		return 0;
	}

	// MARK: array access
	value& append(value const& v = value())
	{
		check_type(type_array);
		data.a->emplace_back(v);
		return data.a->back();
	}

	value& append(value&& v)
	{
		check_type(type_array);
		data.a->emplace_back(v);
		return data.a->back();
	}

	value const& operator [] (std::size_t index) const
	{
		if (type == type_array && data.a && index < data.a->size()) return (*data.a)[index];
		else return null();
	}

	value& operator [] (std::size_t index)
	{
		check_type(type_array);
		if (index >= data.a->size()) data.a->resize(index + 1);
		return (*data.a)[index];
	}
	
	// MARK: object access
	value const& operator () (std::string const& key) const { return type == type_object ? data.o->get(key) : null(); }
	value&       operator () (std::string const& key)       { check_type(type_object); return data.o->get(key); }
	
	void remove(char const* key) { if (type == type_object) data.o->remove(key); }
	void remove(std::string const& key) { remove(key.c_str()); }
	
	void remove(std::size_t index)
	{
		if (type == type_array) data.a->erase(data.a->begin() + index);
	}
	
private:

	value(type_t t) : type(t)
	{
		switch (type)
		{
			case type_string: data.s = new string_t(); break;
			case type_array:  data.a = new array_t();  break;
			case type_object: data.o = new object_t(); break;
			default:;
		}
	}
		
	void check_type(type_t t)
	{
		if (type != t)
		{
			this->~value();
			new (this) value(t);
		}
	}

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
		
		typedef std::runtime_error fail;
		
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
				case '"': read_string(val); break;
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
			val = value(value::type_object);
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
					std::string key = read_raw_string();
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
			val = value(value::type_array);
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
		
		void read_number(value& val)
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
			
			val = negate ? -result : result;
		}
		
		void read_string(value& val)
		{
			val = read_raw_string();
		}
		
		std::string read_raw_string() // TODO: optimize?
		{
			std::ostringstream ss;
			++source;
			while (*source)
			{
				if (*source == '\\')
				{
					ss << read_escaped_symbol();
				}
				else if (*source == '"')
				{
					++source;
					return ss.str();
				}
				else
				{
					ss << *source++;
				}
			}
			throw fail("unexpected end of string");
		}
		
		char read_escaped_symbol()
		{
			++source;
			if (*source)
			{
				switch (*source++)
				{
					case '"':  return '"';
					case '/':  return '/';
					case '\\': return '\\';
					case 'b':  return '\b';
					case 'f':  return '\f';
					case 'n':  return '\n';
					case 'r':  return '\r';
					case 't':  return '\t';
					case 'u':  return read_unicode_character();
					default: throw fail("invalid escaped symbol");
				}
			}
			else
			{
				throw fail("unexpected end of escaped symbol");
			}
		}
		
		wchar_t hex_to_char(char hex)
		{
			switch (hex)
			{
				case '0': return 0;
				case '1': return 1;
				case '2': return 2;
				case '3': return 3;
				case '4': return 4;
				case '5': return 5;
				case '6': return 6;
				case '7': return 7;
				case '8': return 8;
				case '9': return 9;
				case 'a': case 'A': return 10;
				case 'b': case 'B': return 11;
				case 'c': case 'C': return 12;
				case 'd': case 'D': return 13;
				case 'e': case 'E': return 14;
				case 'f': case 'F': return 15;
				default: throw fail("invalid hex value");
			}
		}
		
		wchar_t read_unicode_character()
		{
			wchar_t result = 0;
			for (int i=0; i<4; ++i)
			{
				if (!*source) throw fail("unexpected unicode character end");
				char c = *source++;
				wchar_t wc = hex_to_char(c);
				result += wc << ((3 - i) * 4);
			}
			return result;
		}
	};

	// MARK: serializer
	class writer
	{
	public:

		writer(std::ostream& os) : m_os(os)
		{
			m_os << std::setprecision(17);
		}

		void write(value const& v, serialize_options const& options = serialize_options())
		{
			m_options = options;
			m_indents = 0;
			write_value(v);
		}

	private:

		writer(writer const&) = delete;
		writer(writer&&) = delete;
		writer& operator = (writer const&) = delete;
		writer& operator = (writer&&) = delete;

		std::ostream&     m_os;
		int               m_indents;
		serialize_options m_options;

		void write_string(std::string const* str)
		{
			if (!str) return;
			for (auto c : *str)
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
			//	case '\u': break; // TODO: ...
				default:   m_os << c;      break;
				}
			}
		}

		std::ostream& write_value(value const& v)
		{
			switch (v.type)
			{
			case value::type_null:
				m_os << "null";
				break;

			case value::type_number:
				m_os << v.data.n;
				break;

			case value::type_bool:
				m_os << (v.data.b ? "true" : "false");
				break;

			case value::type_string:
				m_os << '"';
				write_string(v.data.s);
				m_os << '"';
				break;

			case value::type_array:
				{
					m_os << '[';
					++m_indents;
					put_newline();
					
					if (v.data.a)
					{
						for (auto const& it : *v.data.a)
						{
							write_value(it);
							if (&it != &v.data.a->back())
							{
								m_os << ',';
								put_newline();
							}
						}
					}
					
					--m_indents;
					put_newline();
					m_os << ']';
				}
				break;

			case value::type_object:
				{
					m_os << '{';
					++m_indents;
					put_newline();
					
					for (std::size_t i=0; i<v.data.o->size(); ++i)
					{
						auto const& it = v.data.o->at(i);
						auto const& key = it.first;
						auto const& val = it.second;
						
						m_os << '"' << key << '"';
						put_space();
						m_os << ':';

						if (val.type == value::type_array || val.type == value::type_object) put_newline();
						else put_space();
						
						write_value(val);

						if (i != v.data.o->size() - 1)
						{
							m_os << ',';
							put_newline();
						}
					}

					--m_indents;
					put_newline();
					m_os << '}';
				}
				break;
			}
			return m_os;
		}

		void put_space()
		{
			if (m_options.pretty_print) m_os << ' ';
		}

		void put_newline()
		{
			if (m_options.pretty_print)
			{
				m_os << '\n';
				for (int i = 0; i < m_indents; ++i) m_os << m_options.tab_character;
			}
		}
	};
};

// MARK: type conversion specializations
template <> inline char const* value::as<char const*>() const
{
	return type == type_string ? data.s->c_str() : "";
}

template <> inline std::string value::as<std::string>() const
{
	return as<char const*>();
}

template <> inline std::string const& value::as<std::string const&>() const
{
	static std::string empty;
	return type == type_string ? *data.s : empty;
}

template <> inline bool value::as<bool>() const
{
	switch (type)
	{
	case type_null:   return  false;
	case type_bool:   return  data.b;
	case type_number: return  data.n != 0;
	case type_string: return !data.s->empty();
	case type_array:  return !data.a->empty();
	case type_object: return !data.o->empty();
	}
}

// MARK: helpers
inline std::ostream& operator << (std::ostream& ss, value const& val)
{
	switch (val.type)
	{
		case value::type_null:   ss << "null";                break;
		case value::type_bool:   ss << val.as<bool>();        break;
		case value::type_number: ss << val.as<double>();      break;
		case value::type_string: ss << val.as<std::string>(); break;
		case value::type_array:  ss << "array";               break;
		case value::type_object: ss << "object";              break;
	}
	return ss;
}

}

#undef NOEXCEPT
