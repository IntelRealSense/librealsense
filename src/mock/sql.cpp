// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#include "sql.h"
#include "../third-party/sqlite/sqlite3.h"
#include <stdexcept>
#include <cstring>
#include <thread>

using namespace std;

namespace sql
{
    template<class T>
    int do_with_retries(T action, int max_retries = 1000)
    {
        int result;
        auto retries = 0;
        do
        {
            result = action();
            if (result == SQLITE_BUSY)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } while (result == SQLITE_BUSY && ++retries < max_retries);

        if (result == SQLITE_BUSY)
        {
            result = SQLITE_BUSY;
        }

        return result;
    }

    void connection_handle_traits::close(ptr value)
    {
        if (SQLITE_OK != do_with_retries([&]() { return sqlite3_close(value); }))
        {
            throw runtime_error(sqlite3_errmsg(value));
        }
    }

    void statement_handle_traits::close(ptr value)
    {
        if (SQLITE_OK != do_with_retries([&]() { return sqlite3_finalize(value); }))
        {
            throw runtime_error("cannot finalize statement");
        }
    }

    connection::connection(const char* filename)
    {
        connection_handle handle;
        auto const code = do_with_retries([&]() { return sqlite3_open(filename, handle.get_address()); });
        if (SQLITE_OK != code)
        {
            throw runtime_error(sqlite3_errmsg(handle.get()));
        }
        m_handle = move(handle);
    }

    void connection::execute(const char * command) const
    {
        auto const code = do_with_retries([&]() { return sqlite3_exec(m_handle.get(), command, nullptr, nullptr, nullptr); });
        if (SQLITE_OK != code)
        {
            throw runtime_error(sqlite3_errmsg(m_handle.get()));
        }
    }

    bool connection::table_exists(const char* name) const
    {
        statement stmt(*this, "SELECT COUNT(name) FROM sqlite_master WHERE type=? AND name=?");
        stmt.bind(1, "table");
        stmt.bind(2, name);
        return stmt()[0].get_bool();
    }

    void connection::transaction(std::function<void()> transaction) const
    {
        char* errorMessage;
        sqlite3_exec(m_handle.get(), "BEGIN TRANSACTION", NULL, NULL, &errorMessage);

        transaction();

        sqlite3_exec(m_handle.get(), "COMMIT TRANSACTION", NULL, NULL, &errorMessage);
    }

    statement::statement(const connection& conn, const char * sql)
    {
        auto const code = do_with_retries([&]() {
            return sqlite3_prepare_v2(conn.m_handle.get(), sql, static_cast<int>(strlen(sql)), m_handle.get_address(), nullptr);
        });
        if (SQLITE_OK != code)
        {
            throw runtime_error(sqlite3_errmsg(conn.m_handle.get()));
        }
    }



    bool statement::step() const
    {
        auto const code = do_with_retries([&]() { return sqlite3_step(m_handle.get()); });

        if (code == SQLITE_ROW) return true;
        if (code == SQLITE_DONE) return false;

        throw runtime_error(sqlite3_errmsg(sqlite3_db_handle(m_handle.get())));
    }

    int statement::get_int(int const column) const
    {
        return sqlite3_column_int(m_handle.get(), column);
    }

    double statement::get_double(int const column) const
    {
        auto val = sqlite3_column_double(m_handle.get(), column);
        return val;
    }

    string statement::get_string(int const column) const
    {
        return reinterpret_cast<const char*>(sqlite3_column_text(m_handle.get(), column));
    }

    vector<uint8_t> statement::get_blob(int column) const
    {
        auto size = sqlite3_column_bytes(m_handle.get(), column);
        vector<uint8_t> result(size, 0);
        auto blob = (unsigned char*)sqlite3_column_blob(m_handle.get(), column);
        std::copy(blob, blob + size, result.data());

        return result;
    }

    void statement::bind(int param, int value) const
    {
        sqlite3_bind_int(m_handle.get(), param, value);
    }

    void statement::bind(int param, double value) const
    {
        sqlite3_bind_double(m_handle.get(), param, value);
    }

    void statement::bind(int param, const char* value) const
    {
        sqlite3_bind_text(m_handle.get(), param, value, -1, SQLITE_STATIC);
    }

    void statement::bind(int param, const std::vector<uint8_t>& value) const
    {
        sqlite3_bind_blob(m_handle.get(), param, value.data(), static_cast<int>(value.size()), SQLITE_STATIC);
    }

    statement::row_value statement::iterator::operator*() const
    {
        return row_value(m_owner, m_end);
    }

    void statement::row_value::assert_good() const
    {
        if (m_bad)
            throw runtime_error("query returned zero results");
    }
}

sql::statement::iterator begin(sql::statement& stmt) { return stmt.begin(); }
sql::statement::iterator end(sql::statement& stmt) { return stmt.end(); }
