
//===- SVF-Project -------------------------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013->  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===-----------------------------------------------------------------------===//
#include "SVF-LLVM/LLVMUtil.h"
#include "Graphs/SVFG.h"
#include "Graphs/ICFGNode.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "SVFIR/SVFModule.h"
#include "Util/Options.h"

#include <iostream>
#include <vector>
#include <set>

using namespace llvm;
using namespace std;
using namespace SVF;

static llvm::cl::opt<std::string> InputFilename(cl::Positional,
        llvm::cl::desc("<input bitcode>"), llvm::cl::init("-"));

bool reachable = false;
vector<vector<int>> paths;
vector<vector<int>> cycles;

void dfs(const ICFGNode* node, int parent_id, vector<int>& visited, Map<NodeID, int>& parent, vector<int>& cur_path, const string& cur_func) {

    if (cur_func == "sink") {
        reachable = true;
        paths.push_back(cur_path);
        return;
    }

    for (auto it = node->OutEdgeBegin(), eit = node->OutEdgeEnd(); it != eit; ++it) {
        ICFGEdge* edge = *it;
        auto dst_node = edge->getDstNode();
        auto dst_node_id = dst_node->getId();
        auto name = dst_node->getFun()->getName();
        
        if (find(visited.begin(), visited.end(), dst_node_id) != visited.end()) {
            // Cycle detected.
            auto copy = visited;
            cycles.push_back(move(copy));
            continue;
        }

        cur_path.push_back(dst_node_id);
        visited.push_back(dst_node_id);

        dfs(dst_node, node->getId(), visited, parent, cur_path, name);

        cur_path.pop_back();
        visited.pop_back();
    }
}

void print_cyclic_path(vector<int>& path) {
    string result;
    bool cycle_init = false;
    for (int i = 0; i < path.size(); ++i) {
        if (path[i] == -1) {
            if (cycle_init) {
               result += "]";
               cycle_init = false;
            } else {
                result += ((i == 0) ? "Cycle[" : "-->Cycle[");
                result += to_string(path[++i]);
                cycle_init = true;
            }
            continue;
        } else {
            if (i != 0) {
                result += "-->";
            }
        
            result += to_string(path[i]);
        }
    }
    cout << result << endl;
}


int main(int argc, char ** argv) {

    int arg_num = 0;
    char **arg_value = new char*[argc];
    std::vector<std::string> moduleNameVec;
    SVF::LLVMUtil::processArguments(argc, argv, arg_num, arg_value, moduleNameVec);
    cl::ParseCommandLineOptions(arg_num, arg_value,
                                "Whole Program Points-to Analysis\n");

    SVFModule* svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);

    // Build Program Assignment Graph (SVFIR)
    SVFIRBuilder builder(svfModule);
    SVFIR* pag = builder.build();

    // ICFG
    ICFG* icfg = pag->getICFG();

    auto src = svfModule->getSVFFunction("src");
    auto sink = svfModule->getSVFFunction("sink");

    if (!src || !sink) {
        cout << "Unreachable" << endl;
        SVFIR::releaseSVFIR();

        SVF::LLVMModuleSet::releaseLLVMModuleSet();
        llvm::llvm_shutdown();
        return 0;
    }

    auto iNode = icfg->getFunEntryICFGNode(src);
    vector<int> visited;
    Map<NodeID, int> parent;
    parent[iNode->getId()] = 0;
    visited.push_back(iNode->getId());
    vector<int> cur_path;
    cur_path.push_back(iNode->getId());
    // Identify all paths.
    dfs(iNode, iNode->getId(), visited, parent, cur_path, "src");

    cout << (reachable ? "Reachable" : "Unreachable") << endl;

    if (paths.size()) {
        // First print all paths without including cycles.     
        for (const auto& path : paths) {
            string result;
            for (int i = 0; i < path.size(); ++i) {
                result += to_string(path[i]);

                if (i != path.size() - 1) {
                    result += "-->";
                }
            }
            cout << result << endl;
        }

        // Combine cycles into the paths.
        if (cycles.size()) {
            for (const auto& cycle : cycles) {
                for (const auto& path : paths) {
                    for (int i = 0; i < path.size(); ++i) {
                        deque<int> temp_cycle(cycle.begin(), cycle.end()); // Make a copy of the cycle to modify.
                        vector<int> temp_path = path;

                        auto it = find(cycle.begin(), cycle.end(), path[i]);
                        if (it == cycle.end()) {
                            continue;
                        }

                        // Get the index of the element.
                        int index = it - cycle.begin();

                        // Remove all elements from the temp_cycle up until the index and push them to the back.
                        for (int j = 0; j < index; ++j) {
                            temp_cycle.push_back(temp_cycle.front());
                            temp_cycle.pop_front();
                        }

                        temp_cycle.push_back(temp_cycle.front());
                        // Add markers to signify start and end of cycle.
                        temp_cycle.push_front(-1);
                        temp_cycle.push_back(-1);

                        // Merge cycle into the path.
                        auto path_it = find(temp_path.begin(), temp_path.end(), path[i]);
                        if (path_it == temp_path.end()) {
                            continue;
                        }
                        auto path_index = path_it - temp_path.begin();
                        temp_path.erase(path_it);
                        temp_path.insert(path_index + temp_path.begin(), temp_cycle.begin(), temp_cycle.end());

                        print_cyclic_path(temp_path);
                    }
                }
            }
        }
    }

    SVFIR::releaseSVFIR();

    SVF::LLVMModuleSet::releaseLLVMModuleSet();
    llvm::llvm_shutdown();

    return 0;
}
