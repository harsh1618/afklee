#include "Passes.h"

#include "klee/Config/Version.h"
#include "llvm/Support/Debug.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DataLayout.h"
#else
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/InstrTypes.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Module.h"
#include "llvm/Type.h"

#include "llvm/LLVMContext.h"

#if LLVM_VERSION_CODE <= LLVM_VERSION(3, 1)
#include "llvm/Target/TargetData.h"
#else
#include "llvm/DataLayout.h"
#endif
#endif
#include "llvm/Pass.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/Passes.h"

using namespace llvm;
using namespace klee;

char ErrorBitPass::ID;

bool ErrorBitPass::runOnModule(Module &M) {

  AliasAnalysis &AA = getAnalysis<AliasAnalysis>();
  GlobalValue *errorBit = M.getNamedValue("klee_free_status");
  assert(errorBit && "Error bit global not found");
  uint64_t size = AA.getTypeStoreSize(errorBit->getType()->getElementType());
  AliasAnalysis::Location loc(errorBit, size);

  for (Module::iterator f = M.begin(), fe = M.end(); f != fe; ++f) {
    for (Function::iterator b = f->begin(), be = f->end(); b != be; ++b) {
      for (BasicBlock::iterator i = b->begin(), ie = b->end(); i != ie; ++i) {
        if (isa<CallInst>(i) || isa<InvokeInst>(i)) {
          CallSite cs(i);
          if (AA.getModRefInfo(cs, loc) == AliasAnalysis::NoModRef) {
            kmodule->errorBitCalls.insert(cs);
          }
        }
      }
    }
  }
  return false;
}
