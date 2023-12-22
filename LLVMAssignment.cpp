//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#include <llvm/Support/CommandLine.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/ToolOutputFile.h>

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>

#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/Scalar.h>

#include <llvm/IR/Function.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include "Liveness.h"
#include "PointsTo.h"
#include "Dataflow.h"
using namespace llvm;
static ManagedStatic<LLVMContext> GlobalContext;
static LLVMContext &getGlobalContext() { return *GlobalContext; }

struct EnableFunctionOptPass : public FunctionPass
{
    static char ID;
    EnableFunctionOptPass() : FunctionPass(ID) {}
    bool runOnFunction(Function &F) override
    {
        if (F.hasFnAttribute(Attribute::OptimizeNone))
        {
            F.removeFnAttr(Attribute::OptimizeNone);
        }
        return true;
    }
};

char EnableFunctionOptPass::ID = 0;

///!TODO TO BE COMPLETED BY YOU FOR ASSIGNMENT 3
struct FuncPtrPass : public ModulePass
{
    static char ID; // Pass identification, replacement for typeid
    std::map<int, std::set<std::string>> line2func;

    FuncPtrPass() : ModulePass(ID) {}

    bool runOnModule(Module &M) override
    {
        PointsToVisitor visitor;
        DataflowResult<PointsToInfo>::Type result;
        PointsToInfo initval;
        for (Function &F : M)
        {
            compForwardDataflow(&F, &visitor, &result, initval);
        }

        // for (Function &F : M)
        // {
        //     for (BasicBlock &BB : F)
        //     {
        //         errs() << "BasicBlock:\n";
        //         errs() << BB << "\n";
        //         PointsToInfo incoming = result[&BB].first;
        //         for (auto iter = incoming.pointsToSets.begin(); iter != incoming.pointsToSets.end(); ++iter)
        //         {
        //             errs() << "Incoming: " << *(iter->first) << "\n";
        //             for (auto iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2)
        //             {
        //                 errs() << **iter2 << "\n";
        //             }
        //         }
        //         PointsToInfo outgoing = result[&BB].second;
        //         errs() << "Outgoing:\n";
        //         for (auto iter = outgoing.pointsToSets.begin(); iter != outgoing.pointsToSets.end(); ++iter)
        //         {
        //             errs() << "Outgoing: " << *(iter->first) << "\n";
        //             for (auto iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2)
        //             {
        //                 errs() << **iter2 << "\n";
        //             }
        //         }
        //     }
        // }

        
        for (Function &F : M)
        {
            for (BasicBlock &BB : F)
            {
                PointsToInfo incoming = result[&BB].first;
                for (Instruction &I : BB)
                {
                    if (const CallInst *C = dyn_cast<CallInst>(&I))
                    {
                        if (I.getDebugLoc().getLine())
                        {
                            // errs() << "Location: " << I.getDebugLoc().getLine() << "\n";
                            // errs() << "CallInst: " << I << "\n";
                            std::set<Value *> S;
                            S = incoming.pointsToSets[C->getCalledOperand()];
                            for (Value *f : S)
                                line2func[I.getDebugLoc().getLine()].insert(f->getName());
                        }
                    }
                    visitor.compDFVal(&I, &incoming);
                }
            }
        }
        for (auto iter = line2func.begin(); iter != line2func.end(); ++iter)
        {
            outs() << iter->first << " : ";
            for (auto iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2)
            {
                if (iter2 != iter->second.begin())
                    outs() << ", ";
                outs() << *iter2;
            }
            outs() << "\n";
        }
        return false;
    }
};

char FuncPtrPass::ID = 0;
static RegisterPass<FuncPtrPass> X("funcptrpass", "Print function call instruction");

char Liveness::ID = 0;
static RegisterPass<Liveness> Y("liveness", "Liveness Dataflow Analysis");

static cl::opt<std::string>
    InputFilename(cl::Positional,
                  cl::desc("<filename>.bc"),
                  cl::init(""));

int main(int argc, char **argv)
{
    LLVMContext &Context = getGlobalContext();
    SMDiagnostic Err;
    // Parse the command line to read the Inputfilename
    cl::ParseCommandLineOptions(argc, argv,
                                "FuncPtrPass \n My first LLVM too which does not do much.\n");

    // Load the input module
    std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);
    if (!M)
    {
        Err.print(argv[0], errs());
        return 1;
    }

    llvm::legacy::PassManager Passes;
#if LLVM_VERSION_MAJOR == 5
    Passes.add(new EnableFunctionOptPass());
#endif
    /// Transform it to SSA
    Passes.add(llvm::createPromoteMemoryToRegisterPass());

    /// Your pass to print Function and Call Instructions
    // Passes.add(new Liveness());
    Passes.add(new FuncPtrPass());
    Passes.run(*M.get());
    // #ifndef NDEBUG
    //     system("pause");
    // #endif
}
