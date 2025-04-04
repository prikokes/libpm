#include "procmine/database.h"
#include <stdexcept>

namespace procmine {

QueryResult::QueryResult() {}

QueryResult::~QueryResult() {}

int QueryResult::get_row_count() const {
    return data_.size();
}

int QueryResult::get_column_count() const {
    return column_names_.size();
}

std::vector<std::string> QueryResult::get_column_names() const {
    return column_names_;
}

std::string QueryResult::get_string(int row, int col) const {
    if (row < 0 || row >= static_cast<int>(data_.size()) ||
        col < 0 || col >= static_cast<int>(data_[row].size())) {
        throw std::out_of_range("Row or column index out of range");
    }
    
    if (!data_[row][col].has_value()) {
        return "";
    }
    
    return data_[row][col].value();
}

std::string QueryResult::get_string(int row, const std::string& col_name) const {
    int col = -1;
    for (int i = 0; i < static_cast<int>(column_names_.size()); ++i) {
        if (column_names_[i] == col_name) {
            col = i;
            break;
        }
    }
    
    if (col == -1) {
        throw std::invalid_argument("Column name not found");
    }
    
    return get_string(row, col);
}

int QueryResult::get_int(int row, int col) const {
    std::string str = get_string(row, col);
    try {
        return std::stoi(str);
    } catch (const std::exception&) {
        throw std::runtime_error("Cannot convert value to int");
    }
}

int QueryResult::get_int(int row, const std::string& col_name) const {
    std::string str = get_string(row, col_name);
    try {
        return std::stoi(str);
    } catch (const std::exception&) {
        throw std::runtime_error("Cannot convert value to int");
    }
}

double QueryResult::get_double(int row, int col) const {
    std::string str = get_string(row, col);
    try {
        return std::stod(str);
    } catch (const std::exception&) {
        throw std::runtime_error("Cannot convert value to double");
    }
}

double QueryResult::get_double(int row, const std::string& col_name) const {
    std::string str = get_string(row, col_name);
    try {
        return std::stod(str);
    } catch (const std::exception&) {
        throw std::runtime_error("Cannot convert value to double");
    }
}

bool QueryResult::is_null(int row, int col) const {
    if (row < 0 || row >= static_cast<int>(data_.size()) ||
        col < 0 || col >= static_cast<int>(data_[row].size())) {
        throw std::out_of_range("Row or column index out of range");
    }
    
    return !data_[row][col].has_value();
}

bool QueryResult::is_null(int row, const std::string& col_name) const {
    int col = -1;
    for (int i = 0; i < static_cast<int>(column_names_.size()); ++i) {
        if (column_names_[i] == col_name) {
            col = i;
            break;
        }
    }
    
    if (col == -1) {
        throw std::invalid_argument("Column name not found");
    }
    
    return is_null(row, col);
}

void QueryResult::add_row(char** row_data, int num_cols) {
    std::vector<std::optional<std::string>> row;
    for (int i = 0; i < num_cols; ++i) {
        if (row_data[i] == nullptr) {
            row.push_back(std::nullopt);
        } else {
            row.push_back(std::string(row_data[i]));
        }
    }
    data_.push_back(row);
}

void QueryResult::set_column_names(char** col_names, int num_cols) {
    column_names_.clear();
    for (int i = 0; i < num_cols; ++i) {
        column_names_.push_back(col_names[i]);
    }
}

Database::Database(const std::string& db_path) : db_(nullptr) {
    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string errmsg = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        throw std::runtime_error("Cannot open database: " + errmsg);
    }
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool Database::execute(const std::string& sql) {
    char* errmsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errmsg);
    
    if (rc != SQLITE_OK) {
        std::string error = errmsg;
        sqlite3_free(errmsg);
        return false;
    }
    
    return true;
}

std::shared_ptr<QueryResult> Database::query(const std::string& sql) {
    auto result = std::make_shared<QueryResult>();
    char* errmsg = nullptr;
    
    int rc = sqlite3_exec(db_, sql.c_str(), query_callback, result.get(), &errmsg);
    
    if (rc != SQLITE_OK) {
        std::string error = errmsg;
        sqlite3_free(errmsg);
        throw std::runtime_error("SQL error: " + error);
    }
    
    return result;
}

int Database::query_callback(void* data, int argc, char** argv, char** col_names) {
    QueryResult* result = static_cast<QueryResult*>(data);
    
    if (result->column_names_.empty()) {
        result->set_column_names(col_names, argc);
    }
    
    result->add_row(argv, argc);
    return 0;
}

Database::Statement::Statement(sqlite3_stmt* stmt) : stmt_(stmt) {}

Database::Statement::~Statement() {
    if (stmt_) {
        sqlite3_finalize(stmt_);
    }
}

void Database::Statement::bind(int index, int value) {
    sqlite3_bind_int(stmt_, index, value);
}

void Database::Statement::bind(int index, double value) {
    sqlite3_bind_double(stmt_, index, value);
}

void Database::Statement::bind(int index, const std::string& value) {
    sqlite3_bind_text(stmt_, index, value.c_str(), -1, SQLITE_TRANSIENT);
}

void Database::Statement::bind(int index, std::nullptr_t) {
    sqlite3_bind_null(stmt_, index);
}

bool Database::Statement::execute() {
    int rc = sqlite3_step(stmt_);
    sqlite3_reset(stmt_);
    return rc == SQLITE_DONE;
}

std::shared_ptr<QueryResult> Database::Statement::query() {
    auto result = std::make_shared<QueryResult>();

    int num_cols = sqlite3_column_count(stmt_);
    std::vector<std::string> column_names;
    for (int i = 0; i < num_cols; ++i) {
        const char* name = sqlite3_column_name(stmt_, i);
        column_names.push_back(name);
    }
    
    char** col_names = new char*[num_cols];
    for (int i = 0; i < num_cols; ++i) {
        col_names[i] = const_cast<char*>(column_names[i].c_str());
    }
    
    result->set_column_names(col_names, num_cols);
    delete[] col_names;

    while (sqlite3_step(stmt_) == SQLITE_ROW) {
        char** row_data = new char*[num_cols];
        for (int i = 0; i < num_cols; ++i) {
            if (sqlite3_column_type(stmt_, i) == SQLITE_NULL) {
                row_data[i] = nullptr;
            } else {
                const unsigned char* text = sqlite3_column_text(stmt_, i);
                row_data[i] = const_cast<char*>(reinterpret_cast<const char*>(text));
            }
        }
        
        result->add_row(row_data, num_cols);
        delete[] row_data;
    }
    
    sqlite3_reset(stmt_);
    return result;
}

std::shared_ptr<Database::Statement> Database::prepare(const std::string& sql) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db_)));
    }
    
    return std::make_shared<Statement>(stmt);
}

bool Database::begin_transaction() {
    return execute("BEGIN TRANSACTION;");
}

bool Database::commit() {
    return execute("COMMIT;");
}

bool Database::rollback() {
    return execute("ROLLBACK;");
}

int64_t Database::last_insert_rowid() const {
    return sqlite3_last_insert_rowid(db_);
}

int Database::get_error_code() const {
    return sqlite3_errcode(db_);
}

std::string Database::get_error_message() const {
    return sqlite3_errmsg(db_);
}

}
