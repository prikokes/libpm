#pragma once

#include "procmine/models.h"
#include <string>
#include <memory>

namespace procmine {

class LogReader {
public:
    virtual ~LogReader() = default;
    virtual std::shared_ptr<EventLog> read() = 0;
};

class CSVLogReader : public LogReader {
public:
    CSVLogReader(const std::string& filepath, char delimiter = ',');
    std::shared_ptr<EventLog> read() override;

    void set_case_column(const std::string& column_name);
    void set_activity_column(const std::string& column_name);
    void set_timestamp_column(const std::string& column_name);
    void set_resource_column(const std::string& column_name);
    
private:
    std::string filepath_;
    char delimiter_;
    std::string case_column_;
    std::string activity_column_;
    std::string timestamp_column_;
    std::string resource_column_;
};

class SQLiteLogReader : public LogReader {
public:
    SQLiteLogReader(const std::string& db_path, const std::string& query);
    std::shared_ptr<EventLog> read() override;

    void set_case_column(const std::string& column_name);
    void set_activity_column(const std::string& column_name);
    void set_timestamp_column(const std::string& column_name);
    void set_resource_column(const std::string& column_name);
    
private:
    std::string db_path_;
    std::string query_;
    std::string case_column_;
    std::string activity_column_;
    std::string timestamp_column_;
    std::string resource_column_;
};

class LogWriter {
public:
    virtual ~LogWriter() = default;
    virtual void write(const EventLog& log) = 0;
};

class CSVLogWriter : public LogWriter {
public:
    CSVLogWriter(const std::string& filepath, char delimiter = ',');
    void write(const EventLog& log) override;
    
private:
    std::string filepath_;
    char delimiter_;
};

class SQLiteLogWriter : public LogWriter {
public:
    SQLiteLogWriter(const std::string& db_path, const std::string& table_name);
    void write(const EventLog& log) override;
    
private:
    std::string db_path_;
    std::string table_name_;
};

}
