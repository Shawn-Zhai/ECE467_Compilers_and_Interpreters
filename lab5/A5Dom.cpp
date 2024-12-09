//
//  A5Dom.cpp
//  ECE467 Lab 5
//
//  Created by Tarek Abdelrahman on 2023-11-18.
//  Copyright © 2023 Tarek Abdelrahman. All rights reserved.
//
//  Permission is hereby granted to use this code in ECE467 at
//  the University of Toronto. It is prohibited to distribute
//  this code, either publicly or to third parties.
//

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <map>
#include <vector>

using namespace llvm;
using namespace std;

namespace {

// This method implements what the pass does
void processFunction(Function &F) {

    // ECE467 STUDENT: add your code here

    map<BasicBlock*, vector<BasicBlock*>> dominators;
    map<string, vector<string>> dominatorNames;

    map<BasicBlock*, BasicBlock*> immediateDominators;
    map<string, string> iDomNames;

    map<BasicBlock*, vector<BasicBlock*>> postDominators;
    map<string, vector<string>> postDominatorNames;

    map<BasicBlock*, BasicBlock*> immediatePostDominators;
    map<string, string> iPostDomNames;

    BasicBlock* entry = &F.getEntryBlock();

    dominators[entry].push_back(entry);

    for (auto& BB : F) {
        if (&BB != entry) {
            for (auto& PBB : F) {
                dominators[&BB].push_back(&PBB);
            }
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;

        for (auto& BB : F) {
            if (&BB == entry) continue;

            vector<BasicBlock*> intersectDoms;
            auto pred = pred_begin(&BB);
            if (pred == pred_end(&BB)) continue;

            intersectDoms = dominators[*pred];

            for (pred = std::next(pred); pred != pred_end(&BB); ++pred) {
                vector<BasicBlock*> temp;
                for (auto dom : intersectDoms) {
                    if (std::find(dominators[*pred].begin(), dominators[*pred].end(), dom) != dominators[*pred].end()) {
                        temp.push_back(dom);
                    }
                }
                intersectDoms = temp;
            }

            intersectDoms.push_back(&BB);

            std::sort(dominators[&BB].begin(), dominators[&BB].end());
            std::sort(intersectDoms.begin(), intersectDoms.end());
            if (dominators[&BB] != intersectDoms) {
                dominators[&BB] = intersectDoms;
                changed = true;
            }
        }
    }

    map<BasicBlock*, vector<BasicBlock*>> properDominators;

    for (auto& pair : dominators) {
        BasicBlock* BB = pair.first;
        vector<BasicBlock*> properDoms = pair.second;
        properDoms.erase(std::remove(properDoms.begin(), properDoms.end(), BB), properDoms.end());
        properDominators[BB] = properDoms;
    }

    for (auto& pair : properDominators) {
        BasicBlock* BB = pair.first;
        vector<BasicBlock*> properDoms = pair.second;

        vector<BasicBlock*> filteredDoms;

        for (BasicBlock* candidate : properDoms) {
            bool dominatesOthers = false;

            for (BasicBlock* other : properDoms) {
                if (candidate != other &&
                    std::find(properDominators[other].begin(), properDominators[other].end(), candidate) != properDominators[other].end()) {
                    dominatesOthers = true;
                    break;
                }
            }

            if (!dominatesOthers) {
                filteredDoms.push_back(candidate);
            }
        }

        if (!filteredDoms.empty()) {
            immediateDominators[BB] = filteredDoms.front();
        } 
        else {
            immediateDominators[BB] = nullptr;
        }
    }

    BasicBlock* exit = nullptr;
    for (auto& BB : F) {
        if (succ_begin(&BB) == succ_end(&BB)) {
            exit = &BB;
            break;
        }
    }
    assert(exit);

    postDominators[exit].push_back(exit);
    for (auto& BB : F) {
        if (&BB != exit) {
            for (auto& PBB : F) {
                postDominators[&BB].push_back(&PBB);
            }
        }
    }

    bool changed_p = true;
    while (changed_p) {
        changed_p = false;

        for (auto& BB : F) {
            if (&BB == exit) continue;

            vector<BasicBlock*> intersectPostDoms;
            auto succ = succ_begin(&BB);
            if (succ == succ_end(&BB)) continue;

            intersectPostDoms = postDominators[*succ];

            for (succ = std::next(succ); succ != succ_end(&BB); ++succ) {
                vector<BasicBlock*> temp;
                for (auto dom : intersectPostDoms) {
                    if (std::find(postDominators[*succ].begin(), postDominators[*succ].end(), dom) != postDominators[*succ].end()) {
                        temp.push_back(dom);
                    }
                }
                intersectPostDoms = temp;
            }

            intersectPostDoms.push_back(&BB);

            std::sort(postDominators[&BB].begin(), postDominators[&BB].end());
            std::sort(intersectPostDoms.begin(), intersectPostDoms.end());
            if (postDominators[&BB] != intersectPostDoms) {
                postDominators[&BB] = intersectPostDoms;
                changed_p = true;
            }
        }
    }

    map<BasicBlock*, vector<BasicBlock*>> properPostDominators;

    for (auto& pair : postDominators) {
        BasicBlock* BB = pair.first;
        vector<BasicBlock*> properPostDoms = pair.second;
        properPostDoms.erase(std::remove(properPostDoms.begin(), properPostDoms.end(), BB), properPostDoms.end());
        properPostDominators[BB] = properPostDoms;
    }

    for (auto& pair : properPostDominators) {
        BasicBlock* BB = pair.first;
        vector<BasicBlock*> properPostDoms = pair.second;

        vector<BasicBlock*> filteredPostDoms;

        for (BasicBlock* candidate : properPostDoms) {
            bool dominatesOthers = false;

            for (BasicBlock* other : properPostDoms) {
                if (candidate != other &&
                    std::find(properPostDominators[other].begin(), properPostDominators[other].end(), candidate) != properPostDominators[other].end()) {
                    dominatesOthers = true;
                    break;
                }
            }

            if (!dominatesOthers) {
                filteredPostDoms.push_back(candidate);
            }
        }

        if (!filteredPostDoms.empty()) {
            immediatePostDominators[BB] = filteredPostDoms.front();
        } 
        else {
            immediatePostDominators[BB] = nullptr;
        }
    }

    for (const auto& pair : dominators) {
        string BBname = pair.first->getName().str();
        vector<string> domNames;
        for (auto BB : pair.second) {
            domNames.push_back(BB->getName().str());
        }
        std::sort(domNames.begin(), domNames.end());
        dominatorNames[BBname] = domNames;
    }

    for (const auto& pair : immediateDominators) {
        string BBname = pair.first->getName().str();
        string idomName = pair.second ? pair.second->getName().str() : "X";
        iDomNames[BBname] = idomName;
    }

    for (const auto& pair : postDominators) {
        string BBname = pair.first->getName().str();
        vector<string> postDomNames;
        for (auto BB : pair.second) {
            postDomNames.push_back(BB->getName().str());
        }
        std::sort(postDomNames.begin(), postDomNames.end());
        postDominatorNames[BBname] = postDomNames;
    }

    for (const auto& pair : immediatePostDominators) {
        string BBname = pair.first->getName().str();
        string iPostDomName = pair.second ? pair.second->getName().str() : "X";
        iPostDomNames[BBname] = iPostDomName;
    }

    for (const auto& pair : dominatorNames) {
        outs() <<  pair.first << ":\n    ";
        for (const auto& name : pair.second) {
            outs() <<  name << " ";
        }
        outs() << "\n";

        outs() << "    " << iDomNames[pair.first] << "\n    ";

        for (const auto& name : postDominatorNames[pair.first]) {
            outs() << name << " ";
        }
        outs() << "\n";

        outs() << "    " << iPostDomNames[pair.first] << "\n";

    }
}

struct A5Dom : PassInfoMixin<A5Dom> {
  // This is the main entry point for processing the IR of a function
  // It simply calls the function that has your code
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    processFunction(F);
    return PreservedAnalyses::all();
  }

  // This makes sure that the pass is executed, even when functions are
  // decorated with the optnone attribute; this is the case when using
  // clang without -O flags
  static bool isRequired() { return true; }
};
} // namespace

// Register the pass with the pass manager as a function pass
llvm::PassPluginLibraryInfo getA5DomPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "A5Dom", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "A5Dom") {
                    FPM.addPass(A5Dom());
                    return true;
                  }
                  return false;
                });
          }};
}

// This ensures that opt recognizes A5Dom when specified on the
// command line by -passes="A5Dom"
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getA5DomPluginInfo();
}
