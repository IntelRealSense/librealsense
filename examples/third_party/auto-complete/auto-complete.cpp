#include "auto-complete.h"
#include <iostream>
#include <thread>
#include <algorithm>
#include <string>

using namespace std;


vector<string> dictionary::search(const string& word) const
{
    vector<string> finds;

    for (auto it = _dictionary.begin(); it != _dictionary.end(); ++it)
    {
        it = find_if(it, _dictionary.end(), [&](string elemnt) -> bool {
            return (elemnt.compare(0, word.size(), word) == 0) ? true : false;
        });

        if (it != _dictionary.end())
        {
            if (it->compare(word) == 0)
                continue;

            finds.push_back(it->substr(word.size(), it->size()));
        }
        else
        {
            break;
        }
    }

    return finds;
}

void dictionary::add_word(const string& word)
{
    _dictionary.insert(word);
}

bool dictionary::remove_word(const string& word)
{
    return (_dictionary.erase(word) > 0);
}

auto_complete::auto_complete()
    : _history_words_index(0), _num_of_chars_in_line(0), _tab_index(0), _is_first_time_up_arrow(true)
{}

auto_complete::auto_complete(const dictionary& dictionary)
    : _dictionary(dictionary) , _history_words_index(0), _num_of_chars_in_line(0), _tab_index(0), _is_first_time_up_arrow(true)
{}

void auto_complete::update_dictionary(const dictionary& dictionary)
{
    _dictionary = dictionary;
}

string auto_complete::chars_queue_to_string() const
{
    auto temp_chars_queue(_chars_queue);
    temp_chars_queue.push_back('\0');
    auto word = string(temp_chars_queue.data());

    return word;
}

string auto_complete::get_last_word(const string& line) const
{
    auto index = line.find_last_of(' ');
    if (index > line.size())
        return line;

        return line.substr(index + 1);
}

void auto_complete::backspace(const int num_of_backspaces)
{
    for (auto i = 0; i < num_of_backspaces; ++i)
    {
        if (_num_of_chars_in_line == 0)
            break;

        cout << '\b';
        cout << BACKSLASH_ZERO;
        cout << '\b';

        if (_num_of_chars_in_line != 0)
            --_num_of_chars_in_line;

        if (!_chars_queue.empty())
        {
            _chars_queue.pop_back();
        }
    }
}

void auto_complete::handle_special_key(vector<uint8_t> chars)
{
    auto up_arrow_key = vector<uint8_t>(UP_ARROW_KEY);
    auto down_arrow_key = vector<uint8_t>(DOWN_ARROW_KEY);
    if (up_arrow_key == chars && _history_words.size()) // Up arrow key
    {
        if (_history_words_index != 0 && !_is_first_time_up_arrow)
            --_history_words_index;

        _finds_vec.clear();
        _tab_index = 0;
        backspace(_num_of_chars_in_line);
        cout << _history_words[_history_words_index];

        for (auto c : _history_words[_history_words_index])
        {
            _chars_queue.push_back(c);
            ++_num_of_chars_in_line;
        }

        if (_is_first_time_up_arrow)
        {
            _is_first_time_up_arrow = false;
        }
    }
    else if (down_arrow_key == chars && _history_words.size()) // Down arrow key
    {
        if ((_history_words_index + 1) < _history_words.size())
            ++_history_words_index;

        _finds_vec.clear();
        _tab_index = 0;
        backspace(_num_of_chars_in_line);
        cout << _history_words[_history_words_index];

        for (auto c : _history_words[_history_words_index])
        {
            _chars_queue.push_back(c);
            ++_num_of_chars_in_line;
        }
    }
}

char auto_complete::getch_nolock()
{
#if defined(WIN32)
    auto ch = static_cast<char>(_getch_nolock());
    if (ch < 0)
    {
        vector<uint8_t> vec_ch = {static_cast<uint8_t>(_getch_nolock())};
        handle_special_key(vec_ch);
        return -1;
    }

    return ch;
#else
    struct termios old_settings, new_settings;
    tcgetattr( fileno( stdin ), &old_settings );
    new_settings = old_settings;
    new_settings.c_lflag &= (~ICANON & ~ECHO);

    tcsetattr( fileno( stdin ), TCSANOW, &new_settings );
    fd_set set;
    struct timeval tv;

    tv.tv_sec = INFINITY;
    tv.tv_usec = 0;

    FD_ZERO( &set );
    FD_SET( fileno( stdin ), &set );

    int res = select( fileno( stdin )+1, &set, NULL, NULL, &tv );
    if( res > 0 )
    {
        uint8_t ch[10];
        auto num_of_chars = read( fileno( stdin ), ch, 10 );
        tcsetattr( fileno( stdin ), TCSAFLUSH, &old_settings );
        if (num_of_chars == 1)
            return ch[0];

        //TODO: Arrow keys
        vector<uint8_t> vec_ch;
        for (auto i = 0; i < num_of_chars; ++i)
            vec_ch.push_back(ch[i]);

        handle_special_key(vec_ch);
        return -1;
    }
#endif
}


string auto_complete::get_line()
{
    while (true)
    {
        auto ch = getch_nolock();

        if (ch == -1)
        {
            cout.clear();
            cin.clear();
            fflush(nullptr);
            continue;
        }

        if (ch == '\t')
        {
            if (_finds_vec.empty())
            {
                _finds_vec = _dictionary.search(get_last_word(chars_queue_to_string()));
            }


            if (_finds_vec.empty() || _tab_index == _finds_vec.size())
                continue;


            if (_tab_index != 0)
            {
                backspace(int(_finds_vec[_tab_index - 1].length()));
            }

            auto vec_word = _finds_vec[_tab_index++];
            cout << vec_word;

            for (auto c : vec_word)
            {
                _chars_queue.push_back(c);
                ++_num_of_chars_in_line;
            }
        }
        else if (ch == ' ')
        {
            _finds_vec.clear();
            _tab_index = 0;
            _chars_queue.push_back(' ');
            ++_num_of_chars_in_line;

            cout << ch;
        }
        else if (ch == BACKSPACE)
        {
            _finds_vec.clear();
            _tab_index = 0;

            backspace(1);
        }
        else if (ch == NEW_LINE)
        {
            _finds_vec.clear();
            _tab_index = 0;

            cout << '\n';
            cout << ch;
            auto str = chars_queue_to_string();

            if (str.compare(""))
            {
                auto it = find_if(_history_words.begin(), _history_words.end(), [&](string element) -> bool {
                    return (element.compare(str) == 0);
                });

                if (it == _history_words.end())
                {
                    _history_words.push_back(str);
                    _history_words_index = int(_history_words.size()) - 1;
                }
            }
            _is_first_time_up_arrow = true;
        }
        else if (isprint(ch))
        {
            _finds_vec.clear();
            _tab_index = 0;

            cout << ch;
            _chars_queue.push_back(ch);
            ++_num_of_chars_in_line;
        }
        else
            getch_nolock(); // TODO

        if (ch == NEW_LINE)
        {
            auto ret_str = chars_queue_to_string();
            _chars_queue.clear();
            _num_of_chars_in_line = 0;
            return ret_str;
        }

        cout.clear();
        cin.clear();
        fflush(nullptr);
    }
}
