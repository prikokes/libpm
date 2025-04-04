#include "procmine/log.h"
#include "procmine/database.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <iomanip>
#include <unordered_set>

namespace procmine {

CSVLogReader::CSVLogReader(const std::string& filepath, char delimiter)
    : filepath_(filepath), delimiter_(delimiter),
      case_column_("case_id"), activity_column_("activity"),
      timestamp_column_("timestamp"), resource_column_("resource") {}

std::shared_ptr<EventLog> CSVLogReader::read() {
    std::ifstream file(filepath_);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filepath_);
    }
    
    std::string line;
    std::shared_ptr<EventLog> log = std::make_shared<EventLog>();

    std::getline(file, line);
    std::stringstream ss(line);
    std::string cell;
    std::vector<std::string> header;
    
    while (std::getline(ss, cell, delimiter_)) {
        header.push_back(cell);
    }

    int case_idx = -1, activity_idx = -1, timestamp_idx = -1, resource_idx = -1;
    for (size_t i = 0; i < header.size(); ++i) {
        if (header[i] == case_column_) case_idx = i;
        else if (header[i] == activity_column_) activity_idx = i;
        else if (header[i] == timestamp_column_) timestamp_idx = i;
        else if (header[i] == resource_column_) resource_idx = i;
    }
    
    if (case_idx == -1 || activity_idx == -1) {
        throw std::runtime_error("Required columns not found in CSV header");
    }

    std::unordered_map<std::string, Trace> traces;

    std::vector<std::string> case_order;

    while (std::getline(file, line)) {
        ss.clear();
        ss.str(line);
        std::vector<std::string> row;
        
        while (std::getline(ss, cell, delimiter_)) {
            row.push_back(cell);
        }
        
        if (row.size() != header.size()) {
            continue;
        }
        
        std::string case_id = row[case_idx];
        std::string activity = row[activity_idx];

        Event event;
        event.activity = activity;
        
        if (resource_idx != -1) {
            event.resource = row[resource_idx];
        }
        
        if (timestamp_idx != -1) {
            try {
                std::tm tm = {};
                std::istringstream ts_stream(row[timestamp_idx]);
                ts_stream >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
                
                if (ts_stream.fail()) {
                    ts_stream.clear();
                    ts_stream.str(row[timestamp_idx]);
                    ts_stream >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
                }
                
                if (!ts_stream.fail()) {
                    event.timestamp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
                }
            } catch (...) {
                event.timestamp = std::chrono::system_clock::now();
            }
        } else {
            event.timestamp = std::chrono::system_clock::now();
        }

        for (size_t i = 0; i < header.size(); ++i) {
            if (i != case_idx && i != activity_idx && i != timestamp_idx && i != resource_idx) {
                event.attributes[header[i]] = row[i];
            }
        }

        if (traces.find(case_id) == traces.end()) {
            case_order.push_back(case_id);
            traces[case_id] = Trace(case_id);
        }
        
        traces[case_id].add_event(event);
    }

    for (const auto& case_id : case_order) {
        log->add_trace(std::move(traces[case_id]));
    }
    
    return log;
}

void CSVLogReader::set_case_column(const std::string& column_name) {
    case_column_ = column_name;
}

void CSVLogReader::set_activity_column(const std::string& column_name) {
    activity_column_ = column_name;
}

void CSVLogReader::set_timestamp_column(const std::string& column_name) {
    timestamp_column_ = column_name;
}

void CSVLogReader::set_resource_column(const std::string& column_name) {
    resource_column_ = column_name;
}

SQLiteLogReader::SQLiteLogReader(const std::string& db_path, const std::string& query)
    : db_path_(db_path), query_(query),
      case_column_("case_id"), activity_column_("activity"),
      timestamp_column_("timestamp"), resource_column_("resource") {}

std::shared_ptr<EventLog> SQLiteLogReader::read() {
    Database db(db_path_);
    auto query_result = db.query(query_);
    
    std::shared_ptr<EventLog> log = std::make_shared<EventLog>();

    auto column_names = query_result->get_column_names();
    
    bool has_case_column = false;
    bool has_activity_column = false;
    
    for (const auto& col : column_names) {
        if (col == case_column_) has_case_column = true;
        else if (col == activity_column_) has_activity_column = true;
    }
    
    if (!has_case_column || !has_activity_column) {
        throw std::runtime_error("Required columns not found in query result");
    }

    std::unordered_map<std::string, Trace> traces;

    for (int row = 0; row < query_result->get_row_count(); ++row) {
        std::string case_id = query_result->get_string(row, case_column_);
        std::string activity = query_result->get_string(row, activity_column_);

        Event event;
        event.activity = activity;

        if (std::find(column_names.begin(), column_names.end(), resource_column_) != column_names.end()) {
            event.resource = query_result->get_string(row, resource_column_);
        }

        if (std::find(column_names.begin(), column_names.end(), timestamp_column_) != column_names.end()) {
            try {
                std::string timestamp_str = query_result->get_string(row, timestamp_column_);

                std::tm tm = {};
                std::istringstream ts_stream(timestamp_str);
                ts_stream >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
                
                if (ts_stream.fail()) {
                    ts_stream.clear();
                    ts_stream.str(timestamp_str);
                    ts_stream >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
                }
                
                if (!ts_stream.fail()) {
                    event.timestamp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
                } else {
                    event.timestamp = std::chrono::system_clock::now();
                }
            } catch (...) {
                event.timestamp = std::chrono::system_clock::now();
            }
        } else {
            event.timestamp = std::chrono::system_clock::now();
        }

        for (const auto& col : column_names) {
            if (col != case_column_ && col != activity_column_ && 
                col != timestamp_column_ && col != resource_column_) {
                event.attributes[col] = query_result->get_string(row, col);
            }
        }

        if (traces.find(case_id) == traces.end()) {
            traces[case_id] = Trace(case_id);
        }
        
        traces[case_id].add_event(event);
    }

    for (auto& trace_pair : traces) {
        log->add_trace(trace_pair.second);
    }
    
    return log;
}

void SQLiteLogReader::set_case_column(const std::string& column_name) {
    case_column_ = column_name;
}

void SQLiteLogReader::set_activity_column(const std::string& column_name) {
    activity_column_ = column_name;
}

void SQLiteLogReader::set_timestamp_column(const std::string& column_name) {
    timestamp_column_ = column_name;
}

void SQLiteLogReader::set_resource_column(const std::string& column_name) {
    resource_column_ = column_name;
}

CSVLogWriter::CSVLogWriter(const std::string& filepath, char delimiter)
    : filepath_(filepath), delimiter_(delimiter) {}

void CSVLogWriter::write(const EventLog& log) {
    std::ofstream file(filepath_);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filepath_);
    }

    file << "case_id" << delimiter_ << "activity" << delimiter_ 
         << "timestamp" << delimiter_ << "resource";

    std::unordered_set<std::string> attribute_names;
    for (const auto& trace : log.get_traces()) {
        for (const auto& event : trace.get_events()) {
            for (const auto& attr : event.attributes) {
                attribute_names.insert(attr.first);
            }
        }
    }

    for (const auto& attr_name : attribute_names) {
        file << delimiter_ << attr_name;
    }
    file << std::endl;

    for (const auto& trace : log.get_traces()) {
        for (const auto& event : trace.get_events()) {
            file << trace.get_case_id() << delimiter_
                 << event.activity << delimiter_;

            auto time_t = std::chrono::system_clock::to_time_t(event.timestamp);
            std::tm tm = *std::localtime(&time_t);
            file << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << delimiter_
                 << event.resource;

            for (const auto& attr_name : attribute_names) {
                file << delimiter_;
                auto it = event.attributes.find(attr_name);
                if (it != event.attributes.end()) {
                    file << it->second;
                }
            }
            
            file << std::endl;
        }
    }
}

SQLiteLogWriter::SQLiteLogWriter(const std::string& db_path, const std::string& table_name)
    : db_path_(db_path), table_name_(table_name) {}

void SQLiteLogWriter::write(const EventLog& log) {
    Database db(db_path_);

    std::unordered_set<std::string> attribute_names;
    for (const auto& trace : log.get_traces()) {
        for (const auto& event : trace.get_events()) {
            for (const auto& attr : event.attributes) {
                attribute_names.insert(attr.first);
            }
        }
    }

    std::string create_table = "CREATE TABLE IF NOT EXISTS " + table_name_ + " ("
                              + "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                              + "case_id TEXT, "
                              + "activity TEXT, "
                              + "timestamp TEXT, "
                              + "resource TEXT";
    
    for (const auto& attr_name : attribute_names) {
        create_table += ", " + attr_name + " TEXT";
    }
    
    create_table += ")";
    
    if (!db.execute(create_table)) {
        throw std::runtime_error("Failed to create table: " + db.get_error_message());
    }

    db.begin_transaction();

    std::string insert_sql = "INSERT INTO " + table_name_ + " (case_id, activity, timestamp, resource";
    
    for (const auto& attr_name : attribute_names) {
        insert_sql += ", " + attr_name;
    }
    
    insert_sql += ") VALUES (?, ?, ?, ?";
    
    for (size_t i = 0; i < attribute_names.size(); ++i) {
        insert_sql += ", ?";
    }
    
    insert_sql += ")";
    
    auto stmt = db.prepare(insert_sql);

    for (const auto& trace : log.get_traces()) {
        for (const auto& event : trace.get_events()) {
            int param_index = 1;

            stmt->bind(param_index++, trace.get_case_id());
            stmt->bind(param_index++, event.activity);

            auto time_t = std::chrono::system_clock::to_time_t(event.timestamp);
            std::tm tm = *std::localtime(&time_t);
            char timestamp_str[20];
            std::strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%d %H:%M:%S", &tm);
            
            stmt->bind(param_index++, std::string(timestamp_str));
            stmt->bind(param_index++, event.resource);

            for (const auto& attr_name : attribute_names) {
                auto it = event.attributes.find(attr_name);
                if (it != event.attributes.end()) {
                    stmt->bind(param_index++, it->second);
                } else {
                    stmt->bind(param_index++, nullptr);
                }
            }
            
            if (!stmt->execute()) {
                db.rollback();
                throw std::runtime_error("Failed to insert data: " + db.get_error_message());
            }
        }
    }

    db.commit();
}

}
