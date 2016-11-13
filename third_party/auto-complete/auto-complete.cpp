#include "auto-complete.h"
#include <iostream>
#include <thread>
#include <algorithm>
#include <string>

using namespace std;


vector<string> Dictionary::Search(const string& word) const
{
    lock_guard<recursive_mutex> lock(_dictionaryMtx);
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

void Dictionary::AddWord(const std::string& word)
{
    lock_guard<recursive_mutex> lock(_dictionaryMtx);
    _dictionary.insert(word);
}

bool Dictionary::RemoveWord(const std::string& word)
{
    lock_guard<recursive_mutex> lock(_dictionaryMtx);
    return (_dictionary.erase(word) > 0);
}

AutoComplete::AutoComplete()
    : _historyWordsIndex(0), _numOfCharsInLine(0), _tabIndex(0), _action(false), _callback(nullptr), _isFirstTimeUpArrow(true)
{}

AutoComplete::AutoComplete(const Dictionary& dictionary)
    : _dictionary(dictionary) , _historyWordsIndex(0), _numOfCharsInLine(0), _tabIndex(0), _action(false), _callback(nullptr), _isFirstTimeUpArrow(true)
{}

AutoComplete::~AutoComplete()
{
    if (_action)
        Stop();
}

void AutoComplete::UpdateDictionary(const Dictionary& dictionary)
{
    lock_guard<recursive_mutex> lock(_dictionaryMtx);
    _dictionary = dictionary;
}

void AutoComplete::RegisterOnKeyCallback(const vector<char>& keys, Callback callback)
{
    lock_guard<recursive_mutex> lock(_regUnregMtx);

    if (_callback)
        throw std::logic_error("Already registered to on-key callback.");

    _callback = callback;
    _keys = keys;
}

void AutoComplete::UnregisterOnKeyCallback()
{
    lock_guard<recursive_mutex> lock(_regUnregMtx);

    if (!_callback)
        throw std::logic_error("There is no any registration to on-key callback.");

    Stop();
    _callback = nullptr;
    _keys.clear();
}

string AutoComplete::CharsQueueToString() const
{
    vector<char> tempCharsQueue(_charsQueue);
    tempCharsQueue.push_back('\0');
    auto word = string(tempCharsQueue.data());

    return word;
}

string AutoComplete::GetLastWord(const string& line) const
{
    auto index = line.find_last_of(' ');
    if (index > line.size())
        return line;

        return line.substr(index + 1);
}

void AutoComplete::Backspace(const int numOfBackspaces)
{
    for (auto i = 0; i < numOfBackspaces; ++i)
    {
        if (_numOfCharsInLine == 0)
            break;

        cout << '\b';
        cout << BACKSLASH_ZERO;
        cout << '\b';

        if (_numOfCharsInLine != 0)
            --_numOfCharsInLine;

        if (!_charsQueue.empty())
        {
            _charsQueue.pop_back();
        }
    }
}

void AutoComplete::Stop()
{
    lock_guard<recursive_mutex> lock(_startStopMtx);

    if (!_action)
        throw std::logic_error("Not started yet.");

    _action = false;
    //FreeConsole(); // TODO: Only on Win OS
    _thread->join();
}

void AutoComplete::HandleSpecialKey(std::vector<uint8_t> chars)
{
    auto up_arrow_key = std::vector<uint8_t>(UP_ARROW_KEY);
    auto down_arrow_key = std::vector<uint8_t>(DOWN_ARROW_KEY);
    if (up_arrow_key == chars && _historyWords.size()) // Up arrow key
    {
        if (_historyWordsIndex != 0 && !_isFirstTimeUpArrow)
            --_historyWordsIndex;

        _findsVec.clear();
        _tabIndex = 0;
        Backspace(_numOfCharsInLine);
        cout << _historyWords[_historyWordsIndex];

        for (auto c : _historyWords[_historyWordsIndex])
        {
            _charsQueue.push_back(c);
            ++_numOfCharsInLine;
        }

        if (_isFirstTimeUpArrow)
        {
            _isFirstTimeUpArrow = false;
        }
    }
    else if (down_arrow_key == chars && _historyWords.size()) // Down arrow key
    {
        if ((_historyWordsIndex + 1) < _historyWords.size())
            ++_historyWordsIndex;

        _findsVec.clear();
        _tabIndex = 0;
        Backspace(_numOfCharsInLine);
        cout << _historyWords[_historyWordsIndex];

        for (auto c : _historyWords[_historyWordsIndex])
        {
            _charsQueue.push_back(c);
            ++_numOfCharsInLine;
        }
    }
}

uint8_t AutoComplete::getch_nolock()
{
#if defined(RS_USE_WMF_BACKEND)
    auto ch = static_cast<uint8_t>(_getch_nolock());
    if (ch < 0)
    {
        std::vector<uint8_t> vec_ch = {static_cast<uint8_t>(_getch_nolock())};
        HandleSpecialKey(vec_ch);
        return -1;
    }

    return ch;
#else
    struct termios oldSettings, newSettings;
    tcgetattr( fileno( stdin ), &oldSettings );
    newSettings = oldSettings;
    newSettings.c_lflag &= (~ICANON & ~ECHO);

    tcsetattr( fileno( stdin ), TCSANOW, &newSettings );
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
        tcsetattr( fileno( stdin ), TCSAFLUSH, &oldSettings );
        if (num_of_chars == 1)
            return ch[0];

        //TODO: Arrow keys
        std::vector<uint8_t> vec_ch;
        for (auto i = 0; i < num_of_chars; ++i)
            vec_ch.push_back(ch[i]);

        HandleSpecialKey(vec_ch);
        return -1;
    }
#endif
}


void AutoComplete::Start()
{
    lock_guard<recursive_mutex> lock(_startStopMtx);

    if (_action)
        throw std::logic_error("Already started.");

    _action = true;
    _thread = std::unique_ptr<std::thread>(new thread([&]()
    {
        while (_action)
        {
            auto ch = getch_nolock();

            if (!_action)
                break;

            if (ch == -1)
            {
                cout.clear();
                cin.clear();
                fflush(nullptr);
                continue;
            }

            if (ch == '\t')
            {
                if (_findsVec.empty())
                {
                    lock_guard<recursive_mutex> lock(_dictionaryMtx);
                    _findsVec = _dictionary.Search(GetLastWord(CharsQueueToString()));
                }


                if (_findsVec.empty() || _tabIndex == _findsVec.size())
                    continue;


                if (_tabIndex != 0)
                {
                    Backspace(int(_findsVec[_tabIndex - 1].length()));
                }

                auto vecWord = _findsVec[_tabIndex++];
                cout << vecWord;

                for (auto c : vecWord)
                {
                    _charsQueue.push_back(c);
                    ++_numOfCharsInLine;
                }
            }
            else if (ch == ' ')
            {
                _findsVec.clear();
                _tabIndex = 0;
                _charsQueue.push_back(' ');
                ++_numOfCharsInLine;

                cout << ch;
            }
            else if (ch == BACKSPACE)
            {
                _findsVec.clear();
                _tabIndex = 0;

                Backspace(1);
            }
            else if (ch == NEW_LINE)
            {
                _findsVec.clear();
                _tabIndex = 0;

                cout << '\n';
                cout << ch;
                auto str = CharsQueueToString();

                if (str.compare(""))
                {
                    auto it = find_if(_historyWords.begin(), _historyWords.end(), [&](string element) -> bool {
                        return (element.compare(str) == 0);
                    });

                    if (it == _historyWords.end())
                    {
                        _historyWords.push_back(str);
                        _historyWordsIndex = int(_historyWords.size()) - 1;
                    }
                }
                _isFirstTimeUpArrow = true;
            }
            else if (isprint(ch))
            {
                _findsVec.clear();
                _tabIndex = 0;

                cout << ch;
                _charsQueue.push_back(ch);
                ++_numOfCharsInLine;
            }
            else
                getch_nolock(); // TODO


            auto it = find_if(_keys.begin(), _keys.end(), [&](char element) -> bool {
                return (element == ch);
            });

            if (it != _keys.end())
            {
                auto str = CharsQueueToString();
                (_callback)(*it, str.c_str(), int(str.size()));
            }           

            if (ch == NEW_LINE)
            {
                _charsQueue.clear();
                _numOfCharsInLine = 0;
            }



            cout.clear();
            cin.clear();
            fflush(nullptr);
        }
    }));
    _thread->detach();
}
