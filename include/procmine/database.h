#pragma once

#include <string>
#include <vector>
#include <memory>
#include <sqlite3.h>
#include <optional>
#include <functional>

namespace procmine {

class QueryResult {
public:
    QueryResult();
    ~QueryResult();
    
    int get_row_count() const;
    
    int get_column_count() const;
    
    std::vector<std::string> get_column_names() const;
    
    std::string get_string(int row, int col) const;
    std::string get_string(int row, const std::string& col_name) const;
    
    int get_int(int row, int col) const;
    int get_int(int row, const std::string& col_name) const;
    
    double get_double(int row, int col) const;
    double get_double(int row, const std::string& col_name) const;
    
    bool is_null(int row, int col) const;
    bool is_null(int row, const std::string& col_name) const;
    
private:
    friend class Database;
    void add_row(char** row_data, int num_cols);
    void set_column_names(char** col_names, int num_cols);
    
    std::vector<std::vector<std::optional<std::string>>> data_;
    std::vector<std::string> column_names_;
};

class Database {
public:
    Database(const std::string& db_path);
    ~Database();
    
    bool execute(const std::string& sql);
    
    std::shared_ptr<QueryResult> query(const std::string& sql);
    
    class Statement {
    public:
        Statement(sqlite3_stmt* stmt);
        ~Statement();
        
        void bind(int index, int value);
        void bind(int index, double value);
        void bind(int index, const std::string& value);
        void bind(int index, std::nullptr_t);
        
        bool execute();
        
        std::shared_ptr<QueryResult> query();
        
    private:
        sqlite3_stmt* stmt_;
    };
    
    std::shared_ptr<Statement> prepare(const std::string& sql);
    
    bool begin_transaction();
    
    bool commit();
    
    bool rollback();
    
    int64_t last_insert_rowid() const;
    
    int get_error_code() const;
    
    std::string get_error_message() const;
    
private:
    sqlite3* db_;
    static int query_callback(void* data, int argc, char** argv, char** col_names);
};

}