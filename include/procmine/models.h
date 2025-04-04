#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <memory>

namespace procmine {

struct Event {
    std::string activity;
    std::string resource;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> attributes;
};

class Trace {
public:
    Trace() : case_id_("") {}
    Trace(const std::string& case_id);
    void add_event(const Event& event);
    void add_event(Event&& event);
    
    const std::string& get_case_id() const;
    const std::vector<Event>& get_events() const;
    
    std::string get_attribute(const std::string& key) const;
    void set_attribute(const std::string& key, const std::string& value);
    
private:
    std::string case_id_;
    std::vector<Event> events_;
    std::unordered_map<std::string, std::string> attributes_;
};

class EventLog {
public:
    EventLog();
    void add_trace(const Trace& trace);
    void add_trace(Trace&& trace);
    
    const std::vector<Trace>& get_traces() const;
    
    std::vector<std::string> get_activities() const;
    
    std::shared_ptr<EventLog> filter_by_activity(const std::string& activity) const;
    std::shared_ptr<EventLog> filter_by_timeframe(
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end) const;
    
private:
    std::vector<Trace> traces_;
};

}