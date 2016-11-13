// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#include <thread>
#include <set>
#include <vector>
#include <atomic>
#include <functional>
#include <mutex>
#include <stdint.h>
#if defined(RS_USE_WMF_BACKEND)
    #include <conio.h>
    #define NEW_LINE '\r'
    #define BACKSPACE '\b'
    #define BACKSLASH_ZERO '\0'
    #define UP_ARROW_KEY {72}
    #define DOWN_ARROW_KEY {80}
#else
    #include <unistd.h>
    #include <termios.h>
    #define NEW_LINE '\n'
    #define BACKSPACE 127
    #define BACKSLASH_ZERO ' '
    #define UP_ARROW_KEY {27, 91, 65}
    #define DOWN_ARROW_KEY {27, 91, 66}
#endif

class Dictionary
{
public:
    Dictionary() {}
    void AddWord(const std::string& word);
    bool RemoveWord(const std::string& word);
    std::set<std::string> GetAllWords() const {std::lock_guard<std::recursive_mutex> lock(_dictionaryMtx); return _dictionary;}
    std::vector<std::string> Search(const std::string& word) const;
    Dictionary(const Dictionary& obj)
    {
        std::lock_guard<std::recursive_mutex> lock(obj._dictionaryMtx);
        _dictionary = obj.GetAllWords();
    }

    Dictionary& operator=(const Dictionary& other) noexcept
    {
        std::lock_guard<std::recursive_mutex> lock1(_dictionaryMtx);
        std::lock_guard<std::recursive_mutex> lock2(other._dictionaryMtx);
        if (this != &other)
        {
            _dictionary = other.GetAllWords();
        }
        return *this;
    }

private:
    mutable std::recursive_mutex _dictionaryMtx;
    mutable std::set<std::string> _dictionary;
};

typedef std::function<void(const char, const char*, unsigned)> Callback;

class AutoComplete
{
public:
    AutoComplete();
    AutoComplete(const Dictionary& dictionary);
    ~AutoComplete();
    void UpdateDictionary(const Dictionary& dictionary);
    void RegisterOnKeyCallback(const std::vector<char>& keys, Callback callback);
    void UnregisterOnKeyCallback();
    void Start();
    void Stop();

private:
    void HandleSpecialKey(std::vector<uint8_t> chars);
    uint8_t getch_nolock();
    void Backspace(const int numOfBackspaces);
    std::string CharsQueueToString() const;
    std::string GetLastWord(const std::string& line) const;

    std::vector<std::string> _historyWords;
    unsigned _historyWordsIndex;
    std::vector<char> _keys;
    Callback _callback;
    Dictionary _dictionary;
    std::vector<char> _charsQueue;
    int _numOfCharsInLine;
    unsigned _tabIndex;
    std::vector<std::string> _findsVec;
    std::atomic<bool> _action;
    std::recursive_mutex _startStopMtx;
    std::recursive_mutex _regUnregMtx;
    std::recursive_mutex _dictionaryMtx;
    std::unique_ptr<std::thread> _thread;
    bool _isFirstTimeUpArrow;
};
