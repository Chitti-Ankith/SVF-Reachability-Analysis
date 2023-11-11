
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
vector<vector<string>> paths;

void dfs(const ICFGNode* node, Set<NodeID>& visited, vector<string> cur_path, const string& prev_func) {
    // cout << "DFS on " << node->getId() << ", Size = " << node->getOutEdges().size() << endl;
    for (auto it = node->OutEdgeBegin(), eit = node->OutEdgeEnd(); it != eit; ++it) {
        ICFGEdge* edge = *it;
        auto dst_node = edge->getDstNode();
        auto dst_node_id = dst_node->getId();
        if (visited.count(dst_node_id)) {
            continue;
        }
        // cout << dst_node_id << endl;
        auto name = dst_node->getFun()->getName();
        // cout << name << endl;
        if (name == "sink") {
            reachable = true;
            cur_path.push_back(name);
            paths.push_back(cur_path);
            return; // check this
        }

        visited.insert(dst_node_id);

        if (name != prev_func) {
            cur_path.push_back(name);
        }
        dfs(dst_node, visited, cur_path, name);
        if (name != prev_func) {
            cur_path.pop_back();
        }
    }
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
    Set<NodeID> visited;
    visited.insert(iNode->getId());
    vector<string> cur_path;
    cur_path.push_back("src");
    dfs(iNode, visited, cur_path, "src");
    

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
                result += path[i];

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
