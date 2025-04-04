#include "procmine/models.h"
#include <unordered_set>

namespace procmine {

Trace::Trace(const std::string& case_id) : case_id_(case_id) {}

void Trace::add_event(const Event& event) {
    events_.push_back(event);
}

void Trace::add_event(Event&& event) {
    events_.push_back(std::move(event));
}

const std::string& Trace::get_case_id() const {
    return case_id_;
}

const std::vector<Event>& Trace::get_events() const {
    return events_;
}

std::string Trace::get_attribute(const std::string& key) const {
    auto it = attributes_.find(key);
    if (it != attributes_.end()) {
        return it->second;
    }
    return "";
}

void Trace::set_attribute(const std::string& key, const std::string& value) {
    attributes_[key] = value;
}

EventLog::EventLog() {}

void EventLog::add_trace(const Trace& trace) {
    traces_.push_back(trace);
}

void EventLog::add_trace(Trace&& trace) {
    traces_.push_back(std::move(trace));
}

const std::vector<Trace>& EventLog::get_traces() const {
    return traces_;
}

std::vector<std::string> EventLog::get_activities() const {
    std::unordered_set<std::string> unique_activities;
    
    for (const auto& trace : traces_) {
        for (const auto& event : trace.get_events()) {
            unique_activities.insert(event.activity);
        }
    }
    
    return std::vector<std::string>(unique_activities.begin(), unique_activities.end());
}

std::shared_ptr<EventLog> EventLog::filter_by_activity(const std::string& activity) const {
    auto filtered_log = std::make_shared<EventLog>();
    
    for (const auto& trace : traces_) {
        Trace filtered_trace(trace.get_case_id());
        bool has_activity = false;
        
        for (const auto& event : trace.get_events()) {
            if (event.activity == activity) {
                has_activity = true;
                filtered_trace.add_event(event);
            }
        }
        
        if (has_activity) {
            filtered_log->add_trace(filtered_trace);
        }
    }
    
    return filtered_log;
}

std::shared_ptr<EventLog> EventLog::filter_by_timeframe(
    const std::chrono::system_clock::time_point& start,
    const std::chrono::system_clock::time_point& end) const {
    
    auto filtered_log = std::make_shared<EventLog>();
    
    for (const auto& trace : traces_) {
        Trace filtered_trace(trace.get_case_id());
        bool has_events = false;
        
        for (const auto& event : trace.get_events()) {
            if (event.timestamp >= start && event.timestamp <= end) {
                has_events = true;
                filtered_trace.add_event(event);
            }
        }
        
        if (has_events) {
            filtered_log->add_trace(filtered_trace);
        }
    }
    
    return filtered_log;
}

}
