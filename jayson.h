#pragma once

#ifndef _MSC_VER
#define NOEXCEPT noexcept
#else
#define NOEXCEPT
#endif

#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>

namespace json
{
	class value
	{
	private:
	
		template <typename T> struct list
		{
			struct node
			{
				node* next;
				char* key;
				T     val;
			};
		
			node* head;
			node* tail;

			node* push()
			{
				node* n = new node();
				n->next = nullptr;
				n->key = nullptr;
				
				if (!tail)
				{
					head = tail = n;
				}
				else
				{
					tail->next = n;
					tail = n;
				}
				return n;
			}
		};
		
		typedef char type_t;
		typedef list<value> list_t;

		union
		{
			double n;
			bool   b;
			char*  s;
			list_t a;
		} data;
		type_t type;
		
		value(type_t type) : type(type)
		{
			switch (type)
			{
			case 's':
				data.s = nullptr;
				break;
				
			case 'a': case 'o':
				data.a.head = data.a.tail = nullptr;
				break;
				
			default:
				break;
			}
		}
		
		char* new_string(char const* s)
		{
			if (s)
			{
				size_t len = strlen(s);
				char* r = new char[len + 1];
				memcpy(r, s, len + 1);
				return r;
			}
			else return nullptr;
		}

		friend class reader;
		friend class writer;
		friend class tests;

	public:
		
		~value()
		{
			switch (type)
			{
			case 's':
				delete[] data.s;
				break;

			case 'a': case 'o':
				auto head = data.a.head;
				while (head)
				{
					auto next = head->next;
					delete[] head->key;
					delete head;
					head = next;
				}
				break;
			}
			type = 0;
		}

		value() : value(' ') {}
		
		value(value const& v) : value(v.type)
		{
			switch (type)
			{
				case 's':
					data.s = new_string(v.data.s);
					break;
					
				case 'a': case 'o':
					for (auto n = v.data.a.head; n; n = n->next)
					{
						auto nn = data.a.push();
						nn->key = new_string(n->key);
						nn->val = n->val;
					}
					break;
					
				case ' ':
					break;
					
				default:
					data = v.data;
					break;
			}
		}
		
		value(value&& v) NOEXCEPT : value(' ')
		{
			std::swap(type, v.type);
			std::swap(data, v.data);
		}
		
		value(std::initializer_list<value> const& list) : value('a')
		{
			// TODO: preallocate?
			for (value const& val : list)
			{
				auto node = data.a.push();
				node->key = nullptr;
				node->val = val; // XXX: copying
			}
		}
		
		value(char const* v) : value('s')        { data.s = new_string(v); }
		value(std::string const& v) : value('s') { data.s = new_string(v.c_str()); }
		value(bool v) : value('b') { data.b = v; }
		// TODO: char/uchar constructors?
		value(short n)              : value('n') { data.n = n; }
		value(unsigned short n)     : value('n') { data.n = n; }
		value(int n)                : value('n') { data.n = n; }
		value(unsigned int n)       : value('n') { data.n = n; }
		value(long n)               : value('n') { data.n = n; }
		value(unsigned long n)      : value('n') { data.n = n; }
		value(long long n)          : value('n') { data.n = n; }
		value(unsigned long long n) : value('n') { data.n = n; }
		value(float n)              : value('n') { data.n = n; }
		value(double n)             : value('n') { data.n = n; }

		operator bool () const
		{
			switch (type)
			{
			case 'b': return data.b;
			case 's': return data.s != nullptr;
			case 'n': return data.n != 0;
			case 'a': case 'o': return true;
			case ' ': default: return false;
			}
		}
		
		operator char const* () const { return type == 's' ? data.s : ""; }
		operator std::string () { return type == 's' && data.s ? std::string(data.s) : std::string(); }
		operator std::string () const { return type == 's' && data.s ? std::string(data.s) : std::string(); }
		// TODO: char/uchar operators?
		operator short () const              { return type == 'n' ? data.n : 0; }
		operator unsigned short () const     { return type == 'n' ? data.n : 0; }
		operator int () const                { return type == 'n' ? data.n : 0; }
		operator unsigned int () const       { return type == 'n' ? data.n : 0; }
		operator long () const               { return type == 'n' ? data.n : 0; }
		operator unsigned long () const      { return type == 'n' ? data.n : 0; }
		operator long long () const          { return type == 'n' ? data.n : 0; }
		operator unsigned long long () const { return type == 'n' ? data.n : 0; }
		operator float () const              { return type == 'n' ? data.n : 0; }
		operator double () const             { return type == 'n' ? data.n : 0; }

		bool operator == (bool b) const               { return static_cast<bool>(*this) == b; }
		bool operator == (char const* s) const        { return type == 's' ? strcmp(data.s, s) == 0 : false; }
		bool operator == (std::string const& s) const { return *this == s.c_str(); }
		bool operator == (unsigned short n) const     { return type == 'n' ? data.n == n : false; }
		bool operator == (int n) const                { return type == 'n' ? data.n == n : false; }
		bool operator == (unsigned int n) const       { return type == 'n' ? data.n == n : false; }
		bool operator == (long n) const               { return type == 'n' ? data.n == n : false; }
		bool operator == (unsigned long n) const      { return type == 'n' ? data.n == n : false; }
		bool operator == (long long n) const          { return type == 'n' ? data.n == n : false; }
		bool operator == (unsigned long long n) const { return type == 'n' ? data.n == n : false; }
		bool operator == (float n) const              { return type == 'n' ? data.n == n : false; }
		bool operator == (double n) const             { return type == 'n' ? data.n == n : false; }

		bool operator != (bool b) const               { return !(*this == b); }
		bool operator != (char const* s) const        { return !(*this == s); }
		bool operator != (std::string const& s) const { return !(*this == s); }
		bool operator != (unsigned short n) const     { return !(*this == n); }
		bool operator != (int n) const                { return !(*this == n); }
		bool operator != (unsigned int n) const       { return !(*this == n); }
		bool operator != (long n) const               { return !(*this == n); }
		bool operator != (unsigned long n) const      { return !(*this == n); }
		bool operator != (long long n) const          { return !(*this == n); }
		bool operator != (unsigned long long n) const { return !(*this == n); }
		bool operator != (float n) const              { return !(*this == n); }
		bool operator != (double n) const             { return !(*this == n); }

		bool operator < (char const* s) const        { return type == 's' ? strcmp(data.s, s) < 0 : false; }
		bool operator < (std::string const& s) const { return *this < s.c_str(); }
		bool operator < (unsigned short n) const     { return type == 'n' ? data.n < n : false; }
		bool operator < (int n) const                { return type == 'n' ? data.n < n : false; }
		bool operator < (unsigned int n) const       { return type == 'n' ? data.n < n : false; }
		bool operator < (long n) const               { return type == 'n' ? data.n < n : false; }
		bool operator < (unsigned long n) const      { return type == 'n' ? data.n < n : false; }
		bool operator < (long long n) const          { return type == 'n' ? data.n < n : false; }
		bool operator < (unsigned long long n) const { return type == 'n' ? data.n < n : false; }
		bool operator < (float n) const              { return type == 'n' ? data.n < n : false; }
		bool operator < (double n) const             { return type == 'n' ? data.n < n : false; }

		bool operator > (char const* s) const        { return type == 's' ? strcmp(data.s, s) > 0 : false; }
		bool operator > (std::string const& s) const { return *this > s.c_str(); }
		bool operator > (unsigned short n) const     { return type == 'n' ? data.n > n : false; }
		bool operator > (int n) const                { return type == 'n' ? data.n > n : false; }
		bool operator > (unsigned int n) const       { return type == 'n' ? data.n > n : false; }
		bool operator > (long n) const               { return type == 'n' ? data.n > n : false; }
		bool operator > (unsigned long n) const      { return type == 'n' ? data.n > n : false; }
		bool operator > (long long n) const          { return type == 'n' ? data.n > n : false; }
		bool operator > (unsigned long long n) const { return type == 'n' ? data.n > n : false; }
		bool operator > (float n) const              { return type == 'n' ? data.n > n : false; }
		bool operator > (double n) const             { return type == 'n' ? data.n > n : false; }

		bool operator <= (char const* s) const        { return type == 's' ? strcmp(data.s, s) <= 0 : false; }
		bool operator <= (std::string const& s) const { return *this <= s.c_str(); }
		bool operator <= (unsigned short n) const     { return type == 'n' ? data.n <= n : false; }
		bool operator <= (int n) const                { return type == 'n' ? data.n <= n : false; }
		bool operator <= (unsigned int n) const       { return type == 'n' ? data.n <= n : false; }
		bool operator <= (long n) const               { return type == 'n' ? data.n <= n : false; }
		bool operator <= (unsigned long n) const      { return type == 'n' ? data.n <= n : false; }
		bool operator <= (long long n) const          { return type == 'n' ? data.n <= n : false; }
		bool operator <= (unsigned long long n) const { return type == 'n' ? data.n <= n : false; }
		bool operator <= (float n) const              { return type == 'n' ? data.n <= n : false; }
		bool operator <= (double n) const             { return type == 'n' ? data.n <= n : false; }

		bool operator >= (char const* s) const        { return type == 's' ? strcmp(data.s, s) >= 0 : false; }
		bool operator >= (std::string const& s) const { return *this >= s.c_str(); }
		bool operator >= (unsigned short n) const     { return type == 'n' ? data.n >= n : false; }
		bool operator >= (int n) const                { return type == 'n' ? data.n >= n : false; }
		bool operator >= (unsigned int n) const       { return type == 'n' ? data.n >= n : false; }
		bool operator >= (long n) const               { return type == 'n' ? data.n >= n : false; }
		bool operator >= (unsigned long n) const      { return type == 'n' ? data.n >= n : false; }
		bool operator >= (long long n) const          { return type == 'n' ? data.n >= n : false; }
		bool operator >= (unsigned long long n) const { return type == 'n' ? data.n >= n : false; }
		bool operator >= (float n) const              { return type == 'n' ? data.n >= n : false; }
		bool operator >= (double n) const             { return type == 'n' ? data.n >= n : false; }

		value& operator = (value const& v)
		{
			this->~value(); new (this) value(v);
			return *this;
		}
		
		value& operator = (value&& v)
		{
			std::swap(type, v.type);
			std::swap(data, v.data);
			return *this;
		}
		
		value const& empty_value() const
		{
			static value val;
			return val;
		}

		std::size_t size() const
		{
			std::size_t r = 0;
			if (type == 'a' || type == 'o')
			{
				for (auto n = data.a.head; n; n = n->next) ++r;
			}
			return r;
		}
		
		value const& operator () (char const* key) const
		{
			if (type == 'o')
			{
				for (auto n = data.a.head; n; n = n->next)
				{
					if (strcmp(key, n->key) == 0) return n->val;
				}
				return empty_value();
			}
			else return empty_value();
		}
		
		value& operator () (char const* key)
		{
			if (type != 'o')
			{
				this->~value(); new (this) value('o');
			}
			
			// TODO: optimize
			for (auto n = data.a.head; n; n = n->next)
			{
				if (strcmp(key, n->key) == 0) return n->val;
			}
			
			auto n = data.a.push();
			n->key = new_string(key);
			return n->val;
		}

		value& add(value const& v)
		{
			if (type != 'a')
			{
				this->~value(); new (this) value('a');
			}
			data.a.push()->val = v;
			return *this;
		}
		
		value const& operator [] (std::size_t index) const
		{
			if (type == 'a')
			{
				// TODO: optimize
				std::size_t i = 0;
				for (auto n = data.a.head; n; n = n->next)
				{
					if (i++ == index) return n->val;
				}
				return empty_value();
			}
			else return empty_value();
		}
	};

	class reader
	{
	public:
		
		bool parse(char const* string, value& result, std::string* errors)
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

		class stringbuf
		{
		public:
		
			stringbuf() : capacity(0), data(nullptr) { init(); }
			~stringbuf() { if (data) free(data); }
			
			void init() { size = 0; }
			
			char* emit()
			{
				char* result = new char[size];
				memcpy(result, data, size);
				return result;
			}
			
			inline void operator << (char c)
			{
				if (size == capacity)
				{
					capacity = size + 256;
					data = (char*)realloc(data, capacity);
				}
				data[size++] = c;
			}
		
		private:
		
			char*  data;
			size_t capacity;
			size_t size;
		};
		
		char const* source;
		stringbuf   strbuf;
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
			val = value('o');
			++source;
			while (*source)
			{
				auto node = val.data.a.push();
				skip_whitespaces();
				if (*source == '"')
				{
					node->key = read_raw_string();
					skip_whitespaces();
					if (*source++ == ':')
					{
						skip_whitespaces();
						read_value(node->val);
						skip_whitespaces();
						
						char c = *source++;
						if (c == '}') return;
						if (c != ',') throw fail("expected ',' or '}' after key-value pair");
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
			val = value('a');
			++source;
			while (*source)
			{
				skip_whitespaces();
				if (*source == ']')
				{
					++source;
					return;
				}
				
				if (val.data.a.head)
				{
					if (*source == ',') ++source;
					else throw fail("unexpected end of array");
				}
				
				read_value(val.data.a.push()->val);
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
				else                    { exp_mult = 10; }
				
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
			val = value('s');
			val.data.s = read_raw_string();
		}
		
		char* read_raw_string() // XXX: string allocated
		{
			strbuf.init();
			++source;
			while (*source)
			{
				if (*source == '\\')
				{
					strbuf << read_escaped_symbol();
				}
				else if (*source == '"')
				{
					++source;
					strbuf << '\0';
					return strbuf.emit();
				}
				else
				{
					strbuf << *source++;
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
					case 'u': return read_unicode_character();
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

		writer(writer const&) = delete;
		writer(writer&&) = delete;
		writer& operator = (writer const&) = delete;
		writer& operator = (writer&&) = delete;

		std::ostream& m_os;
		int           m_indents;
		options       m_options;

		void write_string(char* str)
		{
			if (!str) return;
			while (*str)
			{
				char c = *str++;
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
				default:
					m_os << c;
					break;
				}
			}
		}

		std::ostream& write_value(value const& v)
		{
			switch (v.type)
			{
			case ' ':
				m_os << "null";
				break;

			case 'n':
				m_os << v.data.n;
				break;

			case 'b':
				m_os << (v.data.b ? "true" : "false");
				break;

			case 's':
				m_os << "\"";
				write_string(v.data.s);
				m_os << "\"";
				break;

			case 'a':
				{
					put_newline();
					m_os << "[";
					++m_indents;
					put_newline();
					
					for (auto node = v.data.a.head; node; node = node->next)
					{
						write_value(node->val);
						if (node->next)
						{
							m_os << ",";
							put_newline();
						//	put_space();
						}
					}
					
					--m_indents;
					put_newline();
					m_os << "]";
				}
				break;

			case 'o':
				{
					put_newline();
					m_os << "{";
					++m_indents;
					put_newline();

					for (auto node = v.data.a.head; node; node = node->next)
					{
						m_os << "\"" << (node->key ? node->key : "") << "\"";
						put_space();
						m_os << ':';
						put_space();
						write_value(node->val);
						if (node->next)
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

	inline bool from_string(char const* str, value& result, std::string* errors = nullptr)
	{
		reader r;
		return r.parse(str, result, errors);
	}

	inline bool from_file(char const* filename, value& result, std::string* errors = nullptr)
	{
		std::ifstream stream(filename);
		if (stream)
		{
			stream.seekg(0, std::ios::end);
			auto sz = stream.tellg();
			stream.seekg(0, std::ios::beg);
			char* text = new char[sz];
			stream.read(text, sz);
			bool status = from_string(text, result, errors);
			delete[] text;
			return status;
		}
		else
		{
			if (errors) *errors = "failed to load file '" + std::string(filename) + "'";
			return false;
		}
	}

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

};

#undef NOEXCEPT
