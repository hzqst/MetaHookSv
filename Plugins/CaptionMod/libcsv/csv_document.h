#ifndef _LIBCSVPLUSPLUS_CSV_DOCUMENT_H_
#define  _LIBCSVPLUSPLUS_CSV_DOCUMENT_H_
#include <vector>
#include <list>
#include <string>
#include <fstream>
#include <metahook.h>
#include <FileSystem.h>

namespace CSV
{
	class CSVParser;

	class CSVDocument
	{
	public:
		typedef std::string element_type;
		typedef std::vector<element_type> row_type;
		typedef std::list<row_type> document_type;
		typedef document_type::size_type row_index_type;
		typedef row_type::size_type column_index_type;
		typedef document_type::iterator iterator;
		typedef document_type::const_iterator const_iterator;

		enum OutputMode{
			CompleteEnclosure,
			OptionalEnclosure
		};

		row_index_type load_file(const char *file_path);
		const document_type& get_document() const;
		const row_type& get_row(row_index_type row) const;
		const element_type& get_element(row_index_type row, column_index_type col) const;
		row_index_type size() const;
		row_index_type row_count() const;
		column_index_type col_count() const;
		iterator begin();
		iterator end();
		row_type& operator[](row_index_type idx);

		void merge_document(const document_type& doc);
		void add_row(const row_type& row);
		void remove_row(row_index_type row_idx);
		void replace_row(row_index_type row_idx, const row_type& row);
		void update_elem(row_index_type row, column_index_type col, const element_type& new_val);
		void clear();

	private:
		int _replace_all( std::string &field, const std::string& old_str, const std::string& new_str );
		void _write_optional_enclosure_field( std::ostream& out_stream, const element_type& elem, bool last_elem );
		void _write_complete_enclosure_field( std::ostream& out_stream, const element_type& elem, bool last_elem );
		void _check_row_index( row_index_type row_idx ) const;
		void _check_col_index( column_index_type col ) const;

		document_type m_doc;
	};

	class CSVParser {
	public:
		enum ParseState{
			LineStart,
			FieldStart,
			FrontQuote,
			BackQuote,
			EscapeOn,
			EscapeOff,
			FieldEnd,
			LineEnd,
			ParseCompleted
		};

		CSVParser();
		~CSVParser();
		CSVDocument::row_index_type parse(CSVDocument* p_doc, const char *file_path);		

	private:
		inline char _curr_char() const;
		inline void _next();

		void _line_start();
		void _field_start();
		void _field_end();
		void _escape_on();
		void _escape_off();
		void _front_quote();
		void _back_quote();
		void _line_end();

		void _post_line_start();
		void _post_field_start();
		void _post_front_quote();
		void _post_escape_on();
		void _post_escape_off();
		void _post_back_quote();
		void _post_field_end();
		void _post_line_end();

		void _open_csv_file(const char *file_path);
		bool _get_line_from_file();
		void _append_another_line_from_file();
		void _initialize(CSVDocument* p_doc, const char * file_path);

		std::string read_str;
		std::string row_str;
		CSVDocument::row_index_type row_count;
		CSVDocument::column_index_type col_count;
		std::string::size_type idx;
		std::string::size_type field_beg;
		std::string::size_type field_end;
		std::string elem;
		CSVDocument::row_type row;
		ParseState state;
		CSVDocument* m_doc;
		FileHandle_t csv_file;
	};
}
#endif