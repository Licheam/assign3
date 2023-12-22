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

#include <llvm/IR/Function.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/IntrinsicInst.h>

#include "Dataflow.h"
#include <set>
using namespace llvm;

struct PointsToInfo
{
    std::map<const Value *, std::set<Value *>> pointsToSets; /// Points-to sets
    PointsToInfo() : pointsToSets() {}
    PointsToInfo(const PointsToInfo &info) : pointsToSets(info.pointsToSets) {}

    bool operator==(const PointsToInfo &info) const
    {
        return pointsToSets == info.pointsToSets;
    }
};

inline raw_ostream &operator<<(raw_ostream &out, const PointsToInfo &info)
{
    for (std::map<const Value *, std::set<Value *>>::const_iterator ii = info.pointsToSets.begin(),
                                                                    ie = info.pointsToSets.end();
         ii != ie; ++ii)
    {
        const Value *val = ii->first;
        const std::set<Value *> &pointsToSet = ii->second;
        out << val->getName() << " -> {";
        for (std::set<Value *>::const_iterator si = pointsToSet.begin(), se = pointsToSet.end();
             si != se; ++si)
        {
            const Value *v = *si;
            out << v->getName() << ", ";
        }
        out << "}\n";
    }
    return out;
}

class PointsToVisitor : public DataflowVisitor<struct PointsToInfo>
{
public:
    PointsToVisitor() {}
    void merge(PointsToInfo *dest, const PointsToInfo &src) override
    {
        errs() << "Merging PointsToInfo\n";
        errs() << "Dest: " << *dest << "\n";
        errs() << "Src: " << src << "\n";
        for (std::map<const Value *, std::set<Value *>>::const_iterator ii = src.pointsToSets.begin(),
                                                                        ie = src.pointsToSets.end();
             ii != ie; ++ii)
        {
            const Value *val = ii->first;
            const std::set<Value *> &srcPointsToSet = ii->second;
            std::set<Value *> &destPointsToSet = dest->pointsToSets[val];
            destPointsToSet.insert(srcPointsToSet.begin(), srcPointsToSet.end());
        }
        errs() << "Result: " << *dest << "\n";
    }

    void compDFVal(Instruction *inst, PointsToInfo *dfval) override
    {
        if (isa<DbgInfoIntrinsic>(inst))
            return;
        if (inst->getDebugLoc())
        {
            errs() << "Location: " << inst->getDebugLoc().getLine() << "\n";
        }
        else
        {
            errs() << "Location: None\n";
        }
        if (StoreInst *storeInst = dyn_cast<StoreInst>(inst))
        {
            Value *value = storeInst->getValueOperand();
            Value *pointer = storeInst->getPointerOperand();
            if (isa<ConstantData>(value))
                return;
            errs() << "StoreInst: " << *storeInst << "\n";
            errs() << "Value: " << *value << "\n";
            errs() << "Pointer: " << *pointer << "\n";

            if (Function *f = dyn_cast<Function>(value))
            {
                dfval->pointsToSets[pointer].insert(value);
            }
            else
            {
                dfval->pointsToSets[pointer].insert(
                    dfval->pointsToSets[value].begin(), dfval->pointsToSets[value].end());
            }
        }
        else if (LoadInst *loadInst = dyn_cast<LoadInst>(inst))
        {
            Value *pointer = loadInst->getPointerOperand();
            if (!pointer->getType()->getContainedType(0)->isPointerTy())
                return;
            errs() << "LoadInst: " << *loadInst << "\n";
            errs() << "Pointer: " << *pointer << "\n";

            dfval->pointsToSets[loadInst] = dfval->pointsToSets[pointer];
        }
        else if (CallInst *callInst = dyn_cast<CallInst>(inst))
        {
            errs() << "CallInst: " << *callInst << "\n";
            Value *operand = callInst->getCalledOperand();
            errs() << "CalledOperand: " << *callInst->getCalledOperand() << "\n";
            std::set<Value *> &pointsToSet = dfval->pointsToSets[callInst->getCalledOperand()];
            for (Value *v : pointsToSet)
            {
                if (Function *f = dyn_cast<Function>(v))
                {
                    errs() << "Function: " << f->getName() << "\n";
                }
            }
        }
        else
        {
            errs() << "Instruction not handled: " << *inst << "\n";
        }
    }
};