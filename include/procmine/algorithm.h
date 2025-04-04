#pragma once

#include "procmine/models.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace procmine {

class ProcessGraph {
public:
    ProcessGraph();

    void add_node(const std::string& activity);

    void add_edge(const std::string& from, const std::string& to, double weight = 1.0);

    std::vector<std::string> get_nodes() const;

    struct Edge {
        std::string from;
        std::string to;
        double weight;
    };
    
    std::vector<Edge> get_outgoing_edges(const std::string& node) const;

    std::string to_dot() const;
    
private:
    std::unordered_set<std::string> nodes_;
    std::unordered_map<std::string, std::vector<Edge>> edges_;
};

class MiningAlgorithm {
public:
    virtual ~MiningAlgorithm() = default;
    virtual std::shared_ptr<ProcessGraph> mine(const EventLog& log) = 0;
};

class AlphaAlgorithm : public MiningAlgorithm {
public:
    AlphaAlgorithm();
    std::shared_ptr<ProcessGraph> mine(const EventLog& log) override;
};

class HeuristicMiner : public MiningAlgorithm {
public:
    HeuristicMiner(double dependency_threshold = 0.9, 
                  double positive_observations_threshold = 1.0);
    std::shared_ptr<ProcessGraph> mine(const EventLog& log) override;
    
private:
    double dependency_threshold_;
    double positive_observations_threshold_;
};

class FrequencyAnalyzer {
public:
    FrequencyAnalyzer();

    struct FrequencyMetrics {
        std::unordered_map<std::string, int> activity_frequency;
        std::unordered_map<std::string, std::unordered_map<std::string, int>> transition_frequency;
        std::unordered_map<std::string, std::vector<std::string>> variant_traces;
        std::unordered_map<std::string, int> variant_frequency;
    };
    
    FrequencyMetrics analyze(const EventLog& log);

    std::shared_ptr<ProcessGraph> build_process_graph(const FrequencyMetrics& metrics,
                                                    double threshold = 0.0);
};

class ConformanceChecker {
public:
    ConformanceChecker(const ProcessGraph& process_model);

    struct ConformanceResult {
        double fitness;
        int matched_activities;
        int total_activities;
        std::vector<std::string> violations;
    };
    
    ConformanceResult check_trace(const Trace& trace);

    std::vector<ConformanceResult> check_log(const EventLog& log);

    double calculate_overall_conformance(const EventLog& log);
    
private:
    const ProcessGraph& process_model_;
};

}