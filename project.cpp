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

    cout << "DFS on " << node->getId() << ":" << cur_func << ", Parent " << parent_id << ", Current Nodes size = " << visited.size() << ", Output edges size = " << node->getOutEdges().size() << endl;

    if (cur_func == "sink") {
        reachable = true;
        // cur_path.push_back(name);
        paths.push_back(cur_path);
        return;
    }

    // if (visited[node->getId()] == 2) {
    //     return;
    // }

    // if (visited[node->getId()] == 1) {
    //     vector<int> v;
    //     int cur = parent_id;
    //     v.push_back(cur);

    //     while (cur != node->getId()) {
    //         cur = parent[cur];
    //         v.push_back(cur);
    //     }
    //     reverse(v.begin(), v.end());
    //     cycles.push_back(move(v));
    //     return;
    // }

    // parent[node->getId()] = parent_id;

    // visited[node->getId()] = 1;
    // visited.insert(node->getId());

    for (auto it = node->OutEdgeBegin(), eit = node->OutEdgeEnd(); it != eit; ++it) {
        ICFGEdge* edge = *it;
        auto dst_node = edge->getDstNode();
        auto dst_node_id = dst_node->getId();
        auto name = dst_node->getFun()->getName();

        cout << "Dest : " << dst_node_id << ":" << name << ", Parent : " << node->getId() << endl;
        
        if (find(visited.begin(), visited.end(), dst_node_id) != visited.end()) {
            // Cycle detected.
            auto copy = visited;
            // reverse(copy.begin(), copy.end());
            cycles.push_back(move(copy));
            continue;
        }

        // if (dst_node_id == parent[node->getId()]) {
        //     continue;
        // }

        // if (visited.count(dst_node_id)) {
        //     continue;
        // }

        // visited.insert(dst_node_id);

        cur_path.push_back(dst_node_id);
        visited.push_back(dst_node_id);

        dfs(dst_node, node->getId(), visited, parent, cur_path, name);

        cur_path.pop_back();
        visited.pop_back();
        // visited.erase(dst_node_id);
    }

    // visited.erase(node->getId());
    // visited[node->getId()] = 2;
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
    // cout << (src ? "Yes" : "No") << endl;
    // cout << (sink ? "Yes" : "No") << endl;

    if (!src || !sink) {
        cout << "Unreachable" << endl;
        SVFIR::releaseSVFIR();

        SVF::LLVMModuleSet::releaseLLVMModuleSet();
        llvm::llvm_shutdown();
        return 0;
    }

    auto iNode = icfg->getFunEntryICFGNode(src);
    // FIFOWorkList<const ICFGNode*> worklist;
    // worklist.push(iNode);
    // Map<NodeID, int> visited;
    vector<int> visited;
    Map<NodeID, int> parent;
    parent[iNode->getId()] = 0;
    // visited.insert(iNode->getId());
    visited.push_back(iNode->getId());
    vector<int> cur_path;
    cur_path.push_back(iNode->getId());
    dfs(iNode, iNode->getId(), visited, parent, cur_path, "src");
    

    // // Traverse along ICFG using BFS
    // while (!worklist.empty()) {
    //     const auto iNode = worklist.pop();
    //     for (auto it = iNode->OutEdgeBegin(), eit = iNode->OutEdgeEnd(); it != eit; ++it) {
    //         ICFGEdge* edge = *it;
    //         auto dst_node = edge->getDstNode();
    //         auto dst_node_id = dst_node->getId();
    //         if (visited.count(dst_node_id)) {
    //             continue;
    //         }
    //         // cout << dst_node_id << endl;
    //         auto name = dst_node->getFun()->getName();
    //         // cout << name << endl;
    //         if (name == "sink") {
    //             reachable = true;
    //         }

    //         visited.insert(dst_node_id);
    //         worklist.push(dst_node);
    //     }
    // }

    cout << (reachable ? "Reachable" : "Unreachable") << endl;

    if (paths.size()) {
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
    }

    if (cycles.size()) {
        for (const auto& path : cycles) {
            string result;
            for (int i = 0; i < path.size(); ++i) {
                result += to_string(path[i]);

                if (i != path.size() - 1) {
                    result += "-->";
                }
            }
            cout << result << endl;
        }
    }

    SVFIR::releaseSVFIR();

    SVF::LLVMModuleSet::releaseLLVMModuleSet();
    llvm::llvm_shutdown();

    return 0;
}
