#include "procmine/algorithm.h"
#include "procmine/log.h"
#include "procmine/database.h"
#include <iostream>
#include <fstream>

using namespace procmine;

int main() {
    EventLog log;

    Trace trace1("case1");
    
    Event e1;
    e1.activity = "A";
    e1.timestamp = std::chrono::system_clock::now();
    trace1.add_event(e1);
    
    Event e2;
    e2.activity = "B";
    e2.timestamp = std::chrono::system_clock::now() + std::chrono::seconds(1);
    trace1.add_event(e2);
    
    Event e3;
    e3.activity = "C";
    e3.timestamp = std::chrono::system_clock::now() + std::chrono::seconds(2);
    trace1.add_event(e3);
    
    Event e4;
    e4.activity = "D";
    e4.timestamp = std::chrono::system_clock::now() + std::chrono::seconds(3);
    trace1.add_event(e4);
    
    log.add_trace(trace1);

    Trace trace2("case2");
    
    Event e5;
    e5.activity = "A";
    e5.timestamp = std::chrono::system_clock::now();
    trace2.add_event(e5);
    
    Event e6;
    e6.activity = "C";
    e6.timestamp = std::chrono::system_clock::now() + std::chrono::seconds(1);
    trace2.add_event(e6);
    
    Event e7;
    e7.activity = "B";
    e7.timestamp = std::chrono::system_clock::now() + std::chrono::seconds(2);
    trace2.add_event(e7);
    
    Event e8;
    e8.activity = "D";
    e8.timestamp = std::chrono::system_clock::now() + std::chrono::seconds(3);
    trace2.add_event(e8);
    
    log.add_trace(trace2);

    AlphaAlgorithm alpha;
    auto process_graph = alpha.mine(log);

    std::string dot = process_graph->to_dot();
    std::ofstream dot_file("process_model.dot");
    dot_file << dot;
    dot_file.close();
    
    std::cout << "Process mining completed! Process model saved to process_model.dot" << std::endl;
    std::cout << "You can visualize this file using Graphviz: dot -Tpng process_model.dot -o process_model.png" << std::endl;

    HeuristicMiner heuristic(0.5, 1.0);
    auto heuristic_graph = heuristic.mine(log);

    std::string heuristic_dot = heuristic_graph->to_dot();
    std::ofstream heuristic_dot_file("heuristic_model.dot");
    heuristic_dot_file << heuristic_dot;
    heuristic_dot_file.close();
    
    std::cout << "Heuristic mining completed! Process model saved to heuristic_model.dot" << std::endl;
    
    return 0;
} 