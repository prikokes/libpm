#include <gtest/gtest.h>
#include "procmine/log.h"
#include "procmine/models.h"
#include <fstream>
#include <filesystem>

namespace {
    using namespace procmine;
    using namespace std::chrono;

    std::string create_test_csv() {
        std::string filepath = "test_log.csv";
        
        std::ofstream file(filepath);
        file << "case_id,activity,timestamp,resource,cost,priority\n";
        file << "case1,A,2023-01-01 10:00:00,user1,100,high\n";
        file << "case1,B,2023-01-01 10:30:00,user2,150,medium\n";
        file << "case1,C,2023-01-01 11:00:00,user1,200,low\n";
        file << "case2,A,2023-01-02 09:00:00,user3,120,high\n";
        file << "case2,C,2023-01-02 09:30:00,user1,180,medium\n";
        file << "case2,B,2023-01-02 10:00:00,user2,90,high\n";
        file.close();
        
        return filepath;
    }

    EventLog create_test_log() {
        EventLog log;

        Trace trace1("case1");
        
        Event e1;
        e1.activity = "A";
        e1.resource = "user1";
        e1.timestamp = system_clock::now();
        e1.attributes["cost"] = "100";
        e1.attributes["priority"] = "high";
        trace1.add_event(e1);
        
        Event e2;
        e2.activity = "B";
        e2.resource = "user2";
        e2.timestamp = system_clock::now() + seconds(1800);
        e2.attributes["cost"] = "150";
        e2.attributes["priority"] = "medium";
        trace1.add_event(e2);
        
        log.add_trace(trace1);

        Trace trace2("case2");
        
        Event e3;
        e3.activity = "A";
        e3.resource = "user3";
        e3.timestamp = system_clock::now() + hours(24);
        e3.attributes["cost"] = "120";
        e3.attributes["priority"] = "high";
        trace2.add_event(e3);
        
        log.add_trace(trace2);
        
        return log;
    }
}

TEST(LogTest, CSVLogReader) {
    std::string csv_path = create_test_csv();

    CSVLogReader reader(csv_path);

    reader.set_case_column("case_id");
    reader.set_activity_column("activity");
    reader.set_timestamp_column("timestamp");
    reader.set_resource_column("resource");

    auto log = reader.read();

    ASSERT_NE(log, nullptr);

    const auto& traces = log->get_traces();
    EXPECT_EQ(traces.size(), 2);

    const auto& trace1 = traces[0];
    EXPECT_EQ(trace1.get_case_id(), "case1");
    EXPECT_EQ(trace1.get_events().size(), 3);

    const auto& events1 = trace1.get_events();
    EXPECT_EQ(events1[0].activity, "A");
    EXPECT_EQ(events1[0].resource, "user1");
    EXPECT_EQ(events1[0].attributes.at("cost"), "100");
    EXPECT_EQ(events1[0].attributes.at("priority"), "high");
    
    EXPECT_EQ(events1[1].activity, "B");
    EXPECT_EQ(events1[1].resource, "user2");
    
    EXPECT_EQ(events1[2].activity, "C");

    const auto& trace2 = traces[1];
    EXPECT_EQ(trace2.get_case_id(), "case2");
    EXPECT_EQ(trace2.get_events().size(), 3);

    auto activities = log->get_activities();
    EXPECT_EQ(activities.size(), 3);

    std::filesystem::remove(csv_path);
}

TEST(LogTest, CSVLogWriter) {
    EventLog log = create_test_log();

    std::string output_csv = "output_log.csv";

    CSVLogWriter writer(output_csv);

    writer.write(log);

    EXPECT_TRUE(std::filesystem::exists(output_csv));

    CSVLogReader reader(output_csv);
    auto read_log = reader.read();

    EXPECT_EQ(read_log->get_traces().size(), 2);

    std::filesystem::remove(output_csv);
}

TEST(LogTest, LogFiltering) {
    EventLog log = create_test_log();

    auto filtered_by_a = log.filter_by_activity("A");
    EXPECT_EQ(filtered_by_a->get_traces().size(), 2);

    for (const auto& trace : filtered_by_a->get_traces()) {
        for (const auto& event : trace.get_events()) {
            EXPECT_EQ(event.activity, "A");
        }
    }

    auto filtered_by_b = log.filter_by_activity("B");
    EXPECT_EQ(filtered_by_b->get_traces().size(), 1);

    auto future_time = system_clock::now() + hours(48);
    auto filtered_by_time = log.filter_by_timeframe(future_time, future_time + hours(24));
    EXPECT_EQ(filtered_by_time->get_traces().size(), 0);
} 