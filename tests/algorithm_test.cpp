#include <gtest/gtest.h>
#include "procmine/algorithm.h"
#include "procmine/models.h"
#include <chrono>

namespace {
    using namespace procmine;
    using namespace std::chrono;

    EventLog create_test_log() {
        EventLog log;

        Trace trace1("case1");
        
        Event e1;
        e1.activity = "A";
        e1.timestamp = system_clock::now();
        trace1.add_event(e1);
        
        Event e2;
        e2.activity = "B";
        e2.timestamp = system_clock::now() + seconds(1);
        trace1.add_event(e2);
        
        Event e3;
        e3.activity = "C";
        e3.timestamp = system_clock::now() + seconds(2);
        trace1.add_event(e3);
        
        Event e4;
        e4.activity = "D";
        e4.timestamp = system_clock::now() + seconds(3);
        trace1.add_event(e4);
        
        log.add_trace(trace1);

        Trace trace2("case2");
        
        Event e5;
        e5.activity = "A";
        e5.timestamp = system_clock::now();
        trace2.add_event(e5);
        
        Event e6;
        e6.activity = "C";
        e6.timestamp = system_clock::now() + seconds(1);
        trace2.add_event(e6);
        
        Event e7;
        e7.activity = "B";
        e7.timestamp = system_clock::now() + seconds(2);
        trace2.add_event(e7);
        
        Event e8;
        e8.activity = "D";
        e8.timestamp = system_clock::now() + seconds(3);
        trace2.add_event(e8);
        
        log.add_trace(trace2);
        
        return log;
    }
}

TEST(AlgorithmTest, AlphaAlgorithm) {
    EventLog log = create_test_log();

    AlphaAlgorithm alpha;

    auto process_graph = alpha.mine(log);

    ASSERT_NE(process_graph, nullptr);

    auto nodes = process_graph->get_nodes();
    EXPECT_EQ(nodes.size(), 4);

    auto edges_from_a = process_graph->get_outgoing_edges("A");

    bool has_edge_to_b = false;
    bool has_edge_to_c = false;
    
    for (const auto& edge : edges_from_a) {
        if (edge.to == "B") has_edge_to_b = true;
        if (edge.to == "C") has_edge_to_c = true;
    }
    
    EXPECT_TRUE(has_edge_to_b);
    EXPECT_TRUE(has_edge_to_c);
}

TEST(AlgorithmTest, HeuristicMiner) {
    EventLog log = create_test_log();

    HeuristicMiner miner(0.5, 1.0);

    auto process_graph = miner.mine(log);

    ASSERT_NE(process_graph, nullptr);

    auto nodes = process_graph->get_nodes();
    EXPECT_EQ(nodes.size(), 4);

    std::string dot = process_graph->to_dot();
    EXPECT_FALSE(dot.empty());
} 