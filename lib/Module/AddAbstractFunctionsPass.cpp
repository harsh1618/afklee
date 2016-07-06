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
#include "llvm/IR/IRBuilder.h"
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

#include <vector>

using namespace llvm;
using namespace klee;

char AddAbstractFunctionsPass::ID;

bool AddAbstractFunctionsPass::runOnModule(Module &M) {

  bool moduleChanged = false;

  AliasAnalysis &AA = getAnalysis<AliasAnalysis>();
  Value *sym_name_gep;

  for (Module::iterator f = M.begin(), fe = M.end(); f != fe; ++f) {
    Function *func = &*f;
    FunctionType *ftype = func->getFunctionType();
    Type *retType = ftype->getReturnType();
    if (ftype->isVarArg()) continue;
    if (retType->isPointerTy() || retType->isSized() || retType->isVoidTy()) continue;
    if (!(func->hasName())) continue;

    // Function doesn't modify memory outside its stack frame.
    // Can be abstracted.
    if (!(AA.getModRefBehavior(func) & AliasAnalysis::Mod)
        || func->doesNotAccessMemory()) {
      // Create a new function of the same type as the original which returns a
      // symbolic value.
      Function *absFun = Function::Create(ftype,
          Function::InternalLinkage,
          "_symbolic_" + func->getName(), &M);
      Function *klee_make_symbolic = M.getFunction("klee_make_symbolic");
      assert(klee_make_symbolic && "klee_make_symbolic function not found in module");
      IRBuilder<> Builder(getGlobalContext());
      BasicBlock *BB = BasicBlock::Create(getGlobalContext(), "", absFun);
      Builder.SetInsertPoint(BB);
      AllocaInst *alloc = Builder.CreateAlloca(ftype->getReturnType());
      Value *ptr = Builder.CreateBitCast(alloc, Type::getInt8PtrTy(getGlobalContext()));
      std::vector<llvm::Value*> args;
      args.push_back(ptr);
      uint64_t size = AA.getTypeStoreSize(retType);
      ConstantInt* size_val = ConstantInt::get(getGlobalContext(), APInt(64,size));
      args.push_back(size_val);
      if (!M.getNamedValue("klee_sym_name"))
        sym_name_gep = Builder.CreateGlobalStringPtr("klee_sym_name", "klee_sym_name");
      args.push_back(sym_name_gep);
      Builder.CreateCall(klee_make_symbolic, args);
      Value *returnVal = Builder.CreateLoad(alloc);
      Builder.CreateRet(returnVal);

      moduleChanged = true;
    }
  }
  return moduleChanged;
}
