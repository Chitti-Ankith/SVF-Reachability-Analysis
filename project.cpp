
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
    cout << (src ? "Yes" : "No") << endl;
    cout << (sink ? "Yes" : "No") << endl;


    auto iNode = icfg->getICFGNode(src);
    // FIFOWorkList<const ICFGNode*> worklist;
    // Set<const ICFGNode*> visited;
    // worklist.push(iNode);

    /// Traverse along VFG
    // while (!worklist.empty())
    // {
        // const ICFGNode* iNode = worklist.pop();
    for (auto it = iNode->OutEdgeBegin(), eit =
                iNode->OutEdgeEnd(); it != eit; ++it)
    {
        ICFGEdge* edge = *it;
        auto name = edge->getDstNode()->getFun()->getName();
        cout << name << endl;
        // if (visited.find(succNode) == visited.end())
        // {
        //     visited.insert(succNode);
        //     worklist.push(succNode);
        // }
    }
    // }


    SVFIR::releaseSVFIR();

    SVF::LLVMModuleSet::releaseLLVMModuleSet();
    llvm::llvm_shutdown();

    cout << "Chitti" << endl;

    return 0;
}
