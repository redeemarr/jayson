#ifndef JAYSON_H
#define JAYSON_H

// rev. 4

#define JAYSON_UNICODE 1

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
	
	class value;
	
	typedef char type_t;
	
	static type_t const type_null    = ' ';
	static type_t const type_number  = 'n';
	static type_t const type_boolean = 'b';
	static type_t const type_string  = 's';
	static type_t const type_array   = 'a';
	static type_t const type_object  = 'o';
	
	typedef double                                 number_t;
	typedef bool                                   boolean_t;
#if JAYSON_UNICODE == 1
	typedef std::wstring                           string_t;
#else
	typedef std::string                            string_t;
#endif
	typedef std::vector<value>                     array_t;
	typedef std::unordered_map<std::string, value> object_t;
	
	class value
	{
	public:
		
		value() : m_type(type_null) {}
		value(type_t type) : m_type(type) { create(); }
		value(value const& v) : m_type(type_null) { *this = v; }
		value(value&& v) NOEXCEPT : m_type(type_null) { *this = std::move(v); }
		
		value(number_t v) : m_type(type_number)           { create();  data.n = v; }
		value(boolean_t v) : m_type(type_boolean)         { create();  data.b = v; }
		value(string_t const& v) : m_type(type_string)    { create(); *data.s = v; }
		value(array_t const& v) : m_type(type_array)      { create(); *data.a = v; }
		value(object_t const& v) : m_type(type_object)    { create(); *data.o = v; }
		
#if JAYSON_UNICODE == 1
		value(wchar_t const* s) : m_type(type_string)     { create(); *data.s = s; }
#else
		value(char const* s) : m_type(type_string)        { create(); *data.s = s; }
#endif
		value(int n) : m_type(type_number)                { create();  data.n = static_cast<number_t>(n); }
		value(unsigned int n) : m_type(type_number)       { create();  data.n = static_cast<number_t>(n); }
		value(long n) : m_type(type_number)               { create();  data.n = static_cast<number_t>(n); }
		value(unsigned long n) : m_type(type_number)      { create();  data.n = static_cast<number_t>(n); }
		value(long long n) : m_type(type_number)          { create();  data.n = static_cast<number_t>(n); }
		value(unsigned long long n) : m_type(type_number) { create();  data.n = static_cast<number_t>(n); }
		
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
		
		value& operator = (value&& v)
		{
			std::swap(m_type, v.m_type);
			std::swap(data, v.data);
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
	
	template <> inline number_t&  value::ref<number_t>()  { return  data.n; }
	template <> inline boolean_t& value::ref<boolean_t>() { return  data.b; }
	template <> inline string_t&  value::ref<string_t>()  { return *data.s; }
	template <> inline array_t&   value::ref<array_t>()   { return *data.a; }
	template <> inline object_t&  value::ref<object_t>()  { return *data.o; }
	
	template <> inline number_t  const& value::ref_const<number_t>()  const { return  data.n; }
	template <> inline boolean_t const& value::ref_const<boolean_t>() const { return  data.b; }
	template <> inline string_t  const& value::ref_const<string_t>()  const { return *data.s; }
	template <> inline array_t   const& value::ref_const<array_t>()   const { return *data.a; }
	template <> inline object_t  const& value::ref_const<object_t>()  const { return *data.o; }
	
	template <> inline type_t value::get_type<number_t>()  const { return type_number;  }
	template <> inline type_t value::get_type<boolean_t>() const { return type_boolean; }
	template <> inline type_t value::get_type<string_t>()  const { return type_string;  }
	template <> inline type_t value::get_type<array_t>()   const { return type_array;   }
	template <> inline type_t value::get_type<object_t>()  const { return type_object;  }
	
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
			m_os << std::setprecision(20);
		}
		
		void write(value const& v, options const& options = writer::options())
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
		
		void write_string(string_t const& str)
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
		
		std::ostream& write_value(value const& v)
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
					for (auto const& elem : a)
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
					for (auto const& it : o)
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
	
	inline void to_file(char const* filename, value const& v, writer::options const& options = writer::options())
	{
		std::ofstream ofs(filename, std::ios::binary);
		writer w(ofs);
		w.write(v, options);
	}
	
	inline std::string to_string(value const& v, writer::options const& options = writer::options())
	{
		std::ostringstream oss;
		writer w(oss);
		w.write(v, options);
		return oss.str();
	}
	
	class reader
	{
	public:
		
		reader(std::string const& str) : m_str(str)
		{
		}
		
		bool read(value& result)
		{
			log_str.clear();
			if (!m_str.empty())
			{
				src = m_str.c_str();
				return read_value(result);
			}
			else
			{
				result.type(type_null);
				return false;
			}
		}
		
		std::string const& get_log() const { return log_str; }
		
	private:
		
		std::string const& m_str;
		char const* src;
		std::string   log_str;
		
		reader(reader&);
		reader& operator = (reader&);
		
		void fail(char const* why)
		{
			char const* begin = m_str.c_str();
			char const* start_line = src;
			char const* end_line = src;
			while (start_line != begin && *start_line != '\n') --start_line;
			while (end_line != begin && *end_line != '\n' && *end_line != '\0') ++end_line;
			log_str = why;
			log_str += ": ";
			log_str.insert(log_str.end(), start_line, src);
			log_str += "<?>";
			log_str.insert(log_str.end(), src, end_line);
		}
		
		inline bool skip_whitespaces()
		{
			while (*src != '\0')
			{
				if (*src != ' ' && *src != '\t' && *src != '\r' && *src != '\n')
				{
					return true;
				}
				else
				{
					++src;
				}
			}
			
			fail("unexpected end of document");
			return false;
		}
		
		bool read_value(value& result)
		{
			if (!skip_whitespaces()) return false;
			switch (*src)
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
			++src;
			while (*src != '\0')
			{
				if (!skip_whitespaces()) return false;
				if (*src == '"')
				{
					std::pair<std::string, value> pair;
					read_ascii_string(pair.first);
					
					object_t& o = val.object();
					auto it = o.find(pair.first);
					if (it == o.end())
					{
						it = o.insert(std::move(pair)).first;
					}
					
					if (!skip_whitespaces()) return false;
					if (*src++ == ':')
					{
						if (!read_value(it->second)) return false;
						if (!skip_whitespaces()) return false;
						
						switch (*src++)
						{
							case ',': break;
							case '}': return true;
							default:
								fail("expected ',' or '}' after key-value pair");
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
			++src;
			while (*src != '\0')
			{
				array_t& arr = val.array();
				arr.emplace_back();
				if (!read_value(arr.back())) return false;
				if (!skip_whitespaces()) return false;
				
				switch (*src++)
				{
					case ',': break;
					case ']': return true;
					default:
						fail("expected ',' or ']' after array element");
						return false;
				}
			}
			fail("unexpected end of array");
			return false;
		}
		
		inline bool read_null(value& val)
		{
			if (memcmp(src, "null", 4) == 0)
			{
				src += 4;
				val.type(type_null);
				return true;
			}
			else
			{
				fail("expected 'null'");
				return false;
			}
		}
		
		inline bool read_true(value& val)
		{
			if (memcmp(src, "true", 4) == 0)
			{
				src += 4;
				val.boolean(true);
				return true;
			}
			else
			{
				fail("expected 'true'");
				return false;
			}
		}
		
		inline bool read_false(value& val)
		{
			if (memcmp(src, "false", 5) == 0)
			{
				src += 5;
				val.boolean(false);
				return true;
			}
			else
			{
				fail("expected 'false'");
				return false;
			}
		}
		
		inline bool read_number(value& val)
		{
			bool negate;
			auto c = *src;
			if      (c == '-')             { negate = true;  ++src; }
			else if (c == '+')             { negate = false; ++src; }
			else if (c >= '0' && c <= '9') { negate = false; }
			else { fail("invalid number value"); return false; }
			
			number_t result = 0;
			while (*src != '\0' && *src != '.' && *src != 'e' && *src != 'E' && *src >= '0' && *src <= '9')
			{
				result = result * 10 + (*src - '0');
				++src;
			}
			
			if (*src != '\0' && *src == '.')
			{
				++src;
				number_t mult = 0.1f;
				while (*src != '\0' && *src != 'e' && *src != 'E' && *src >= '0' && *src <= '9')
				{
					result += (*src - '0') * mult;
					mult *= 0.1f;
					++src;
				}
			}
			
			if (*src != '\0' && (*src == 'e' || *src == 'E'))
			{
				++src;
				number_t exp_mult;
				if      (*src == '-') { exp_mult = 0.1f; ++src; }
				else if (*src == '+') { exp_mult = 10;   ++src; }
				else                  { exp_mult = 10; }
				
				std::size_t exp = 0;
				while (*src != '\0' && *src >= '0' && *src <= '9')
				{
					exp = exp * 10 + (*src - '0');
					++src;
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
		
		inline bool read_ascii_string(std::string& result)
		{
			++src;
			while (*src != '\0')
			{
				if (*src == '"')
				{
					++src;
					return true;
				}
				else
				{
					result += *src++;
				}
			}
			
			fail("unexpected end of string");
			return false;
		}
		
		inline bool read_raw_string(string_t& result)
		{
			++src;
			while (*src != '\0')
			{
				if (*src == '\\')
				{
					if (!read_escaped_symbol(result)) return false;
				}
				else if (*src == '"')
				{
					++src;
					return true;
				}
				else
				{
					result += *src++;
				}
			}
			
			fail("unexpected end of string");
			return false;
		}
		
		inline bool read_escaped_symbol(string_t& to)
		{
			++src;
			if (*src != '\0')
			{
				switch (*src)
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
						if (!read_unicode_character(to)) return false;
						break;
						
					default:
						fail("invalid escaped symbol");
						return false;
				}
				++src;
			}
			else
			{
				fail("unexpected end of escaped symbol");
				return false;
			}
			return true;
		}
		
		inline bool read_unicode_character(string_t& to)
		{
			++src;
			if (*src != '\0')
			{
				wchar_t wc = 0;
				for (int i=0; i<4; ++i)
				{
					if (*src != '\0')
					{
						uint32_t c;
						switch (src[i])
						{
							case '0': c = 0; break;
							case '1': c = 1; break;
							case '2': c = 2; break;
							case '3': c = 3; break;
							case '4': c = 4; break;
							case '5': c = 5; break;
							case '6': c = 6; break;
							case '7': c = 7; break;
							case '8': c = 8; break;
							case '9': c = 9; break;
							case 'a': case 'A': c = 10; break;
							case 'b': case 'B': c = 11; break;
							case 'c': case 'C': c = 12; break;
							case 'd': case 'D': c = 13; break;
							case 'e': case 'E': c = 14; break;
							case 'f': case 'F': c = 15; break;
							default:
								fail("expected valid unicode hex character");
								return false;
						}
						wc += c << ((3 - i) * 4);
					}
					else
					{
						fail("expected valid unicode hex character");
						return false;
					}
				}
				src += 3;
				to += wc;
				return true;
			}
			else
			{
				fail("expected valid unicode hex character");
				return false;
			}
		}
	};
	
	inline std::streamsize file_size(std::ifstream& file)
	{
		auto pos = file.tellg();
		file.seekg(0, std::ios::end);
		auto size = file.tellg();
		file.seekg(0, std::ios::beg);
		size -= file.tellg();
		file.seekg(pos, std::ios::beg);
		return size;
	}
	
	inline bool load_text(char const* filename, std::string& dst)
	{
		std::ifstream ifs(filename);
		if (ifs)
		{
			auto size = file_size(ifs);
			if (size > 0)
			{
				dst.resize(size);
				ifs.read(&*dst.begin(), size);
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	
	inline bool from_file(char const* filename, value& val)
	{
		std::string buf;
		if (load_text(filename, buf))
		{
			reader r(buf);
			return r.read(val);
		}
		else
		{
			return false;
		}
	}
	
	inline bool from_file(char const* filename, value& val, std::string& log)
	{
		std::string buf;
		if (load_text(filename, buf))
		{
			reader r(buf);
			bool ok = r.read(val);
			if (!ok) log = r.get_log();
			return ok;
		}
		else
		{
			log = "failed to load file '" + std::string(filename) + "'";
			return false;
		}
	}
	
	inline bool from_string(value& result, std::string const& str)
	{
		reader r(str);
		return r.read(result);
	}
	
	inline bool from_string(value& result, std::string const& str, std::string& log)
	{
		reader r(str);
		bool ok = r.read(result);
		if (!ok) log = r.get_log();
		return ok;
	}
	
};

#endif
