#include "procmine/algorithm.h"
#include <algorithm>
#include <sstream>
#include <iostream>

namespace procmine {

// ProcessGraph implementation
ProcessGraph::ProcessGraph() {}

void ProcessGraph::add_node(const std::string& activity) {
    nodes_.insert(activity);
}

void ProcessGraph::add_edge(const std::string& from, const std::string& to, double weight) {
    // Добавляем узлы, если их еще нет
    add_node(from);
    add_node(to);
    
    // Добавляем ребро
    Edge edge;
    edge.from = from;
    edge.to = to;
    edge.weight = weight;
    
    edges_[from].push_back(edge);
}

std::vector<std::string> ProcessGraph::get_nodes() const {
    return std::vector<std::string>(nodes_.begin(), nodes_.end());
}

std::vector<ProcessGraph::Edge> ProcessGraph::get_outgoing_edges(const std::string& node) const {
    auto it = edges_.find(node);
    if (it != edges_.end()) {
        return it->second;
    }
    return {};
}

std::string ProcessGraph::to_dot() const {
    std::stringstream ss;
    ss << "digraph ProcessModel {\n";
    
    // Добавляем узлы
    for (const auto& node : nodes_) {
        ss << "  \"" << node << "\" [shape=box];\n";
    }
    
    // Добавляем ребра
    for (const auto& edge_list : edges_) {
        for (const auto& edge : edge_list.second) {
            ss << "  \"" << edge.from << "\" -> \"" << edge.to 
               << "\" [label=\"" << edge.weight << "\"];\n";
        }
    }
    
    ss << "}\n";
    return ss.str();
}

// AlphaAlgorithm implementation
AlphaAlgorithm::AlphaAlgorithm() {}

std::shared_ptr<ProcessGraph> AlphaAlgorithm::mine(const EventLog& log) {
    auto result = std::make_shared<ProcessGraph>();
    
    // Получаем все активности
    auto activities = log.get_activities();
    
    // Добавляем узлы в граф
    for (const auto& activity : activities) {
        result->add_node(activity);
    }
    
    // Для каждого трейса находим отношения следования
    for (const auto& trace : log.get_traces()) {
        const auto& events = trace.get_events();
        for (size_t i = 0; i < events.size() - 1; ++i) {
            result->add_edge(events[i].activity, events[i + 1].activity);
        }
    }
    
    return result;
}

// HeuristicMiner implementation
HeuristicMiner::HeuristicMiner(double dependency_threshold, 
                             double positive_observations_threshold)
    : dependency_threshold_(dependency_threshold),
      positive_observations_threshold_(positive_observations_threshold) {}

std::shared_ptr<ProcessGraph> HeuristicMiner::mine(const EventLog& log) {
    auto result = std::make_shared<ProcessGraph>();
    
    // Получаем все активности
    auto activities = log.get_activities();
    
    // Добавляем узлы в граф
    for (const auto& activity : activities) {
        result->add_node(activity);
    }
    
    // Подсчитываем частоты переходов
    std::unordered_map<std::string, std::unordered_map<std::string, int>> transitions;
    
    for (const auto& trace : log.get_traces()) {
        const auto& events = trace.get_events();
        for (size_t i = 0; i < events.size() - 1; ++i) {
            transitions[events[i].activity][events[i + 1].activity]++;
        }
    }
    
    // Вычисляем метрики зависимости и добавляем ребра
    for (const auto& from_activity : activities) {
        for (const auto& to_activity : activities) {
            if (from_activity == to_activity) continue;
            
            int a_to_b = transitions[from_activity][to_activity];
            int b_to_a = transitions[to_activity][from_activity];
            
            // Вычисляем метрику зависимости
            double dependency = 0.0;
            if (a_to_b + b_to_a > 0) {
                dependency = ((double)(a_to_b - b_to_a)) / ((double)(a_to_b + b_to_a + 1));
            }
            
            // Добавляем ребро, если метрика выше порога
            if (dependency > dependency_threshold_ && a_to_b > positive_observations_threshold_) {
                result->add_edge(from_activity, to_activity, dependency);
            }
        }
    }
    
    return result;
}

// FrequencyAnalyzer implementation
FrequencyAnalyzer::FrequencyAnalyzer() {}

FrequencyAnalyzer::FrequencyMetrics FrequencyAnalyzer::analyze(const EventLog& log) {
    FrequencyMetrics metrics;
    
    // Подсчитываем частоты активностей и переходов
    for (const auto& trace : log.get_traces()) {
        const auto& events = trace.get_events();
        
        // Создаем trace variant (последовательность активностей)
        std::vector<std::string> variant;
        for (const auto& event : events) {
            variant.push_back(event.activity);
            metrics.activity_frequency[event.activity]++;
        }
        
        // Преобразуем вариант в строку
        std::string variant_str;
        for (const auto& activity : variant) {
            if (!variant_str.empty()) variant_str += "->";
            variant_str += activity;
        }
        
        // Увеличиваем счетчик этого варианта
        metrics.variant_frequency[variant_str]++;
        
        // Сохраняем вариант трейса
        metrics.variant_traces[variant_str] = variant;
        
        // Подсчитываем переходы
        for (size_t i = 0; i < events.size() - 1; ++i) {
            metrics.transition_frequency[events[i].activity][events[i + 1].activity]++;
        }
    }
    
    return metrics;
}

std::shared_ptr<ProcessGraph> FrequencyAnalyzer::build_process_graph(
    const FrequencyMetrics& metrics, double threshold) {
    
    auto graph = std::make_shared<ProcessGraph>();
    
    // Добавляем все активности как узлы
    for (const auto& activity_pair : metrics.activity_frequency) {
        graph->add_node(activity_pair.first);
    }
    
    // Добавляем переходы как ребра
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

// ConformanceChecker implementation
ConformanceChecker::ConformanceChecker(const ProcessGraph& process_model)
    : process_model_(process_model) {}

ConformanceChecker::ConformanceResult ConformanceChecker::check_trace(const Trace& trace) {
    ConformanceResult result;
    result.total_activities = trace.get_events().size();
    result.matched_activities = 0;
    
    const auto& events = trace.get_events();
    
    // Проверяем каждый переход
    for (size_t i = 0; i < events.size() - 1; ++i) {
        const std::string& from = events[i].activity;
        const std::string& to = events[i + 1].activity;
        
        // Получаем все исходящие ребра из текущей активности
        auto edges = process_model_.get_outgoing_edges(from);
        
        // Ищем ребро к следующей активности
        bool found = false;
        for (const auto& edge : edges) {
            if (edge.to == to) {
                found = true;
                result.matched_activities++;
                break;
            }
        }
        
        if (!found) {
            // Нарушение: переход не найден в модели
            std::string violation = "Transition from '" + from + "' to '" + to + "' not found in model";
            result.violations.push_back(violation);
        }
    }
    
    // Добавляем последнюю активность, если она есть в модели
    if (!events.empty()) {
        auto nodes = process_model_.get_nodes();
        if (std::find(nodes.begin(), nodes.end(), events.back().activity) != nodes.end()) {
            result.matched_activities++;
        }
    }
    
    // Вычисляем общую оценку соответствия (fitness)
    if (result.total_activities > 0) {
        result.fitness = static_cast<double>(result.matched_activities) / result.total_activities;
    } else {
        result.fitness = 1.0; // Пустой трейс считается полностью соответствующим
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

} // namespace procmine 