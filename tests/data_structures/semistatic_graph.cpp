// expect-success
/*
 * Copyright 2014 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define IN_FRUIT_CPP_FILE

#include <fruit/impl/data_structures/semistatic_graph.h>
#include <fruit/impl/data_structures/semistatic_graph.templates.h>

#include <vector>

using namespace std;
using namespace fruit::impl;

using Graph = SemistaticGraph<int, const char*>;
using node_iterator = Graph::node_iterator;
using edge_iterator = Graph::edge_iterator;

vector<size_t> no_neighbors = {};

struct SimpleNode {
  size_t id;
  const char* value;
  const vector<size_t>* neighbors;
  bool is_terminal;
  
  size_t getId() { return id; }
  const char* getValue() { return value; }
  bool isTerminal() { return is_terminal; }
  vector<size_t>::const_iterator getEdgesBegin() { return neighbors->begin(); }
  vector<size_t>::const_iterator getEdgesEnd() { return neighbors->end(); }
};


void test_empty() {
  vector<SimpleNode> values{};
  Graph graph(values.begin(), values.end());
  assert(graph.find(0) == graph.end());
  assert(graph.find(2) == graph.end());
  assert(graph.find(5) == graph.end());
}

void test_1_node_no_edges() {
  vector<SimpleNode> values{{2, "foo", &no_neighbors, false}};
  Graph graph(values.begin(), values.end());
  assert(graph.find(0) == graph.end());
  assert(!(graph.find(2) == graph.end()));
  assert(graph.at(2).getNode() == string("foo"));
  assert(graph.at(2).isTerminal() == false);
  assert(graph.find(5) == graph.end());
}

void test_1_node_no_edges_terminal() {
  vector<SimpleNode> values{{2, "foo", &no_neighbors, true}};
  Graph graph(values.begin(), values.end());
  assert(graph.find(0) == graph.end());
  assert(!(graph.find(2) == graph.end()));
  assert(graph.at(2).getNode() == string("foo"));
  assert(graph.at(2).isTerminal() == true);
  assert(graph.find(5) == graph.end());
}

void test_1_node_self_edge() {
  vector<size_t> neighbors = {2};
  vector<SimpleNode> values{{2, "foo", &neighbors, false}};
  Graph graph(values.begin(), values.end());
  assert(graph.find(0) == graph.end());
  assert(!(graph.find(2) == graph.end()));
  assert(graph.at(2).getNode() == string("foo"));
  assert(graph.at(2).isTerminal() == false);
  edge_iterator itr = graph.at(2).neighborsBegin();
  (void)itr;
  assert(itr.getNodeIterator(graph).getNode() == string("foo"));
  assert(itr.getNodeIterator(graph).isTerminal() == false);
  assert(graph.find(5) == graph.end());
}

void test_2_nodes_one_edge() {
  vector<size_t> neighbors = {2};
  vector<SimpleNode> values{{2, "foo", &no_neighbors, false}, {3, "bar", &neighbors, false}};
  Graph graph(values.begin(), values.end());
  assert(graph.find(0) == graph.end());
  assert(!(graph.find(2) == graph.end()));
  assert(graph.at(2).getNode() == string("foo"));
  assert(graph.at(2).isTerminal() == false);
  assert(graph.at(3).getNode() == string("bar"));
  assert(graph.at(3).isTerminal() == false);
  edge_iterator itr = graph.at(3).neighborsBegin();
  (void)itr;
  assert(itr.getNodeIterator(graph).getNode() == string("foo"));
  assert(itr.getNodeIterator(graph).isTerminal() == false);
  assert(graph.find(5) == graph.end());
}

void test_3_nodes_two_edges() {
  vector<size_t> neighbors = {2, 4};
  vector<SimpleNode> values{{2, "foo", &no_neighbors, false}, {3, "bar", &neighbors, false}, {4, "baz", &no_neighbors, true}};
  Graph graph(values.begin(), values.end());
  assert(graph.find(0) == graph.end());
  assert(!(graph.find(2) == graph.end()));
  assert(graph.at(2).getNode() == string("foo"));
  assert(graph.at(2).isTerminal() == false);
  assert(graph.at(3).getNode() == string("bar"));
  assert(graph.at(3).isTerminal() == false);
  edge_iterator itr = graph.at(3).neighborsBegin();
  assert(itr.getNodeIterator(graph).getNode() == string("foo"));
  assert(itr.getNodeIterator(graph).isTerminal() == false);
  ++itr;
  assert(itr.getNodeIterator(graph).getNode() == string("baz"));
  assert(itr.getNodeIterator(graph).isTerminal() == true);
  assert(graph.at(4).getNode() == string("baz"));
  assert(graph.at(4).isTerminal() == true);
  assert(graph.find(5) == graph.end());
}

void test_add_node() {
  vector<SimpleNode> old_values{{2, "foo", &no_neighbors, false}, {4, "baz", &no_neighbors, true}};
  Graph old_graph(old_values.begin(), old_values.end());
  vector<size_t> neighbors = {2, 4};
  vector<SimpleNode> new_values{{3, "bar", &neighbors, false}};
  Graph graph(old_graph, new_values.begin(), new_values.end());
  assert(graph.find(0) == graph.end());
  assert(!(graph.find(2) == graph.end()));
  assert(graph.at(2).getNode() == string("foo"));
  assert(graph.at(2).isTerminal() == false);
  assert(graph.at(3).getNode() == string("bar"));
  assert(graph.at(3).isTerminal() == false);
  edge_iterator itr = graph.at(3).neighborsBegin();
  assert(itr.getNodeIterator(graph).getNode() == string("foo"));
  assert(itr.getNodeIterator(graph).isTerminal() == false);
  ++itr;
  assert(itr.getNodeIterator(graph).getNode() == string("baz"));
  assert(itr.getNodeIterator(graph).isTerminal() == true);
  assert(graph.at(4).getNode() == string("baz"));
  assert(graph.at(4).isTerminal() == true);
  assert(graph.find(5) == graph.end());
}

void test_set_terminal() {
  vector<size_t> neighbors = {2, 4};
  vector<SimpleNode> values{{2, "foo", &no_neighbors, false}, {3, "bar", &neighbors, false}, {4, "baz", &no_neighbors, true}};
  Graph graph(values.begin(), values.end());
  graph.changeNodeToTerminal(3);
  assert(graph.find(0) == graph.end());
  assert(!(graph.find(2) == graph.end()));
  assert(graph.at(2).getNode() == string("foo"));
  assert(graph.at(2).isTerminal() == false);
  assert(graph.at(3).getNode() == string("bar"));
  assert(graph.at(3).isTerminal() == true);
  assert(graph.at(4).getNode() == string("baz"));
  assert(graph.at(4).isTerminal() == true);
  assert(graph.find(5) == graph.end());
}

void test_move_constructor() {
  vector<size_t> neighbors = {2};
  vector<SimpleNode> values{{2, "foo", &no_neighbors, false}, {3, "bar", &neighbors, false}};
  Graph graph1(values.begin(), values.end());
  Graph graph = std::move(graph1);
  assert(graph.find(0) == graph.end());
  assert(!(graph.find(2) == graph.end()));
  assert(graph.at(2).getNode() == string("foo"));
  assert(graph.at(2).isTerminal() == false);
  assert(graph.at(3).getNode() == string("bar"));
  assert(graph.at(3).isTerminal() == false);
  edge_iterator itr = graph.at(3).neighborsBegin();
  (void)itr;
  assert(itr.getNodeIterator(graph).getNode() == string("foo"));
  assert(itr.getNodeIterator(graph).isTerminal() == false);
  assert(graph.find(5) == graph.end());
}

void test_move_assignment() {
  vector<size_t> neighbors = {2};
  vector<SimpleNode> values{{2, "foo", &no_neighbors, false}, {3, "bar", &neighbors, false}};
  Graph graph1(values.begin(), values.end());
  Graph graph;
  graph = std::move(graph1);
  assert(graph.find(0) == graph.end());
  assert(!(graph.find(2) == graph.end()));
  assert(graph.at(2).getNode() == string("foo"));
  assert(graph.at(2).isTerminal() == false);
  assert(graph.at(3).getNode() == string("bar"));
  assert(graph.at(3).isTerminal() == false);
  edge_iterator itr = graph.at(3).neighborsBegin();
  (void)itr;
  assert(itr.getNodeIterator(graph).getNode() == string("foo"));
  assert(itr.getNodeIterator(graph).isTerminal() == false);
  assert(graph.find(5) == graph.end());
}

int main() {
  
  test_empty();
  test_1_node_no_edges();
  test_1_node_no_edges_terminal();
  test_1_node_self_edge();
  test_2_nodes_one_edge();
  test_3_nodes_two_edges();
  test_add_node();
  test_set_terminal();
  test_move_constructor();
  test_move_assignment();
  
  return 0;
}
