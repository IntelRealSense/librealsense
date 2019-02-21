/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
#pragma once

#include <string>
#include <vector>
#include <stdint.h>
#include <functional>

struct sqlite3;
struct sqlite3_stmt;

namespace sql
{
    struct connection_handle_traits
    {
        using ptr = sqlite3*;

        static auto invalid() -> ptr { return nullptr; }

        static void close(ptr value);
    };

    struct statement_handle_traits
    {
        using ptr = sqlite3_stmt*;

        static auto invalid() -> ptr { return nullptr; }

        static void close(ptr value);
    };

    template<class T>
    class scoped_handle
    {
        typename T::ptr m_handle;
    public:
        scoped_handle() : m_handle(T::invalid()) {}
        scoped_handle(scoped_handle&& other) : m_handle(other.m_handle)
        {
            other.m_handle = T::invalid();
        }
        scoped_handle& operator=(scoped_handle other)
        {
            swap(other);
            return *this;
        }
        void swap(scoped_handle& other)
        {
            std::swap(m_handle, other.m_handle);
        }

        typename T::ptr get() const { return m_handle; }
        typename T::ptr* get_address() { return &m_handle; }

        ~scoped_handle()
        {
            if (m_handle != T::invalid())
            {
                T::close(m_handle);
                m_handle = T::invalid();
            }
        }
    };

    using connection_handle = scoped_handle<connection_handle_traits>;

    class connection;

    using statement_handle = scoped_handle<statement_handle_traits>;

    class statement
    {
        statement_handle m_handle;
    public:
        statement(const connection& conn, const char * sql);

        bool step() const;

        int get_int(int column = 0) const;
        double get_double(int column = 0) const;
        std::string get_string(int column = 0) const;
        std::vector<uint8_t> get_blob(int column = 0) const;

        void bind(int param, int value) const;
        void bind(int param, double value) const;
        void bind(int param, const char* value) const;
        void bind(int param, const std::vector<uint8_t>& value) const;

        class iterator;
        class row_value;

        class column_value
        {
            statement* m_owner;
            int m_column;

            column_value(statement* owner, int column) : m_owner(owner), m_column(column) {}

            friend class row_value;
        public:
            std::string get_string() const { return m_owner->get_string(m_column); }
            int get_int() const { return m_owner->get_int(m_column); }
            double get_double() const { return m_owner->get_double(m_column); }
            int get_bool() const { return m_owner->get_int(m_column) != 0; }
            std::vector<uint8_t> get_blob() const { return m_owner->get_blob(m_column); }
        };

        class row_value
        {
            statement* m_owner;
            bool m_bad;
            row_value(statement* owner, bool bad) : m_owner(owner), m_bad(bad) {}
            void assert_good() const;

            friend class iterator;
        public:
            column_value operator[](int column) const
            {
                assert_good();
                return column_value(m_owner, column);
            }
        };

        class iterator
        {
            statement* m_owner;
            bool m_end;

            iterator(statement* owner) : m_owner(owner), m_end(!owner->step()) { }
            iterator() : m_owner(nullptr), m_end(true) { }

            friend class statement;
        public:
            void operator++()
            {
                m_end = !m_owner->step();
            }

            row_value operator*() const;
            bool operator!=(const iterator& other) { return m_end != other.m_end; }
        };

        iterator begin() { return { this }; }
        iterator end() { return {}; }

        row_value operator()() { return *begin(); }
    };

    class connection
    {
        connection_handle m_handle;

        friend class statement;
    public:
        explicit connection(const char* filename);

        void execute(const char * command) const;

        bool table_exists(const char* name) const;

        void transaction(std::function<void()> transaction) const;
    };
}

sql::statement::iterator begin(sql::statement& stmt);
sql::statement::iterator end(sql::statement& stmt);
