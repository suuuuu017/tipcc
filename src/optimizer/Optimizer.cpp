#include "Optimizer.h"

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

//new headers
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/Passbuilder.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/NewGVN.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"

#include "llvm/Transforms/IPO/Inliner.h"
#include "llvm/Transforms/IPO/PartialInlining.h"
#include "llvm/Transforms/Scalar/LoopUnrollAndJamPass.h"
#include "llvm/Transforms/Scalar/TailRecursionElimination.h"
#include "llvm/Transforms/Scalar/DCE.h"
#include "llvm/Transforms/Scalar/JumpThreading.h"

#include "loguru.hpp"

using namespace llvm;

void Optimizer::optimize(Module* theModule, bool extraOpts = false) {
    // Create the analysis managers.
    LoopAnalysisManager LAM;
    FunctionAnalysisManager FAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;

// Create the new pass manager builder.
// Take a look at the PassBuilder constructor parameters for more
// customization, e.g. specifying a TargetMachine or various debugging
// options.
    PassBuilder PB;

// Register all the basic analyses with the managers.
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

// Create the pass manager.
// This one corresponds to a typical -O2 optimization pipeline.
    ModulePassManager MPM;

// Optimize the IR!
//    MPM.run(MyModule, MAM);

  LOG_S(1) << "Optimizing program " << theModule->getName().str();

  // Create a pass manager to simplify generated module
//  auto TheFPM = std::make_unique<legacy::FunctionPassManager>(theModule);

  FunctionPassManager FPM;

  // Promote allocas to registers.
//  TheFPM->add(createPromoteMemoryToRegisterPass());
  FPM.addPass(PromotePass());

  // Do simple "peephole" optimizations
//  TheFPM->add(createInstructionCombiningPass());
  FPM.addPass(InstCombinePass());

  // Reassociate expressions.
//  TheFPM->add(createReassociatePass());
  FPM.addPass(ReassociatePass());

  // Eliminate Common SubExpressions.
//  TheFPM->add(createGVNPass());
  FPM.addPass(NewGVNPass());

  // Simplify the control flow graph (deleting unreachable blocks, etc).
//  TheFPM->add(createCFGSimplificationPass());
  FPM.addPass(SimplifyCFGPass());

  // initialize and run simplification pass on each function
//  TheFPM->doInitialization();
//  for (auto &fun : theModule->getFunctionList()) {
//    LOG_S(1) << "Optimizing function " << fun.getName().str();
//
//    TheFPM->run(fun);
//  }
//    FPM.addPass(InlinerPass());
//    FPM.addPass(PartialInlinerPass());
    extraOpts = true;
    if(extraOpts){
        FPM.addPass(createFunctionToLoopPassAdaptor(LoopUnrollAndJamPass()));
        FPM.addPass(TailCallElimPass());
        FPM.addPass(JumpThreadingPass());
        FPM.addPass(DCEPass());
    }

    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));

    if(extraOpts){
        MPM.addPass(PartialInlinerPass());
    }

    MPM.run(* theModule, MAM);
}
