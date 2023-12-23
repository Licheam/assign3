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

void dump(std::set<Value *> s)
{
    errs() << "{";
    for (Value *v : s)
    {
        errs() << *v << ", ";
    }
    errs() << "}\n";
}

struct PointsToInfo
{
    std::map<const Value *, std::set<Value *>> pointsToSets;   /// Points-to sets
    std::map<const Value *, std::set<Value *>> pointerInclude; /// Pointer include (exclude itself)
    PointsToInfo() : pointsToSets() {}
    PointsToInfo(const PointsToInfo &info) : pointsToSets(info.pointsToSets) {}

    std::set<Value *> getInclude(Value *p)
    {
        std::set<Value *> result;
        std::set<Value *> everWorkList;
        std::set<Value *> workList = {p};
        while (!workList.empty())
        {
            Value *p = *workList.begin();
            workList.erase(workList.begin());
            if (everWorkList.find(p) != everWorkList.end())
                continue;
            everWorkList.insert(p);

            if (pointerInclude.find(p) == pointerInclude.end())
                result.insert(p);
            else
            {
                for (Value *v : pointerInclude[p])
                {
                    if (everWorkList.find(v) == everWorkList.end())
                        workList.insert(v);
                }
            }
        }
        return result;
    }

    std::set<Value *> getPointsTo(std::set<Value *> ps)
    {
        std::set<Value *> result;
        for (Value *p : ps)
        {
            if (Function *f = dyn_cast<Function>(p))
            {
                result.insert(f);
            }
            else
            {
                if (pointsToSets.find(p) == pointsToSets.end())
                {
                    result.insert(p);
                }
                else
                {
                    result.insert(pointsToSets[p].begin(), pointsToSets[p].end());
                }
            }
        }
        return result;
    }

    std::set<Value *> getPointsTo(Value *p)
    {
        return getPointsTo(getInclude(p));
    }

    void store(Value *pointer, Value *value) // *pointer = value
    {
        errs() << "Store: " << *pointer << " = " << *value << "\n";
        std::set<Value *> pointers = getInclude(pointer);
        std::set<Value *> values = getPointsTo(value);
        errs() << "Pointers: ";
        dump(pointers);
        errs() << "Values: ";
        dump(values);

        if (pointers.size() == 1)
        {
            pointsToSets[*pointers.begin()] = values;
        }
        else
        {
            for (Value *p : pointers)
            {
                pointsToSets[p].insert(values.begin(), values.end());
            }
        }
    }

    void load(Value *pointer, Value *value) // value = *pointer
    {
        std::set<Value *> pointees = getPointsTo(pointer);
        pointerInclude[value] = pointees;
    }

    void assign(Value *pointee, Value *value) // value = pointee
    {
        std::set<Value *> pointees = getInclude(pointee);
        pointerInclude[value] = pointees;
    }

    bool operator==(const PointsToInfo &info) const
    {
        return pointsToSets == info.pointsToSets && pointerInclude == info.pointerInclude;
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
    std::map<int, std::set<std::string>> line2func;

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
        if (AllocaInst *allocaInst = dyn_cast<AllocaInst>(inst))
        {
            errs() << "AllocaInst: " << *allocaInst << "\n";
        }
        else if (StoreInst *storeInst = dyn_cast<StoreInst>(inst))
        {
            errs() << "StoreInst: " << *storeInst << "\n";
            Value *value = storeInst->getValueOperand();
            Value *pointer = storeInst->getPointerOperand();
            if (isa<ConstantData>(value))
                return;
            errs() << "Value: " << *value << "\n";
            errs() << "Pointer: " << *pointer << "\n";

            dfval->store(pointer, value);
        }
        else if (LoadInst *loadInst = dyn_cast<LoadInst>(inst))
        {
            errs() << "LoadInst: " << *loadInst << "\n";
            Value *pointer = loadInst->getPointerOperand();
            if (!pointer->getType()->getContainedType(0)->isPointerTy())
                return;
            errs() << "Pointer: " << *pointer << "\n";

            dfval->load(pointer, loadInst);
        }
        else if (GetElementPtrInst *getElementPtrInst = dyn_cast<GetElementPtrInst>(inst))
        {
            errs() << "GetElementPtrInst: " << *getElementPtrInst << "\n";
            Value *pointer = getElementPtrInst->getPointerOperand();
            errs() << "Pointer: " << *pointer << "\n";
            dfval->assign(pointer, getElementPtrInst);
        }
        else if (MemSetInst *memSetInst = dyn_cast<MemSetInst>(inst))
        {
        }
        else if (MemCpyInst *memCpyInst = dyn_cast<MemCpyInst>(inst))
        {
            errs() << "MemCpyInst: " << *memCpyInst << "\n";
        }
        else if (CallInst *callInst = dyn_cast<CallInst>(inst))
        {
            errs() << "CallInst: " << *callInst << "\n";
            Value *calledValue = callInst->getCalledOperand();
            std::set<Function *> calledFunctions;
            for (Value *v : dfval->getInclude(calledValue))
            {
                if (Function *f = dyn_cast<Function>(v))
                {
                    calledFunctions.insert(f);
                }
            }
            if (inst->getDebugLoc().getLine())
            {
                for (Value *f : calledFunctions)
                    line2func[inst->getDebugLoc().getLine()].insert(f->getName());
            }

            for (Function *f : calledFunctions)
            {
                DataflowResult<PointsToInfo>::Type fResult;
                for (int i = 0; i < f->getFunctionType()->getNumParams(); ++i)
                {
                    Value *arg = callInst->getArgOperand(i);
                    if (arg->getType()->isPointerTy())
                    {
                        Value *callArg = f->getArg(i);
                        dfval->store(callArg, arg);
                        fResult[&f->getEntryBlock()].first = *dfval;
                    }
                }
                compForwardDataflow(f, this, &fResult, PointsToInfo());
            }
        }
        else
        {
            errs() << "Instruction not handled: " << *inst << "\n";
        }
    }
};