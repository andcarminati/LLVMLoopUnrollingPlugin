//========================================================================
// FILE:
//    LivenessAnalysis.h
//
// DESCRIPTION:
//    Declares the LivenessAnalysis Passes
//      * new pass manager interface
//      * legacy pass manager interface
//      * printer pass for the new pass manager
//
// License: MIT
//========================================================================
#ifndef LLVM_LIVENESSANALYSIS_H
#define LLVM_LIVENESSANALYSIS_H

#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/ValueMapper.h"


//------------------------------------------------------------------------------
// New PM interface
//------------------------------------------------------------------------------
namespace llvm
{
class Loop;
class LPMUpdater;

class LoopSplit {
public:
  LoopSplit(LoopInfo &LI, ScalarEvolution &SE, DominatorTree &DT)
      : LI(LI), SE(SE), DT(DT) {}

  bool run(Loop &L) const;
  bool isCandidate(const Loop &L) const;
  bool splitLoopInHalf(Loop &L) const;
  //Loop *cloneLoop(Loop &L, BasicBlock &InsertBefore, BasicBlock &Pred) const;
  Instruction *computeSplitPoint(const Loop &L,
                                 Instruction *InsertBefore) const;
  void dumpFunction(const StringRef Msg, const Function &F) const; 
  Loop *cloneLoop(Loop &L, BasicBlock &InsertBefore, BasicBlock &Pred) const;
  ICmpInst *getLatchCmpInst(const Loop &L) const;

private:
  LoopInfo &LI;
  ScalarEvolution &SE;
  DominatorTree &DT;
};

class LoopUnrollTwice {
public:
  LoopUnrollTwice(LoopInfo &LI, ScalarEvolution &SE, DominatorTree &DT)
      : LI(LI), SE(SE), DT(DT) {}

  bool run(Loop &L) const;
  bool isCandidate(const Loop &L) const;
  void dumpFunction(const StringRef Msg, const Function &F) const;

private:
  LoopInfo &LI;
  ScalarEvolution &SE;
  DominatorTree &DT;
};



class MyLoopPass : public PassInfoMixin<MyLoopPass> {
public:

   PreservedAnalyses run(Loop &L, LoopAnalysisManager &AM,
                        LoopStandardAnalysisResults &AR, LPMUpdater &U);

private:
    // A special type used by analysis passes to provide an address that
    // identifies that particular analysis pass type.
    static llvm::AnalysisKey Key;
    friend struct llvm::PassInfoMixin<MyLoopPass>;
  };

}; // End namespace llvm

#endif // LLVM_LIVENESSANALYSIS_H