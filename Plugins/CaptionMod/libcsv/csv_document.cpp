#include "csv_document.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

extern IFileSystem *g_pFullFileSystem;

namespace CSV
{

	CSVDocument::row_index_type CSVDocument::load_file( const char * file_path )
	{
		// why use a separate parser? 
		// Because parser uses some local variables, if we use a separate parser, all these 
		// variables are de-constructed after loading while parsed data is still valid.
		CSVParser parser;
		return parser.parse(this, file_path);
	}

	const CSVDocument::document_type& CSVDocument::get_document() const
	{
		return m_doc;
	}

	const CSVDocument::row_type& CSVDocument::get_row( row_index_type row ) const
	{
		_check_row_index(row);

		for (document_type::const_iterator itr = m_doc.begin(); itr != m_doc.end(); ++itr)
		{
			if (row-- == 0)
			{
				return *itr;
			}
		}

		// never reach here, but in order to make compiler happy, we add a return statement here.
		return m_doc.front();
	}

	const CSVDocument::element_type& CSVDocument::get_element( row_index_type row, column_index_type col ) const
	{
		_check_row_index(row);
		_check_col_index(col);

		for (document_type::const_iterator itr = m_doc.begin(); itr != m_doc.end(); ++itr)
		{
			if (row-- == 0)
			{
				const row_type& fields = *itr;
				return fields[col];
			}
		}

		// never reach here, but in order to make compiler happy, we add a return statement here.
		return m_doc.front().front();
	}

	void CSVDocument::merge_document( const document_type& doc )
	{
		for (document_type::const_iterator itr = doc.begin(); itr != doc.end(); ++itr)
		{
			m_doc.push_back(*itr);
		}
	}

	void CSVDocument::add_row( const row_type& row )
	{
		m_doc.push_back(row);
	}

	void CSVDocument::remove_row( row_index_type row_idx )
	{
		_check_row_index(row_idx);

		document_type::iterator itr = m_doc.begin();
		std::advance(itr, row_idx);
		m_doc.erase(itr);
	}

	void CSVDocument::replace_row( row_index_type row_idx, const row_type& row )
	{
		_check_row_index(row_idx);

		document_type::iterator itr = m_doc.begin();
		std::advance(itr, row_idx);
		m_doc.erase(itr);

		itr = m_doc.begin();
		std::advance(itr, row_idx - 1);
		m_doc.insert(itr, row);
	}

	void CSVDocument::update_elem( row_index_type row, column_index_type col, const element_type& new_val )
	{
		_check_row_index(row);
		_check_col_index(col);

		document_type::iterator itr = m_doc.begin();
		std::advance(itr, row);
		row_type& update_row = *itr;
		update_row[col] = new_val;
	}

	void CSVDocument::clear()
	{
		m_doc.clear();
	}

	CSVDocument::row_index_type CSVDocument::size() const
	{
		return m_doc.size();
	}

	CSVDocument::iterator CSVDocument::begin()
	{
		return m_doc.begin();
	}

	CSVDocument::iterator CSVDocument::end()
	{
		return m_doc.end();
	}

	CSVDocument::row_type& CSVDocument::operator[]( row_index_type idx )
	{
		document_type::iterator itr = m_doc.begin();
		std::advance(itr, idx);
		return *itr;
	}

	int CSVDocument::_replace_all( std::string &field, const std::string& old_str, const std::string& new_str )
	{
		std::string::size_type quote_pos = 0;
		int replace_count = 0;
		while ((quote_pos = field.find(old_str, quote_pos)) != std::string::npos)
		{
			field.replace(quote_pos, old_str.size(), new_str);
			quote_pos += new_str.size();
			++replace_count;
		}

		return replace_count;
	}

	void CSVDocument::_write_optional_enclosure_field( std::ostream& out_stream, const element_type& elem, bool last_elem )
	{
		if (elem.find("\"") != std::string::npos)
		{
			std::string new_elem = elem;
			_replace_all(new_elem, "\"", "\"\"");
			out_stream << "\"" << new_elem << "\"";
		}
		else if (elem.find(",") != std::string::npos || elem.find("\n") != std::string::npos)
		{
			out_stream << "\"" << elem << "\"";
		}
		else
		{
			out_stream << elem;
		}

		out_stream << (last_elem ? "\n" : ",");
	}

	void CSVDocument::_write_complete_enclosure_field( std::ostream& out_stream, const element_type& elem, bool last_elem )
	{
		if (elem.find("\"") != std::string::npos)
		{
			std::string new_elem = elem;
			_replace_all(new_elem, "\"", "\"\"");
			out_stream << "\"" << new_elem << "\"";
		}
		else
		{
			out_stream << "\"" << elem << "\"";
		}
		out_stream << (last_elem ? "\n" : ",");
	}


	void CSVDocument::_check_row_index( row_index_type row_idx ) const
	{
		if (row_idx >= m_doc.size())
		{
			std::ostringstream str_stream;
			str_stream << "row index " << row_idx << " is out of range. (max: " << m_doc.size() - 1 << ")";
			throw std::out_of_range(str_stream.str());
		}
	}

	void CSVDocument::_check_col_index( column_index_type col ) const
	{
		if (m_doc.front().size() <= col)
		{
			std::ostringstream str_stream;
			str_stream << "column index " << col << " is out of range. (max: " << m_doc.front().size() - 1 << ")";
			throw std::out_of_range(str_stream.str());
		}
	}

	CSVDocument::row_index_type CSVDocument::row_count() const
	{
		return size();
	}

	CSVDocument::column_index_type CSVDocument::col_count() const
	{
		if (m_doc.size() > 0)
		{
			return m_doc.front().size();
		}
		return 0;
	}

	CSVParser::CSVParser()
	{
		idx = field_beg = field_end = 0;
		row_count = col_count = 0;
		state = LineEnd;
	}

	CSVDocument::row_index_type CSVParser::parse( CSVDocument* p_doc, const char * file_path )
	{
		_initialize(p_doc, file_path);
		for (; state != ParseCompleted; _next())
		{
			// I know design pattern, but I don't want to make things more complicated.
			switch(state)
			{
			case LineStart:
				_post_line_start();
				break;
			case FieldStart:
				_post_field_start();
				break;
			case FrontQuote:
				_post_front_quote();
				break;
			case EscapeOn:
				_post_escape_on();
				break;
			case EscapeOff:
				_post_escape_off();
				break;
			case BackQuote:
				_post_back_quote();
				break;
			case FieldEnd:
				_post_field_end();
				break;
			case LineEnd:
				_post_line_end();
				break;
			}
		}

		if (csv_file)
		{
			g_pFullFileSystem->Close(csv_file);
			csv_file = NULL;
		}
		return row_count;
	}

	char CSVParser::_curr_char() const
	{
		return row_str[idx];
	}

	void CSVParser::_next()
	{
		++idx;
	}

	void CSVParser::_post_line_start()
	{
	}

	void CSVParser::_post_field_start()
	{
		if (_curr_char() == ',')
		{
			field_end = idx;
			_field_end();
		}
		else if (_curr_char() == '\n')
		{
			field_end = idx;
			_field_end();
			_line_end();
		}
		else if (_curr_char() == '\"')
		{
			if (field_beg != idx)
			{
				throw std::runtime_error("Syntax error: quote symbol in unenclosed field.");
			}
		}
	}

	void CSVParser::_post_front_quote()
	{
		if (_curr_char() == '"')
		{
			_escape_on();
		}
		else if (_curr_char() == '\n')
		{
			_append_another_line_from_file();
		}
	}

	void CSVParser::_post_escape_on()
	{
		if (_curr_char() == ',')
		{
			_back_quote();
			_field_end();
		}
		else if (_curr_char() == '"')
		{
			_escape_off();
		}
		else if (_curr_char() == '\n')
		{
			_back_quote();
			_field_end();
			_line_end();
		}
		else
		{
			throw std::runtime_error("Syntax error: quote symbol in front of character which is not quote symbol.");
		}
	}

	void CSVParser::_post_escape_off()
	{
		if (_curr_char() == '"')
		{
			_escape_on();
		}
		else if (_curr_char() == '\n')
		{
			_append_another_line_from_file();
		}
	}

	void CSVParser::_post_back_quote()
	{
	}

	void CSVParser::_post_field_end()
	{
		_field_start();

		if (_curr_char() == '"')
		{
			_front_quote();
		}
		else if (_curr_char() == ',')
		{
			_field_end();
		}
		else if (_curr_char() == '\n')
		{
			_field_end();
			_line_end();
		}
	}

	void CSVParser::_post_line_end()
	{
		_line_start();
		_field_start();

		if (_curr_char() == '"')
		{
			_front_quote();
		}
		else if (_curr_char() == ',')
		{
			_field_end();
		}
		else if (_curr_char() == '\n')
		{
			throw std::runtime_error("Syntax error: empty line is not allowed.");
		}
	}

	//Filesystem Related

	void CSVParser::_open_csv_file( const char *file_path )
	{
		csv_file = g_pFullFileSystem->Open(file_path, "rb");
		if (csv_file == FILESYSTEM_INVALID_HANDLE)
		{
			std::ostringstream str_stream;
			str_stream << "Failed to open file " << file_path << ".";
			throw std::runtime_error(str_stream.str());
		}
	}

	bool CSVParser::_get_line_from_file()
	{
		unsigned char c = '\0';
		int numCharRead = 0;
		bool bEndOfFile = false;
		char append[2];
		append[1] = '\0';

		//clear the string
		read_str = "";

		while(!bEndOfFile && c != '\n')
		{
			if(g_pFullFileSystem->Read(&c, 1, csv_file) != 1)
			{
				//hit EOF
				bEndOfFile = true;
				break;
			}
			else
			{
				append[0] = c;
				read_str += append;
			}
		}

		if (bEndOfFile)
		{
			state = ParseCompleted;
		}

		return !bEndOfFile;
	}

	void CSVParser::_append_another_line_from_file()
	{
		if (_get_line_from_file())
		{
			row_str.erase(0, field_beg);
			row_str += read_str;
			idx -= field_beg;
			field_beg = field_end = 0;
		}
		else
		{
			throw std::runtime_error("Hit EOF when parsing is not completed.");
		}
	}

	void CSVParser::_initialize( CSVDocument* p_doc, const char *file_path )
	{
		if (!p_doc)
		{
			throw std::runtime_error("Destination of parsed data is not set.");
		}
		m_doc = p_doc;

		_open_csv_file(file_path);
		_line_end();
	}

	void CSVParser::_field_end()
	{
		elem.append(row_str.c_str() + field_beg, field_end - field_beg);
		field_beg = idx + 1;
		row.push_back(elem);
		state = FieldEnd;
	}

	void CSVParser::_line_start()
	{
		row.clear();
		++row_count;
		idx = 0;
		state = LineStart;
	}

	void CSVParser::_field_start()
	{
		elem.clear();
		field_beg = field_end = idx;
		state = FieldStart;
	}

	void CSVParser::_escape_on()
	{
		state = EscapeOn;
	}

	void CSVParser::_escape_off()
	{
		elem.append(row_str.c_str() + field_beg, idx - field_beg);
		field_beg = idx + 1;
		state = EscapeOff;
	}

	void CSVParser::_front_quote()
	{
		++field_beg;
		state = FrontQuote;
	}

	void CSVParser::_back_quote()
	{
		field_end = idx - 1;
		state = FrontQuote;
	}

	void CSVParser::_line_end()
	{
		if (row.size() > 0)
		{
			if (col_count == 0)
			{
				col_count = row.size(); // Initialize col_count to the size of the first valid line.
			}

			if (row.size() != col_count)
			{
				std::ostringstream str_stream;
				str_stream << "Syntax error: too " 
					<< (row.size() > col_count ? "much" : "few") 
					<< " fields in line " << row_count << ".";
				throw std::runtime_error(str_stream.str());
			}
			else
			{
				m_doc->add_row(row);
			}
		}
		else if(col_count > 0)
		{
			throw std::runtime_error("Syntax error: empty line is not allowed.");
		}

		if (_get_line_from_file())
		{
			row_str = read_str;
			state = LineEnd;
		}
	}
}