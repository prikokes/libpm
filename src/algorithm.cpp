#include "procmine/algorithm.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <boost/graph/breadth_first_search.hpp>

namespace procmine {

ProcessGraph::ProcessGraph() {}

Vertex ProcessGraph::add_node(const std::string& activity) {
    auto it = activity_to_vertex_.find(activity);
    if (it != activity_to_vertex_.end()) {
        return it->second;
    }
    
    Vertex v = boost::add_vertex(graph_);
    graph_[v].activity = activity;
    activity_to_vertex_[activity] = v;
    vertex_to_activity_[v] = activity;
    return v;
}

Edge ProcessGraph::add_edge(const Vertex& from, const Vertex& to, double weight) {
    bool exists;
    Edge e;
    tie(e, exists) = boost::add_edge(from, to, graph_);
    graph_[e].weight = weight;
    return e;
}

Edge ProcessGraph::add_edge(const std::string& from, const std::string& to, double weight) {
    Vertex v_from = add_node(from);
    Vertex v_to = add_node(to);
    return add_edge(v_from, v_to, weight);
}

std::vector<std::string> ProcessGraph::get_nodes() const {
    std::vector<std::string> result;
    auto vertices = boost::vertices(graph_);
    for (auto vit = vertices.first; vit != vertices.second; ++vit) {
        result.push_back(graph_[*vit].activity);
    }
    return result;
}

std::vector<ProcessGraph::EdgeInfo> ProcessGraph::get_outgoing_edges(const std::string& node) const {
    std::vector<EdgeInfo> result;
    
    auto it = activity_to_vertex_.find(node);
    if (it == activity_to_vertex_.end()) {
        return result;
    }
    
    Vertex v = it->second;
    auto out_edges = boost::out_edges(v, graph_);
    
    for (auto eit = out_edges.first; eit != out_edges.second; ++eit) {
        Vertex target = boost::target(*eit, graph_);
        
        EdgeInfo info;
        info.from = node;
        info.to = vertex_to_activity_.at(target);
        info.weight = graph_[*eit].weight;
        
        result.push_back(info);
    }
    
    return result;
}

std::string ProcessGraph::to_dot() const {
    std::stringstream ss;

    boost::write_graphviz(
        ss, graph_,
        [this](std::ostream& out, const Vertex& v) {
            out << "[label=\"" << graph_[v].activity << "\", shape=box]";
        },
        [this](std::ostream& out, const Edge& e) {
            out << "[label=\"" << graph_[e].weight << "\"]";
        }
    );
    
    return ss.str();
}

AlphaAlgorithm::AlphaAlgorithm() {}

std::shared_ptr<ProcessGraph> AlphaAlgorithm::mine(const EventLog& log) {
    auto result = std::make_shared<ProcessGraph>();

    auto activities = log.get_activities();

    for (const auto& activity : activities) {
        result->add_node(activity);
    }

    for (const auto& trace : log.get_traces()) {
        const auto& events = trace.get_events();
        for (size_t i = 0; i < events.size() - 1; ++i) {
            result->add_edge(events[i].activity, events[i + 1].activity);
        }
    }
    
    return result;
}

HeuristicMiner::HeuristicMiner(double dependency_threshold, 
                             double positive_observations_threshold)
    : dependency_threshold_(dependency_threshold),
      positive_observations_threshold_(positive_observations_threshold) {}

std::shared_ptr<ProcessGraph> HeuristicMiner::mine(const EventLog& log) {
    auto result = std::make_shared<ProcessGraph>();

    auto activities = log.get_activities();

    for (const auto& activity : activities) {
        result->add_node(activity);
    }

    std::unordered_map<std::string, std::unordered_map<std::string, int>> transitions;
    
    for (const auto& trace : log.get_traces()) {
        const auto& events = trace.get_events();
        for (size_t i = 0; i < events.size() - 1; ++i) {
            transitions[events[i].activity][events[i + 1].activity]++;
        }
    }

    for (const auto& from_activity : activities) {
        for (const auto& to_activity : activities) {
            if (from_activity == to_activity) continue;
            
            int a_to_b = transitions[from_activity][to_activity];
            int b_to_a = transitions[to_activity][from_activity];

            double dependency = 0.0;
            if (a_to_b + b_to_a > 0) {
                dependency = ((double)(a_to_b - b_to_a)) / ((double)(a_to_b + b_to_a + 1));
            }

            if (dependency > dependency_threshold_ && a_to_b > positive_observations_threshold_) {
                result->add_edge(from_activity, to_activity, dependency);
            }
        }
    }
    
    return result;
}

FrequencyAnalyzer::FrequencyAnalyzer() {}

FrequencyAnalyzer::FrequencyMetrics FrequencyAnalyzer::analyze(const EventLog& log) {
    FrequencyMetrics metrics;

    for (const auto& trace : log.get_traces()) {
        const auto& events = trace.get_events();

        std::vector<std::string> variant;
        for (const auto& event : events) {
            variant.push_back(event.activity);
            metrics.activity_frequency[event.activity]++;
        }

        std::string variant_str;
        for (const auto& activity : variant) {
            if (!variant_str.empty()) variant_str += "->";
            variant_str += activity;
        }

        metrics.variant_frequency[variant_str]++;

        metrics.variant_traces[variant_str] = variant;

        for (size_t i = 0; i < events.size() - 1; ++i) {
            metrics.transition_frequency[events[i].activity][events[i + 1].activity]++;
        }
    }
    
    return metrics;
}

std::shared_ptr<ProcessGraph> FrequencyAnalyzer::build_process_graph(
    const FrequencyMetrics& metrics, double threshold) {
    
    auto graph = std::make_shared<ProcessGraph>();

    for (const auto& activity_pair : metrics.activity_frequency) {
        graph->add_node(activity_pair.first);
    }

    for (const auto& from_pair : metrics.transition_frequency) {
        for (const auto& to_pair : from_pair.second) {
            double frequency = static_cast<double>(to_pair.second);
            
            if (frequency > threshold) {
                graph->add_edge(from_pair.first, to_pair.first, frequency);
            }
        }
    }
    
    return graph;
}

ConformanceChecker::ConformanceChecker(const ProcessGraph& process_model)
    : process_model_(process_model) {}

ConformanceChecker::ConformanceResult ConformanceChecker::check_trace(const Trace& trace) {
    ConformanceResult result;
    result.total_activities = trace.get_events().size();
    result.matched_activities = 0;
    
    const auto& events = trace.get_events();

    for (size_t i = 0; i < events.size() - 1; ++i) {
        const std::string& from = events[i].activity;
        const std::string& to = events[i + 1].activity;

        auto edges = process_model_.get_outgoing_edges(from);

        bool found = false;
        for (const auto& edge : edges) {
            if (edge.to == to) {
                found = true;
                result.matched_activities++;
                break;
            }
        }
        
        if (!found) {
            std::string violation = "Transition from '" + from + "' to '" + to + "' not found in model";
            result.violations.push_back(violation);
        }
    }

    if (!events.empty()) {
        auto nodes = process_model_.get_nodes();
        if (std::find(nodes.begin(), nodes.end(), events.back().activity) != nodes.end()) {
            result.matched_activities++;
        }
    }

    if (result.total_activities > 0) {
        result.fitness = static_cast<double>(result.matched_activities) / result.total_activities;
    } else {
        result.fitness = 1.0;
    }
    
    return result;
}

std::vector<ConformanceChecker::ConformanceResult> ConformanceChecker::check_log(const EventLog& log) {
    std::vector<ConformanceResult> results;
    
    for (const auto& trace : log.get_traces()) {
        results.push_back(check_trace(trace));
    }
    
    return results;
}

double ConformanceChecker::calculate_overall_conformance(const EventLog& log) {
    auto results = check_log(log);
    
    double total_fitness = 0.0;
    for (const auto& result : results) {
        total_fitness += result.fitness;
    }
    
    return results.empty() ? 0.0 : total_fitness / results.size();
}

}
