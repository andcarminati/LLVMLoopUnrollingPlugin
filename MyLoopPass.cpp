//=============================================================================
// FILE:
//    MyLoopPass.cpp
//
// DESCRIPTION:
//    This pass implements My Loop Pass
//
// USAGE:
//    2. New PM
//      opt -load-pass-plugin=MyLoopPass.dylib -passes="my-loop-pass" `\`
//        -disable-output <input-llvm-file>
//
//
// License: MIT
//=============================================================================
#include "MyLoopPass.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

using namespace llvm;

#define DEBUG_TYPE "loop-opt-tutorial"

static const char *VerboseDebug = DEBUG_TYPE "-verbose";

static Loop *myCloneLoopWithPreheader(BasicBlock *Before, BasicBlock *LoopDomBB,
                                      Loop *OrigLoop, ValueToValueMapTy &VMap,
                                      const Twine &NameSuffix, LoopInfo *LI,
                                      SmallVectorImpl<BasicBlock *> &Blocks);

bool LoopSplit::run(Loop &L) const {

  // L.getHeader()->getParent()->viewCFG();
  LLVM_DEBUG(dbgs() << "Entering " << __func__ << "\n");

  LLVM_DEBUG(dbgs() << "TODO: Need to check if Loop is a valid candidate\n");
  if (!isCandidate(L)) {
    LLVM_DEBUG(dbgs() << "Loop " << L.getName()
                      << " is not a candidate for splitting.\n");
    return false;
  }

  LLVM_DEBUG(dbgs() << "Loop " << L.getName()
                    << " is a candidate for splitting!\n");

  return splitLoopInHalf(L);
  ;
}

bool LoopSplit::isCandidate(const Loop &L) const {

  // Require loops with preheaders and dedicated exits.
  if (!L.isLoopSimplifyForm())
    return false;

  // Since we use cloning to split the loop, it has to be safe to clone.
  if (!L.isSafeToClone())
    return false;

  // If the loop has multiple exiting blocks, do not split.
  if (!L.getExitingBlock())
    return false;

  // If loop has multiple exit blocks, do not split.
  if (!L.getExitBlock())
    return false;

  // Only split innermost loops. Thus, if the loop has any children, it cannot
  // be split.
  if (!L.getSubLoops().empty())
    return false;

  if (!L.getBounds(SE).hasValue())
    return false;

  return true;
}

bool LoopSplit::splitLoopInHalf(Loop &L) const {
  assert(L.isLoopSimplifyForm() && "Expecting a loop in simplify form");
  assert(L.isSafeToClone() && "Loop is not safe to be cloned");
  assert(L.getSubLoops().empty() && "Expecting a innermost loop");

  const Function &F = *L.getHeader()->getParent();

  DEBUG_WITH_TYPE(VerboseDebug, dumpFunction("Original loop:\n", F););

  LLVM_DEBUG(dbgs() << "Splitting loop " << L.getName() << "\n");

  // Generate the code that computes the split point.
  Instruction *Split =
      computeSplitPoint(L, L.getLoopPreheader()->getTerminator());

  DEBUG_WITH_TYPE(VerboseDebug, dumpFunction("After split instruction:\n", F););

  // Split the loop preheader to create an insertion point for the cloned loop.
  BasicBlock *Preheader = L.getLoopPreheader();
  BasicBlock *Pred = Preheader;
  BasicBlock *InsertBefore =
      SplitBlock(Preheader, Preheader->getTerminator(), &DT, &LI);

  DEBUG_WITH_TYPE(VerboseDebug,
                  dumpFunction("After splitting preheader:\n", F););

  // Clone the original loop, and insert the clone before the original loop.
  Loop *ClonedLoop = cloneLoop(L, *InsertBefore, *Pred);

  DEBUG_WITH_TYPE(VerboseDebug, dumpFunction("After clone loop:\n", F););

  // Modify the upper bound of the cloned loop.
  ICmpInst *LatchCmpInst = getLatchCmpInst(*ClonedLoop);
  assert(LatchCmpInst && "Unable to find the latch comparison instruction");
  LatchCmpInst->setOperand(1, Split);

  // Modify the lower bound of the original loop.
  PHINode *IndVar = L.getInductionVariable(SE);
  assert(IndVar && "Unable to find the induction variable PHI node");
  IndVar->setIncomingValueForBlock(L.getLoopPreheader(), Split);

  DEBUG_WITH_TYPE(VerboseDebug,
                  dumpFunction("After splitting the loop:\n", F););
  return true;
}

Instruction *LoopSplit::computeSplitPoint(const Loop &L,
                                          Instruction *InsertBefore) const {
  Optional<Loop::LoopBounds> Bounds = L.getBounds(SE);
  assert(Bounds.hasValue() && "Unable to retrieve the loop bounds");

  Value &IVInitialVal = Bounds->getInitialIVValue();
  Value &IVFinalVal = Bounds->getFinalIVValue();
  auto *Sub = BinaryOperator::Create(Instruction::Sub, &IVFinalVal,
                                     &IVInitialVal, "", InsertBefore);

  return BinaryOperator::Create(Instruction::UDiv, Sub,
                                ConstantInt::get(IVFinalVal.getType(), 2), "",
                                InsertBefore);
}

void LoopSplit::dumpFunction(const StringRef Msg, const Function &F) const {
  dbgs() << Msg;
  F.dump();
}

Loop *LoopSplit::cloneLoop(Loop &L, BasicBlock &InsertBefore,
                           BasicBlock &Pred) const {
  // Clone the original loop, insert the clone before the "InsertBefore" BB.
  Function &F = *L.getHeader()->getParent();
  SmallVector<BasicBlock *, 4> ClonedLoopBlocks;
  ValueToValueMapTy VMap;

  // Same as cloneLoopWithPreheader but does not update the dominator tree.
  // Use for education purposes only, use cloneLoopWithPreheader in production
  // code.
  Loop *NewLoop = myCloneLoopWithPreheader(&InsertBefore, &Pred, &L, VMap, "",
                                           &LI, ClonedLoopBlocks);

  assert(NewLoop && "Run out of memory");
  DEBUG_WITH_TYPE(VerboseDebug,
                  dbgs() << "Create new loop: " << NewLoop->getName() << "\n";
                  dumpFunction("After cloning loop:\n", F););

  // Update instructions referencing the original loop basic blocks to
  // reference the corresponding block in the cloned loop.
  VMap[L.getExitBlock()] = &InsertBefore;
  remapInstructionsInBlocks(ClonedLoopBlocks, VMap);
  DEBUG_WITH_TYPE(VerboseDebug,
                  dumpFunction("After instruction remapping:\n", F););

  // Make the predecessor of original loop jump to the cloned loop.
  Pred.getTerminator()->replaceUsesOfWith(&InsertBefore,
                                          NewLoop->getLoopPreheader());

  return NewLoop;
}

static Loop *myCloneLoopWithPreheader(BasicBlock *Before, BasicBlock *LoopDomBB,
                                      Loop *OrigLoop, ValueToValueMapTy &VMap,
                                      const Twine &NameSuffix, LoopInfo *LI,
                                      SmallVectorImpl<BasicBlock *> &Blocks) {
  assert(OrigLoop->getSubLoops().empty() && "Cannot split an outer loop");

  Function *F = OrigLoop->getHeader()->getParent();
  Loop *ParentLoop = OrigLoop->getParentLoop();
  DenseMap<Loop *, Loop *> LMap;

  Loop *NewLoop = LI->AllocateLoop();
  LMap[OrigLoop] = NewLoop;
  if (ParentLoop)
    ParentLoop->addChildLoop(NewLoop);
  else
    LI->addTopLevelLoop(NewLoop);

  BasicBlock *OrigPH = OrigLoop->getLoopPreheader();
  assert(OrigPH && "No preheader");
  BasicBlock *NewPH = CloneBasicBlock(OrigPH, VMap, NameSuffix, F);
  // To rename the loop PHIs.
  VMap[OrigPH] = NewPH;
  Blocks.push_back(NewPH);

  // Update LoopInfo.
  if (ParentLoop) {
    ParentLoop->addBasicBlockToLoop(NewPH, *LI);
  }

  for (BasicBlock *BB : OrigLoop->getBlocks()) {
    BasicBlock *NewBB = CloneBasicBlock(BB, VMap, NameSuffix, F);
    VMap[BB] = NewBB;

    // Update LoopInfo.
    NewLoop->addBasicBlockToLoop(NewBB, *LI);
    if (BB == OrigLoop->getHeader())
      NewLoop->moveToHeader(NewBB);

    Blocks.push_back(NewBB);
  }

  // Move them physically from the end of the block list.
  F->getBasicBlockList().splice(Before->getIterator(), F->getBasicBlockList(),
                                NewPH);
  F->getBasicBlockList().splice(Before->getIterator(), F->getBasicBlockList(),
                                NewLoop->getHeader()->getIterator(), F->end());

  return NewLoop;
}

ICmpInst *LoopSplit::getLatchCmpInst(const Loop &L) const {
  if (BasicBlock *Latch = L.getLoopLatch())
    if (BranchInst *BI = dyn_cast_or_null<BranchInst>(Latch->getTerminator()))
      if (BI->isConditional())
        return dyn_cast<ICmpInst>(BI->getCondition());

  return nullptr;
}

using PHIRemap =
    SmallMapVector<Instruction *, std::pair<PHINode *, Instruction *>, 8>;

static void MapValuesUsedInPHIs(BasicBlock *BB, PHIRemap &Map) {
  for (Instruction &I : *BB)
    for (User *U : I.users())
      if (PHINode *PHI = dyn_cast_or_null<PHINode>(U))
        Map[&I] = std::make_pair(PHI, &I);
}

static Instruction *CloneInstr(Instruction &I, ValueToValueMapTy &VMap,
                               PHIRemap &LoopPHIRemap, BasicBlock *Dest) {
  Instruction *NewInst = I.clone();

  if (I.hasName())
    NewInst->setName(I.getName());

  Dest->getInstList().push_back(NewInst);
  VMap[&I] = NewInst;
  // Update operands to reflect values generated by the new instructions.
  // in the block
  for (unsigned i = 0, e = NewInst->getNumOperands(); i != e; ++i) {
    if (Instruction *Inst =
            dyn_cast_or_null<Instruction>(NewInst->getOperand(i))) {
      auto IT = VMap.find(Inst);
      if (IT != VMap.end())
        NewInst->setOperand(i, IT->second);
    }
  }
  // If a I is used in a PHI in NewInst, replace
  // PHI in NewInst by I and Update the Map for the next
  // unrolling.
  auto IT = LoopPHIRemap.find(&I);
  if (IT != LoopPHIRemap.end()) {
    auto &Pair = IT->second;
    auto *PHI = Pair.first;
    auto *Replacement = Pair.second;
    NewInst->replaceUsesOfWith(PHI, Replacement);
    Pair.second = NewInst;
  }

  return NewInst;
}

static void UnrollLoop(Loop &L, int UF, Loop::LoopBounds &Bounds,
                       ScalarEvolution &SE, LoopInfo &LI) {
  BasicBlock *BB = L.getHeader();
  Function *F = BB->getParent();
  BasicBlock *Latch = L.getLoopLatch();
  // Instruction *Term = BB->getTerminator();
  Instruction *Last = BB->getFirstNonPHI();
  ValueToValueMapTy VMap;
  PHIRemap LoopPHIRemap;
  int NumberOfInstructions = BB->size();

  PHINode *IndVar = L.getInductionVariable(SE);
  DEBUG_WITH_TYPE(VerboseDebug,
                  dbgs() << "Induction variable: " << *IndVar << "\n";);
  // Get the loop bound and the Inst that updates de induction
  // var.
  auto Step = Bounds.getStepValue();
  auto &IndStepInst = Bounds.getStepInst();
  Instruction *IndVarNew = IndVar;

  // Initialize the values that will track the changes of of incoming
  // values if the PHI nodes.
  MapValuesUsedInPHIs(BB, LoopPHIRemap);

  for (unsigned i = 0; i < UF - 1; i++) {
    // Create a new header BB
    BasicBlock *NewHeader =
        BasicBlock::Create(BB->getContext(), "", BB->getParent());
    BasicBlock *OldPred = Latch->getSinglePredecessor();
    NewHeader->moveBefore(Latch);
    OldPred->getTerminator()->replaceUsesOfWith(Latch, NewHeader);
    Instruction *Term = BranchInst::Create(Latch, NewHeader);
    L.addBasicBlockToLoop(NewHeader, LI);

    // clone the Inst that updates the induction after the unrolled part
    // of the loop and move it to the end of the block and before the
    // terminator.
    auto IndInstUpdated = IndStepInst.clone();
    NewHeader->getInstList().push_back(IndInstUpdated);
    IndInstUpdated->moveBefore(Term);
    IndInstUpdated->replaceUsesOfWith(IndVar, IndVarNew);
    VMap[IndVar] = IndInstUpdated;
    IndVarNew = IndInstUpdated;

    // Clone any non PHI and non terminator instruction of
    // the original block
    auto IT = BB->begin();
    for (unsigned i = 0; i < NumberOfInstructions - 1; i++, IT++) {
      Instruction &I = *IT;

      // skip PHI nodes.
      if (PHINode *PHI = dyn_cast_or_null<PHINode>(&I))
        continue;

      Last = CloneInstr(I, VMap, LoopPHIRemap, NewHeader);
    }
    // Move terminator again to the end.
    Term->moveAfter(Last);
  }

  // Update PHIs in header using to use the new SSA values that were
  // generated by the unrolling.
  for (auto IT : LoopPHIRemap) {
    auto *OrigInst = IT.first;
    auto Pair = IT.second;
    auto *PHI = Pair.first;
    auto *ReplacementInst = Pair.second;
    PHI->replaceUsesOfWith(OrigInst, ReplacementInst);
  }

  // Update LCSSA Phis
  for (Instruction &I : *L.getExitBlock()) {
    if (PHINode *PHI = dyn_cast_or_null<PHINode>(&I)) {
      for (unsigned i = 0, e = PHI->getNumOperands(); i != e; ++i) {
        if (Instruction *Inst =
                dyn_cast_or_null<Instruction>(PHI->getOperand(i))) {
          auto IT = VMap.find(Inst);
          if (IT != VMap.end())
            PHI->setOperand(i, IT->second);
        }
      }
    }
  }

  // Updating the latch to double the induction due to the single
  // unrolling
  Constant *ConstStep = dyn_cast<Constant>(Step);
  auto StepAP = APInt(ConstStep->getUniqueInteger());
  Step->dump();

  auto ConstToMult = ConstantInt::get(F->getContext(), APInt(32, UF));
  ConstToMult->dump();
  auto *Add = BinaryOperator::Create(Instruction::Mul, Step, ConstToMult, "",
                                     &IndStepInst);
  IndStepInst.replaceUsesOfWith(Step, Add);
}

bool LoopUnrollTwice::run(Loop &L) const {

  Function &F = *L.getHeader()->getParent();
  Optional<Loop::LoopBounds> Bounds = L.getBounds(SE);

  DEBUG_WITH_TYPE(VerboseDebug, dumpFunction("Original loop:\n", F););

  if (!isCandidate(L) || !Bounds.hasValue()) {
    LLVM_DEBUG(dbgs() << "Loop " << L.getName()
                      << " is not a candidate for unroll.\n");
    return false;
  }
  LLVM_DEBUG(dbgs() << "Loop " << L.getName()
                    << " is a candidate for unroll.\n");

  UnrollLoop(L, 5, Bounds.getValue(), SE, LI);

  DEBUG_WITH_TYPE(VerboseDebug,
                  dumpFunction("After instruction cloning:\n", F););

  return true;
}

bool LoopUnrollTwice::isCandidate(const Loop &L) const {

  // Require loops with preheaders and dedicated exits.
  if (!L.isLoopSimplifyForm())
    return false;

  // Since we use cloning to split the loop, it has to be safe to clone.
  if (!L.isSafeToClone())
    return false;

  // If the loop has multiple exiting blocks, do not split.
  if (!L.getExitingBlock())
    return false;

  // If loop has multiple exit blocks, do not split.
  if (!L.getExitBlock())
    return false;

  // Only unroll innermost loops. Thus, if the loop has any children, it cannot
  // be split.
  if (!L.getSubLoops().empty())
    return false;

  // Only unroll loop containing header and latch blocks.
  if (L.getNumBlocks() > 2)
    return false;

  return true;
}

void LoopUnrollTwice::dumpFunction(const StringRef Msg,
                                   const Function &F) const {
  dbgs() << Msg;
  F.dump();
}

//-----------------------------------------------------------------------------
// MyLoopPass implementation
//-----------------------------------------------------------------------------

PreservedAnalyses MyLoopPass::run(Loop &L, LoopAnalysisManager &AM,
                                  LoopStandardAnalysisResults &AR,
                                  LPMUpdater &U) {

  LLVM_DEBUG(dbgs() << "Entering LoopOptTutorialPass::run\n");
  LLVM_DEBUG(dbgs() << "Loop: "; L.dump(); dbgs() << "\n");

  // bool Changed = LoopSplit(AR.LI, AR.SE, AR.DT).run(L);
  bool Changed = LoopUnrollTwice(AR.LI, AR.SE, AR.DT).run(L);

  if (!Changed)
    return PreservedAnalyses::all();

  return llvm::getLoopPassPreservedAnalyses();
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
AnalysisKey MyLoopPass::Key;

llvm::PassPluginLibraryInfo getMyLoopPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "my-loop-pass", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, LoopPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "my-loop-pass") {
                    FPM.addPass(MyLoopPass());
                    return true;
                  }
                  return false;
                });
          }};

  // };
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize LivenessAnalysis when added to the pass pipeline on the
// command line, i.e. via '-passes=liveness'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getMyLoopPassPluginInfo();
}
