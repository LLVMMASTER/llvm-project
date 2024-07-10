#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include <map>

using namespace llvm;

namespace {
class ControlFlowFlatteningPass : public PassInfoMixin<ControlFlowFlatteningPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    // Skip functions with less than two basic blocks
    if (F.size() < 2) {
      return PreservedAnalyses::all();
    }

    // Create a new entry block
    LLVMContext &Ctx = F.getContext();
    BasicBlock *EntryBlock = &F.getEntryBlock();
    BasicBlock *NewEntry = BasicBlock::Create(Ctx, "NewEntry", &F, EntryBlock);
    IRBuilder<> builder(NewEntry);

    // Create a switch variable
    Type *Int32Ty = Type::getInt32Ty(Ctx);
    AllocaInst *SwitchVar = builder.CreateAlloca(Int32Ty, nullptr, "switchVar");
    builder.CreateStore(ConstantInt::get(Int32Ty, 0), SwitchVar);

    // Create a switch statement
    LoadInst *SwitchVarLoad = builder.CreateLoad(Int32Ty, SwitchVar, "switchVarLoad");
    SwitchInst *Switch = builder.CreateSwitch(SwitchVarLoad, EntryBlock, F.size());

    // Create a map of basic blocks to case values
    std::map<BasicBlock*, int> BlockMap;
    int CaseValue = 1;
    for (auto &BB : F) {
      if (&BB != EntryBlock) {
        BlockMap[&BB] = CaseValue++;
      }
    }

    // Update each basic block
    for (auto &BB : F) {
      if (&BB == EntryBlock) continue;

      IRBuilder<> bbBuilder(&*BB.getFirstInsertionPt());
      bbBuilder.CreateStore(ConstantInt::get(Int32Ty, BlockMap[&BB]), SwitchVar);

      // Replace the original terminator with a branch to the new entry block
      auto *terminator = BB.getTerminator();
      if (terminator) {
        terminator->eraseFromParent();
      }
      bbBuilder.CreateBr(NewEntry);

      // Add the case to the switch statement
      Switch->addCase(cast<ConstantInt>(ConstantInt::get(Int32Ty, BlockMap[&BB])), &BB);
    }

    // Move the entry block after the new entry block
    EntryBlock->moveAfter(NewEntry);

    builder.SetInsertPoint(NewEntry->getTerminator());
    builder.CreateBr(EntryBlock);

    return PreservedAnalyses::none();
  }
};
} // namespace

// Register the pass with the new pass manager
llvm::PassPluginLibraryInfo getControlFlowFlatteningPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "ControlFlowFlatteningPass", LLVM_VERSION_STRING, [](PassBuilder &PB) {
    PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
          if (Name == "cff") {
            FPM.addPass(ControlFlowFlatteningPass());
            return true;
          }
          return false;
        });
  }};
}

// Export the pass registration routine
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return getControlFlowFlatteningPassPluginInfo();
}

