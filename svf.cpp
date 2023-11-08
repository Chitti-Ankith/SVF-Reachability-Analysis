#include "SVF-LLVM/LLVMUtil.h"
#include "Graphs/SVFG.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "SVFIR/SVFModule.h"
#include "Util/Options.h"

#include <iostream>
#include <vector>
#include <set>
#include <unordered_set>
#include <stack>

using namespace llvm;
using namespace std;
using namespace SVF;

static llvm::cl::opt<std::string> InputFilename(cl::Positional,
        llvm::cl::desc("<input bitcode>"), llvm::cl::init("-"));


// llvm-dis test1.bc
// opt -dot-cfg test_cases/test1.ll 

/*
 * An example to query/collect all successor nodes from a ICFGNode (iNode) along control-flow graph (ICFG)
 */
void traverseOnICFG(ICFG* icfg, const Instruction* inst)
{
    SVFInstruction* svfinst = LLVMModuleSet::getLLVMModuleSet()->getSVFInstruction(inst);

    ICFGNode* iNode = icfg->getFunEntryICFGNode(svfinst);
    FIFOWorkList<const ICFGNode*> worklist;
    Set<const ICFGNode*> visited;
    worklist.push(iNode);

    /// Traverse along VFG
    while (!worklist.empty())
    {
        const ICFGNode* iNode = worklist.pop();
        for (ICFGNode::const_iterator it = iNode->OutEdgeBegin(), eit =
                    iNode->OutEdgeEnd(); it != eit; ++it)
        {
            ICFGEdge* edge = *it;
            ICFGNode* succNode = edge->getDstNode();
            if (visited.find(succNode) == visited.end())
            {
                visited.insert(succNode);
                worklist.push(succNode);
            }
        }
    }
}


for (auto it = iNode->OutEdgeBegin(), eit =
                iNode->OutEdgeEnd(); it != eit; ++it) {
        ICFGEdge* edge = *it;
        auto name = edge->getDstNode()->getFun()->getName();
        cout << name << endl;
        // if (visited.find(succNode) == visited.end())
        // {
        //     visited.insert(succNode);
        //     worklist.push(succNode);
        // }
}


// Function to perform context-sensitive DFS reachability analysis and detect cycles
bool findAllPathsDFS(const ICFG& icfg, Node* current, Node* sink, std::unordered_set<Node*>& visited, std::vector<Node*>& currentPath, std::vector<std::vector<Node*>>& allPaths) {
    // Mark the current node as visited
    visited.insert(current);

    // Add the current node to the current path
    currentPath.push_back(current);

    // Check if the current node is the sink
    if (current == sink) {
        // If the sink is reached, add the current path to allPaths
        allPaths.push_back(currentPath);
        return true; // Indicate that the sink is reached
    } else {
        // Process successors of the current node in a context-sensitive manner
        for (Node* successor : icfg.getSuccessors(current)) {
            // Check contextual information using SVF APIs
            // Add your context-sensitive conditions here

            // If the successor has not been visited, recursively call DFS
            if (visited.find(successor) == visited.end()) {
                if (findAllPathsDFS(icfg, successor, sink, visited, currentPath, allPaths)) {
                    return true; // Sink is reached in the current path
                }
            } else {
                // Detected a cycle, annotate the cyclic region
                auto it = std::find(currentPath.begin(), currentPath.end(), successor);
                if (it != currentPath.end()) {
                    std::cout << "Cycle[";
                    for (; it != currentPath.end(); ++it) {
                        std::cout << (*it)->getName(); // Adjust this based on your actual Node representation
                        if (std::next(it) != currentPath.end()) {
                            std::cout << " --> ";
                        }
                    }
                    std::cout << "]";
                    return true; // Stop further traversal for this path
                }
            }
        }
    }

    // Backtrack: remove the current node from the current path
    currentPath.pop_back();

    // Unmark the current node as visited for other paths
    visited.erase(current);

    return false; // Sink is not reached in the current path
}

// Wrapper function for DFS reachability analysis and cycle detection
void findAllPathsContextSensitiveDFS(const ICFG& icfg, Node* source, Node* sink) {
    std::unordered_set<Node*> visited;
    std::vector<Node*> currentPath;
    std::vector<std::vector<Node*>> allPaths;

    findAllPathsDFS(icfg, source, sink, visited, currentPath, allPaths);
}

// Function to print a path in the specified format
void printPath(const std::vector<Node*>& path) {
    for (size_t i = 0; i < path.size(); ++i) {
        std::cout << path[i]->getName(); // Adjust this based on your actual Node representation
        if (i < path.size() - 1) {
            std::cout << " --> ";
        }
    }
    std::cout << "\n";
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

    // Find all paths from source to sink and detect cycles
    findAllPathsContextSensitiveDFS(icfg, source, sink);


    SVFIR::releaseSVFIR();

    SVF::LLVMModuleSet::releaseLLVMModuleSet();
    llvm::llvm_shutdown();

    return 0;
}
