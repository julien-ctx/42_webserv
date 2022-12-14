#include "../includes/toml/parse.hpp"

namespace TOML
{
    //insertion of new values
    // interprete and change the string val, so it can be include in the _hash_tables
    void	parse::insert(type_string key, type_string val)
    {
        type_table new_value;
        if (str_is_string(val))
        {
            if (val[0] == '"')
            {
                if (!val.substr(0, 3).compare("\"\"\"") )
                    val = double_quote_change_string(check_empty_string(val, 6));
                else
                    val = double_quote_change_string(check_empty_string(val, 2));
            }
            else
            {
                if (!val.substr(0, 3).compare("'''") )
                {
                    val = check_empty_string(val, 6);
                    if (val[0] == '\n')
                        val.erase(0, 1);
                }
                else
                    val = check_empty_string(val, 2);
            }
            new_value = type_table(key, val);
        }
        else if (str_is_nbr(val))
        {
            TOML::types	t;
            if (str_is_bool(val))
                t = T_bool;
            else if (str_is_int(val))
                t = T_int;
            else
                t = T_float;
            new_value = type_table(key, TOML::parse::atof(val), t);
        }
		else
		{
            new_value =  type_table(key, false);
		}
        // to do ou don't bother ? to be continued... ==>
        // else if (str_is_date(val))
        // {
            
        // }
        new_value._parent = this->_here;
        this->_hash_tables.push_back(new_value);
    }

    void    parse::insert_array(type_string key, type_string str)
    {
        type_string prev_here(this->_here);
		str.erase(0, 1);
		str.erase(str.size() - 1, 1);
		adding_here(key);
		std::vector<type_string> vec(str_split(str, ","));
		value::type_array	ar;
        for (size_t i = 0; i < vec.size(); i++)
        {
			wspace_trimmer(0, vec[i]);
			for (size_t j = vec[i].size() - 1; j > 0; j--)
			{
				if (!is_whitespace(vec[i][j]) || vec[i][j] != '\n')
				{
					wspace_trimmer(j + 1, vec[i]);
					break ;
				}
			}
			if (str_is_array(vec[i]))
				insert_array(std::to_string(i), vec[i]);
			else
				insert(std::to_string(i), vec[i]);
			ar.push_back(*at_key_parent(std::to_string(i), this->_here));
        }

        type_table	new_value(key, ar);
        new_value._parent = prev_here;
        this->_hash_tables.push_back(new_value);
		this->_here = prev_here;
    }

	void	parse::insert_table(type_string key, bool is_array)
    {
		type_table new_value(key, is_array);
        new_value._parent = this->_here;
        this->_hash_tables.push_back(new_value);
    }

    //parse line by line
    void    parse::begin_parse(type_string config_file)
    {
        type_string     line;
        type_string     temp;
        size_t          pos;
        type_string     tquote;
        size_t          line_nbr = 1;
        std::ifstream	file(config_file);
		char			dquote;
		bool			quote_mode = false;

        while(std::getline(file, temp))
        {
            line = temp;
			pos = 0;
			while (temp[pos]!= '=' && temp[pos])
				pos++;
			while (pos < temp.size())
			{
				if (!quote_mode)
				{
					if (pos <= temp.size() - 3 && (!temp.substr(pos, 3).compare("\"\"\"") || !temp.substr(pos, 3).compare("'''")))
					{
						if (temp.find("\"\"\"", pos) != std::string::npos && (temp.find("'''", pos) == std::string::npos || (int)temp.find("'''", pos) <= (int)temp.find("\"\"\"", pos)))
							tquote = "\"\"\"";
						else
							tquote = "'''";
						pos = temp.find(tquote) + 1;
						do
						{
							if (pos == 0)
							{
								line += "\n";
								line += temp;
							}
							while (temp.find(tquote, pos) != std::string::npos && temp[temp.find(tquote, pos) - 1] == '\\')
								pos = temp.find(tquote, pos) + 1;
							if (temp.find(tquote, pos) != std::string::npos)
								break ;
							pos = 0;
							line_nbr++;
						}  while (std::getline(file, temp));
						break ;
					}
					if (!temp.substr(pos, 1).compare("["))
					{
						size_t	count_brackets = 1;
						pos++;
						do
						{
							if (pos == 0)
							{
								line += "\n";
								line += temp;
							}
							while (pos < temp.size())
							{
								if (!quote_mode)
								{
									if (pos <= temp.size() - 3 && (!temp.substr(pos, 3).compare("\"\"\"") || !temp.substr(pos, 3).compare("'''")))
									{
										dquote = '0';
										tquote = temp.substr(pos, 3);
										quote_mode = true;
									}
									else if (temp[pos] == '\'' || temp[pos] == '"')
									{
										tquote = "0";
										dquote = temp[pos];
										quote_mode = true;
									}
									else if (temp[pos] == '[')
										count_brackets++;
									else if (temp[pos] == ']')
										count_brackets--;
								}
								else
								{
									if (!temp.substr(pos, 3).compare(tquote) || temp[pos] == dquote)
										quote_mode = false;
								}
								if (!count_brackets)
									break ;
								pos++;
							}
							if (!count_brackets)
								break ;
							pos = 0;
						} while (std::getline(file, temp));
						break ;
					}
					if (temp[pos] == '\'' || temp[pos] == '"')
					{
						dquote = temp[pos];
						quote_mode = true;
					}
				}
				else
				{
					if (temp[pos] == dquote)
						quote_mode = false;
				}
				pos++;
			}
            parse_line(line, line_nbr);
            line_nbr++;
        }
       file.close();
    }

    //  parse a line in a document
	void	parse::parse_line(type_string str, size_t line_nbr)
    {
        // remove all whitespace in the begining and ending
        wspace_trimmer(0, str);
        if (str.length() == 0)
            return ;
        for (size_t i = str.length() - 1; i > 0 ; i--)
        {
            if (!is_whitespace(str[i]))
            {
                if (i < str.length() - 1)
                    wspace_trimmer(i + 1, str);
                break ;
            }
        }
        size_t  is_bracket = 0;
        if (str[0] == '[')
            is_bracket = 1;
        if (is_bracket)
        {
			if (str[str.length() - 1] != ']')
				throw	ErrorParse("Expected ']' character", line_nbr);
            if (str[1] == '[')
            {
				is_bracket++;
                if (str[str.length() - 2] != ']')
                    throw	ErrorParse("Expected two ']' character", line_nbr);
            }
			this->_here = _root._key;
            size_t  i = str.length() - is_bracket - 1;
            while (is_whitespace(str[i]))
                    i--;
            wspace_trimmer(i, str);
            if (!str_is_table(str.substr(is_bracket, str.length() - (is_bracket * 2))))
            {
                throw	ErrorParse("It is a wrong table statement", line_nbr);
            }
            if (is_bracket == 1)
            {
                table_last_key(str.substr(1, str.length() - 2), T_table, false, line_nbr);
            }
            else
            {
                table_last_key(str.substr(2, str.length() - 4), T_table, true, line_nbr);
            }
        }
        else
        {
            std::vector<type_string>	key_value(str_split(str, type_string("=")));
        	type_string					prev_here(this->_here);
			

            if (key_value.size() != 2)
				throw	ErrorParse("Not a correct line in TOML", line_nbr);

            size_t  i = key_value[0].length() - 1;
            while (is_whitespace(key_value[0][i]))
                i--;
            wspace_trimmer(i + 1, key_value[0]);
            if (!str_is_table(key_value[0]))
                throw	ErrorParse("It is a wrong table statement", line_nbr);
            type_string key(table_last_key(key_value[0], T_int, false, line_nbr));
            if (!key_value[1].length())
                throw	ErrorParse("No value", line_nbr);
            wspace_trimmer(0, key_value[1]);
            if (str_is_nbr(key_value[1]) || str_is_string(key_value[1]))
            	insert(key, key_value[1]);
			else if (str_is_array(key_value[1]))
				insert_array(key, key_value[1]);
			else
                throw	TypeUndefined("Couldn't find a type that correspond for this value");
			this->_here = prev_here;
        }

    }

	bool	parse::is_hexa(char c)
    {
        if (( c >= '0' && c <= '9') || ( c >= 'a' && c <= 'f') || ( c >= 'A' && c <= 'F'))
            return true;
        return false;
    }

    bool    parse::is_whitespace(char c)
    {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r')
            return true;
        return false;
    }

    bool    parse::is_lower(char c)
    {
        if ((c >= 'a' && c <= 'z'))
            return true;
        return false;
    }

    bool    parse::is_upper(char c)
    {
        if ((c >= 'A' && c <= 'Z'))
            return true;
        return false;
    }

    // All valid characters in keys of TOML files
	bool	parse::valid_key_char(char c)
    {
        if (is_hexa(c) || c == '"'|| c == '\'' || c == '_' || c == '.' || is_whitespace(c) || is_upper(c) || is_lower(c))
            return true;
        return false;
    }

    // Some valid antislash characters in string. Anti slash whitespace is interpreted as trimmed all the whitespace after the anti slash
	bool	parse::valid_antislash(char c)
    {
        if (c == 'b' || c == 't' || c == 'r' || c == 't' || c == '"'  || c == 'u' || c == '\\' || (is_whitespace(c) && c != '\n'))
            return true;
        return false;
    }

    // all functions begining by "str_is" parse a string and verify if it corespond to this kind of type
    bool    parse::str_is_string(type_string str)
    {
		size_t	aslash_counter = 0;
        if (str.length() >= 6
            && ((str.substr(0, 3).compare("\"\"\"") == 0 && str.substr(str.length() - 3, 3).compare("\"\"\"") == 0)
            || (str.substr(0, 3).compare("'''") == 0 && str.substr(str.length() - 3, 3).compare("'''") == 0)))
        {
            if (str[0] == '"')
            {
				size_t	count_dquote = 0;
                for (size_t i = 3; i < str.length() - 3; i++)
				{
                    if (i > 3 && str[i - 1] == '\\' && !valid_antislash(str[i]) && str[i] != 'n' && aslash_counter % 2 == 1)
                        return false;
					if (str[i] == '"' && str[i - 1] != '\\')
						count_dquote++;
					else
						count_dquote = 0;
					if (count_dquote >= 3)
						return false;
					if (str[i] == '\\')
						aslash_counter++;
					else
						aslash_counter = 0;
				}
                if (str[str.length() - 4] == '\\'  && str[str.length() - 5] != '\\')
                    return false;
            }
			if (str[0] == '\'')
            {
				size_t	count_quote = 0;
                for (size_t i = 3; i < str.length() - 3; i++)
				{
					if (str[i] == '\'')
						count_quote++;
					else
						count_quote = 0;
					if (count_quote >= 3)
						return false;
				}
            }
            return true;
        }
        if (str.length() >= 3 && ((str[0] == '"' && str[str.length() - 1] == '"') || (str[0] == '\'' && str[str.length() - 1] == '\'')))
        {
            if (str[0] == '"')
            {
                for (size_t i = 1; i < str.length() - 1; i++)
				{
					if (str[i] == '\n' || (str[i] ==  '"' && i < str.length() - 1))
						return false;
                    if (i > 1 && str[i - 1] == '\\' && (!valid_antislash(str[i]) || is_whitespace(str[i]) || str[i] == '"')
						&& aslash_counter % 2 == 1)
						return false;

					if (i > 1 && str[i - 1] == '\\' && str[i] == 'u')
					{
						if (str.size() < 8 )
							return false;
						for (size_t j = i + 1; j < i + 5; j++)
							if (!is_hexa(str[j]))
								return false;
					}
					if (str[i] == '\\')
						aslash_counter++;
					else
						aslash_counter = 0;
				}
                if (aslash_counter % 2 == 1)
                    return false;
            }
			if (str[0] == '\'')
            {
				for (size_t i = 1; i < str.length() - 1; i++)
					if (str[i] == '\n' || str[i] == '\'')
                        return false;
            }
            return true;
        }
		if (str.length() == 2 && (!str.compare("\"\"") || !str.compare("''")))
			return true;
        return false;
    }

    bool    parse::str_is_nbr(type_string str)
    {
        if (str_is_bool(str) || str_is_int(str) || str_is_float(str))
            return true;
        return false;
    }

	bool	parse::str_is_int(type_string str)
    {
        size_t  count_nbr = 0;
        for (size_t i = 0; i < str.length(); i++)
        {
            if (i == 0 && str[i] != '-' && str[i] != '+' && (str[i] < '0' || str[i] > '9'))
                return false;
            if (i == 1 && (str[i] < '0' || str[i] > '9') && (str[i] != '_' || str[0] < '0' || str[0] > '9')
                && (str[i] != 'x' || str[0] != '0'))
                {
                    if (str[i] == 'o' && str[0] == '0' && str.length() > 2 && only_octal(str.substr(2, str.length() - 2)))
                        break;
                    else if (str[i] == 'b' && str[0] == '0' && str.length() > 2 && only_binary(str.substr(2, str.length() - 2)))
                        break;
                    else
						return false;
                }
            if (i > 1 && (str[i] < '0' || str[i] > '9')
                && (str[i] != '_' || str[1] != 'x' || count_nbr != 4)
                && (!is_hexa(str[i]) || str[1] != 'x')
                && (str[i] != '_' || str[1] == 'x' || !count_nbr || count_nbr > 3))
                	return false;
            if (str[i] == '_')
                count_nbr = 0;
            else if (i == 1 && str[i] == 'x')
                count_nbr = 0;
			else if (is_hexa(str[i]))
                count_nbr++;
        }
        if (!is_hexa(str[str.length() - 1]))
            return false;
        return true;
    }

	bool	parse::str_is_float(type_string str)
    {
        bool    exponent = false;
		if (str[str.length() - 1] < '0' || str[str.length() - 1] > '9')
			return false;
        for (size_t i = 0; i < str.length(); i++)
        {
            if (i == 0 && str[i] != '-' && str[i] != '+' && (str[i] < '0' || str[i] > '9'))
                return false;
            if (i > 0  && (str[i] < '0' || str[i] > '9'))
            {
                if (str[i] == '-' || str[i] == '+')
				{
                    if (str[i - 1] != 'e' && str[i - 1] != 'E')
                        return false;
				}
                else if (str[i] == 'e' || str[i] == 'E' || str[i] == '.')
				{
                    if (exponent || str[i - 1] == '.')
                        return false;
				}
                else
                    return false;
				if (str[i] == 'e' || str[i] == 'E')
                    exponent = true;
            }
        }
        return true;
    }

    bool    parse::str_is_bool(type_string str)
    {
        if (str.compare("true") == 0 || str.compare("false") == 0)
            return true;
        return false;
    }


	bool	parse::str_is_array(type_string str)
    {
		bool	(TOML::parse::*func)(type_string);
        if (str[0] != '[' && str[str.size() - 1] != ']')
            return false;
        std::vector<type_string> vec(str_split(str.substr(1, str.size() - 2) , ","));
		if (!vec.size())
			return false;
		for (size_t i = 0; i < vec.size(); i++)
		{
			wspace_trimmer(0,vec[i]);
			for (size_t j = vec[i].size() - 1; j > 0 ; j--)
			{
				if (!is_whitespace(vec[i][j]))
				{
					wspace_trimmer(j + 1, vec[i]);
					break ;
				}
			}
			if (!i)
			{
				if (str_is_int(vec[0]))
					func = &parse::str_is_int;
				else if (str_is_bool(vec[0]))
					func = &parse::str_is_bool;
				else if (str_is_float(vec[0]))
					func = &parse::str_is_float;
				else if (str_is_string(vec[0]))
					func = &parse::str_is_string;
				else if (str_is_array(vec[0]))
					func = &parse::str_is_array;
				else
					return false;
			}
			else
			{
				if (!(this->*func)(vec[i]))
					return false;
			}
		}
		return true;

    }

    bool    parse::str_is_table(type_string str)
    {
        //change mode if it is a string or not
        bool    quote_mode = false;
        //the begining of the string in quote_mode
        size_t  str_begin;
        //the lenght of the string in quote_mode
        size_t  str_len;
        //the nature of the string (double quote or normql auote)
		char	cquote;
        //verify if there was a space between valid key charcters
		bool	was_space = false;

        for (size_t i = 0; i < str.length(); i++)
        {
			if (quote_mode)
				str_len++;
            if (str[i] == '.' && !was_space)
                return false;
            if ((!valid_key_char(str[i]) && !quote_mode)
				||	(i > 0 && !quote_mode && was_space && valid_key_char(str[i]) && !is_whitespace(str[i]) && str[i] != '.' && is_whitespace(str[i - 1])))
                	return false;
			if (i == 0 || (i > 0 && valid_key_char(str[i]) && !is_whitespace(str[i])
                && str[i] != '.' && (is_whitespace(str[i - 1]) || str[i - 1] == '.')))
				was_space = true;
			if (str[i] == '.' && !quote_mode)
				was_space = false;
            if (str[i] == '"' || str[i] == '\'')
            {
                if (i > 0 && !quote_mode && str[i - 1] != '.' && !is_whitespace(str[i - 1]))
					return false;
                if (quote_mode && cquote == str[i])
                {
					if (str[i - 1] != '\\' || (i > 2 && str[i - 2] == '\\'))
					{
						if ((i + 1 ) < str.length() && str[i + 1] != '.' && !is_whitespace(str[i + 1]))
							return false;
						quote_mode = false;
						if (!str_is_string(str.substr(str_begin, str_len)))
							return false;
					}
                }
				else if (!quote_mode)
                {
                    quote_mode = true;
					str_len = 1;
                    str_begin = i;
					cquote = str[i];
                }
            }
        }
		if (str[str.length() - 1] == '.' || quote_mode)
			return false;
        return true;
    }

	bool	parse::only_binary(type_string str)
    {
        for (size_t i = 0; i < str.length(); i++)
            if (str[i] != '0' && str[i] != '1')
                return false;
        return true;
    }

    bool	parse::only_octal(type_string str)
    {
        for (size_t i = 0; i < str.length(); i++)
            if (str[i] < '0' || str[i] > '7')
                return false;
        return true;
    }

    //searching
    //  return a pointer of value base on is key and parent
	parse::pointer	parse::at_key_parent(type_string key, type_string parent)
    {

		for (size_t i = 0; i < this->_hash_tables.size(); i++)
		{
            if (!key.compare(this->_hash_tables[i]._key) && !parent.compare(this->_hash_tables[i]._parent))
                return &this->_hash_tables[i];
		}
        return NULL;
    }
	// 

	parse::type_array	parse::by_key(type_string key)
	{
		parse::type_array	ar;
		for (size_t i = 0; i < this->_hash_tables.size(); i++)
		{
			if (this->_hash_tables[i]._key == key)
				ar.push_back(this->_hash_tables[i]);
		}
		return ar;
	}

	parse::type_array	parse::by_table(type_string parent)
	{
		parse::type_array	ar;
		for (size_t i = 0; i < this->_hash_tables.size(); i++)
		{
			if (!parent.compare(this->_hash_tables[i]._parent))
				ar.push_back(this->_hash_tables[i]);
		}
		return ar;
	}


    //utiles
    // 
	parse::type_string	parse::table_last_key(type_string str, TOML::types t, bool is_array, size_t line_nbr)
    {
        std::vector<type_string> tables(str_split(str, type_string(".")));
        pointer				original;
        type_string         convert;
        type_string         ret;
        type_string			prev_here;
        pointer             to_add;


        for (size_t i = 0; i < tables.size(); i++)
        {
			
            wspace_trimmer(0, tables[i]);
            for (size_t j = tables[i].size() - 1; j > 0 ; j--)
            {
                if (!is_whitespace(tables[i][j]))
                {
                    wspace_trimmer(j + 1, tables[i]);
                    break ;
                }
            }
            if (str_is_string(tables[i]) && tables[i][0] == '"')
                tables[i] = double_quote_change_string(tables[i].substr(1, tables[i].size() - 2));
            else if (str_is_string(tables[i]))
            {
                tables[i].erase(0, 1);
                tables[i].erase(tables[i].size() - 1, 1);
            }
            original = at_key_parent(tables[i], this->_here);
            if (i < tables.size() - 1)
            {
                if (original != NULL && original->_typing != T_table)
                    throw ErrorParse("Value already exist in this table", line_nbr);
                if (original == NULL)
                        insert_table(tables[i], false);
                original = at_key_parent(tables[i], this->_here);
                adding_here(original->_key);
                if (original->_is_array_table)
                {
                    convert = std::to_string(original->_array.size() - 1);
                    adding_here(at_key_parent(convert, this->_here)->_key);
                }
            }
            else
            {
                if (original != NULL)
                {
                    if (original->_typing != T_table || !is_array)
                        throw ErrorParse("Value already exist in this table", line_nbr);
                    prev_here = this->_here;
                    adding_here(at_key_parent(tables[i], this->_here)->_key);
                    convert = std::to_string(original->_array.size());
                    insert_table(convert, false);
                    original = at_key_parent(convert, this->_here);
                    to_add = at_key_parent( tables[i], prev_here);
                    to_add->_array.push_back(*original);
                    adding_here(at_key_parent(convert, this->_here)->_key);
                }
                else
                {
                    if (t == T_table)
                    {
                        insert_table(tables[i], is_array);
        				original = at_key_parent(tables[i], this->_here);
                        prev_here = this->_here;
                        adding_here(original->_key);
                    }
                    if (is_array)
                    {
                        convert = type_string("0");
                    	insert_table(convert, false);
                    	original = at_key_parent(convert, this->_here);
                        to_add = at_key_parent(tables[i], prev_here);
                    	to_add->_array.push_back(*original);
						adding_here(original->_key);
                    }
                }
                ret = tables[i];
            }
        }
        return ret;
    }

    // a str split splecial to TOML parsing
	std::vector<parse::type_string>	parse::str_split(type_string str, type_string ocu)
    {
        std::vector<type_string> vec;
        size_t  begin = 0;
        size_t  len = 0;
        char    the_quote;
        bool    quote_mode = false;

        
        for (size_t i = 0; i < str.length(); i++ )
        {
            if (quote_mode)
            {
                if ((str[i] == '\'' && the_quote == '\'') || (str[i] == '"' && the_quote == '"') || (str[i] == ']' && the_quote == '['))
                    quote_mode = false;
            }
            else
            {
                if (!ocu.compare(str.substr(i, ocu.length())))
                {
                    vec.push_back(str.substr(begin, len));
                    i += ocu.length();
                    begin = i;
                    len = 0;
                }
                if (str[i] == '\'' || str[i] == '"' || str[i] == '[')
                {
                    quote_mode = true;
                    if (str[i] == '\'')
                        the_quote = '\'';
                    if (str[i] == '"')
                        the_quote = '"';
                    if (str[i] == '[')
                        the_quote = '[';
                }
            }
            len++;
        }
        vec.push_back(str.substr(begin, len));
        return vec;
    }
	// if the string contain only quote or double quotes return a default string
    // else return what is between
    parse::type_string parse::check_empty_string(type_string str, size_t len)
    {
        if (str.length() > len)
            str = str.substr(((len / 2)), str.length() - len);
        else
            str = type_string();
        return str;
    }

    // interprete the string given and return the interpretation
    parse::type_string parse::double_quote_change_string(type_string str)
    {
        bool    already_replace = false;
        for (size_t i = 0; i < str.length(); i++)
        {
            if (i > 0 && str[i - 1] == '\\' && !already_replace)
            {
                i--;
                if (is_whitespace(str[i + 1]))
                {
                    str.erase(i, 1);
                    wspace_trimmer(i, str);
                }
                else
                {
                    switch (str[i + 1])
                    {
                        case 'b':
                        str.replace(i, 2, "\b");
                        break;
                        case 't':
                        str.replace(i, 2, "\t");
                        break;
                        case 'n':
                        str.replace(i, 2, "\n");
                        break;
                        case 'f':
                        str.replace(i, 2, "\f");
                        break;
                        case 'r':
                        str.replace(i, 2, "\r");
                        break;
                        case '"':
                        str.replace(i, 2, "\"");
                        break;
                        case '\\':
                        str.replace(i, 2, "\\");
                        already_replace = true;
                        break;
                        case 'u':
                        already_replace = unicode_interpreter(i, str);
                        break;
                    }
                }
            }
            else
                already_replace = false;
        }
        return str;
    }

    bool parse::unicode_interpreter(size_t pos, type_string &str)
    {
        const long code = std::strtol(str.substr(pos + 2, 6).c_str(), NULL, 16);
        if (code <= 0x7F) //code ascii 0 to 127
        {
            str.replace(pos, 6, 1, static_cast<char>(code));
            if (code == 0x5C)
                return true;
        }
        else if (code >= 0x0080 && code <= 0x07FF) // Two point unicode 128 to 2048
        {
            unsigned char unicode[3] = { 0b11000000, 0b10000000, 0x00 };
            unicode[1] |= (code)		& 0b111111;
            unicode[0] |= (code >> 6)	& 0b011111;
            str.replace(pos, 6, reinterpret_cast<const char *>(unicode));
        }
        else // Three point unicode  //2048 to FFFF
        {
            unsigned char unicode[4] = { 0b11100000, 0b10000000, 0b10000000, 0x00 };
            unicode[2] |= (code)		& 0b111111;
            unicode[1] |= (code >> 6)	& 0b111111;
            unicode[0] |= (code >> 12)	& 0b001111;
            str.replace(pos, 6, reinterpret_cast<const char *>(unicode));
        }
        return false;
    }

    void	parse::wspace_trimmer(size_t pos, type_string &str)
    {
        size_t  i = pos;
		size_t	count= 0;
        while(is_whitespace(str[i++]) || str[i] == '\n')
            count++;
        str.erase(pos, count);
    }

    float	parse::atof(type_string str)
    {
        //bool
		if (str.compare("true") == 0)
			return 1;
		if (str.compare("false") == 0)
			return 0;
		
		float	nbr;
		//int
		if (str_is_int(str))
		{
			//supress every underscore
			for (size_t i = 0; i < str.length() ; i++)
			{
				if (str[i] == '_' || str[i] == '+')
				{
					str.erase(i, 1);
                    if (i > 0)
					    i--;
				}
			}
			//convert to binary, octal or hexadecimal
			if (str[1] == 'b')
				nbr = str_base_to_int(str.substr(2, str.length() - 2), 2);
			else if (str[1] == 'o')
				nbr = str_base_to_int(str.substr(2, str.length() - 2), 8);
			else if (str[1] == 'x')
				nbr = str_base_to_int(str.substr(2, str.length() - 2), 16);
			else
				nbr = str_base_to_int(str, 10);
		}
		//float
		else
		{
            //using dot for knowing where is the comma
            //using expo for knowing where is exponantiel
            size_t  dot = 0;
            size_t  is_expo = 0;
            for (size_t i = 0; i < str.length() ; i++)
            {
                if (str[i] == '.')
                    dot = i;
                if (str[i] == '.' || str[i] == '+')
                {
                    str.erase(i, 1);
                    if (i > 0)
                    {
					    i--;
                        is_expo--;
                    }
                    
                }
                if (str[i] == 'e' || str[i] == 'E')
                    break;
                is_expo++;
            }
            nbr = str_base_to_int(str.substr(0, is_expo), 10);
            //dividing to the original comma
            float  ten = 1;
            if (dot > 0)
            {
                dot = (is_expo - dot);
                for (size_t i = 0; i < dot ; i++)
                    ten *= 10;
                nbr /= ten;
                ten = 1;
            }
            //multiplying with the exponential
            if (is_expo < str.length())
            {
				int64_t	exponent;
                exponent = static_cast<int64_t>(str_base_to_int(str.substr(is_expo + 1, str.length() - is_expo), 10));
                for (int64_t i = 0; i < exponent; i++)
                    ten *= 10;
				for (int64_t i = 0; i > exponent; i--)
                    ten /= 10;
                nbr *= ten;
            }
		}
		return nbr;
    }

    // convert a string into a float depending of the base (yes there is int in the name but i use most of the time for integers)
	float		parse::str_base_to_int(std::string str, size_t base)
	{
		float	nbr = 0;
		float	power;
        bool  minus = false;
        if (str[0] == '-')
        {
            str.erase(0, 1);
            minus = true;
        }
		for (size_t i = 0; i <  str.length(); i++)
		{
			power = 1;
			for (size_t j = 0; j <  (str.length() - i - 1); j++)
				power *= base;
			nbr += (power * char_to_int(str[i]));
		}
        if (minus)
            nbr *= (-1);
		return nbr;
	}

    // interpret a char to a int
	float		parse::char_to_int(char c)
	{
		if (c >= '0' && c <= '9')
			return static_cast<float>(c - 48);
		if (c >= 'a' && c <= 'f')
			return static_cast<float>(c - 87);
		if (c >= 'A' && c <= 'F')
			return static_cast<float>(c - 55);
		return 0;
	}

	parse::type_table	parse::new_table(type_string key, bool	has_array)
	{
		return (type_table(key, has_array));
	}

	// add a table to _here 
	parse::type_string					parse::adding_here(type_string table_key)
	{
		if (this->_here.size())
			this->_here += ".";
		this->_here += table_key;
		return this->_here;
	}

}
