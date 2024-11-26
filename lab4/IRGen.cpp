//
//  IRGen.cpp
//  ECE467 Lab 4
//
//  Created by Tarek Abdelrahman on 2023-10-13.
//  Based on code written by Ao (Dino) Li for ECE467.
//  Copyright © 2023 Tarek Abdelrahman. All rights reserved.
//
//  Permission is hereby granted to use this code in ECE467 at
//  the University of Toronto. It is prohibited to distribute
//  this code, either publicly or to third parties.

#include <iostream>
using namespace std;

#include "IRGen.h"
#include "SymTable.h"
#include "ASTVisitorBase.h"
using namespace smallc;
using namespace llvm;

namespace smallc {

IRGen::IRGen(const std::string moduleName) : ASTVisitorBase(),
                                             prog(nullptr),
                                             TheModule(),
                                             Builder(),
                                             ModuleName(moduleName) {
    TheContext = std::make_unique<llvm::LLVMContext>();
}

unique_ptr<Module> 
IRGen::getModule() {
    Builder.reset();
    return std::move(TheModule);
}

llvm::Type*
IRGen::convertType(TypeNode* type){
    llvm::Type* base;
    switch(type->getTypeEnum()){
        case TypeNode::TypeEnum::Void:
            base = llvm::Type::getVoidTy(*TheContext);
            break;
        case TypeNode::TypeEnum::Int:
            base = llvm::Type::getInt32Ty(*TheContext);
            break;
        case TypeNode::TypeEnum::Bool:
            base = llvm::Type::getInt1Ty(*TheContext);
            break;
    }
    if(type->isArray()){
        ArrayTypeNode* arrType = (ArrayTypeNode*) type;
        return llvm::ArrayType::get(base, arrType->getSize());
    }
    return base;
}

SymTable<VariableEntry>*
IRGen::findTable(IdentifierNode* id){
    ASTNode* parent = id->getParent();
    bool found = false;
    SymTable<VariableEntry>* res = nullptr;
    while (parent && !found){
        if (parent->hasVarTable()){
            if (parent->getParent()){
                ScopeNode* scope = (ScopeNode*)parent;
                res = scope->getVarTable();
            }
            else {
                res = id->getRoot()->getVarTable();
            }
            if (res && res->contains(id->getName()))
                found = true;
        }
        parent = parent->getParent();
    }
    return res;
}

bool isGlobalContext(ASTNode* node) {
    
    ASTNode* parent = node->getParent();
    if (parent != nullptr && dynamic_cast<ProgramNode*>(parent) != nullptr) {
        return true;
    }
    return false;
}

// ECE467 STUDENT: complete the implementation of the visitor functions

void
IRGen::visitASTNode (ASTNode* node) {
    ASTVisitorBase::visitASTNode(node);
}

void 
IRGen::visitArgumentNode (ArgumentNode* arg) {
    if (!arg) return;
    ExprNode* expr = arg->getExpr();
    if (expr) {
        expr->visit(this);
    }
    ASTVisitorBase::visitArgumentNode(arg);
}

void 
IRGen::visitDeclNode (DeclNode* decl) {
    ASTVisitorBase::visitDeclNode(decl);
}

void
IRGen::visitArrayDeclNode (ArrayDeclNode* array) {
    if(!array) return;

    std::string varName = array->getIdent()->getName();
    ArrayTypeNode* arrayTypeNode = array->getType();
    llvm::Type* elementType = convertType(arrayTypeNode);
    int arraySize = arrayTypeNode->getSize();

    // llvm::ArrayType* llvmArrayType = llvm::ArrayType::get(elementType, arraySize);

    if (isGlobalContext(array)) {
        
        llvm::Constant* initVal = llvm::ConstantAggregateZero::get(elementType); // Default initialize to zero
        llvm::GlobalVariable* globalArrayVar = new llvm::GlobalVariable(
            *TheModule,
            elementType,
            false,                                  // Is not constant
            llvm::GlobalValue::CommonLinkage,       // Use 'common' linkage type for global variables
            initVal,                                // Initial value
            varName                                 // Variable name
        );

        array->getRoot()->getVarTable()->setLLVMValue(varName, globalArrayVar);

    } 
    else {
        
        llvm::Function* currentFunc = Builder->GetInsertBlock()->getParent();
        llvm::IRBuilder<> tmpBuilder(&currentFunc->getEntryBlock(), currentFunc->getEntryBlock().begin());


        llvm::AllocaInst* alloca = tmpBuilder.CreateAlloca(elementType, nullptr, varName);
        ScopeNode* scope = dynamic_cast<ScopeNode*>(array->getParent());
        if (scope && scope->getVarTable()) {
            scope->getVarTable()->setLLVMValue(varName, alloca);
        }
    }

    ASTVisitorBase::visitArrayDeclNode(array);
}

void
IRGen::visitFunctionDeclNode (FunctionDeclNode* func) {

    if (!func) return;
    
    llvm::Function* function = TheModule->getFunction(func->getIdent()->getName());

    if (func->getProto()) {
        return;
    }

    llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(*TheContext, "entry", function);
    Builder->SetInsertPoint(entryBlock);

    SymTable<VariableEntry>* varTable = func->getBody()->getVarTable();
    unsigned int index = 0;

    for (auto& arg : function->args()) {
        
        std::string argName = func->getParams()[index]->getIdent()->getName();
        arg.setName(argName);

        llvm::AllocaInst* alloca = Builder->CreateAlloca(arg.getType(), nullptr, argName);
        Builder->CreateStore(&arg, alloca);
        varTable->setLLVMValue(argName, alloca);

        index++;
    }

    // llvm::Type* returnType = function->getReturnType();
    // if (returnType->isVoidTy()) {
    //     // If the function returns void, just create a return void instruction.
    //     Builder->CreateRetVoid();
    // } 
    // else {
    //     // If it returns an int, create a default return instruction (e.g., return 0)
    //     llvm::Value* retVal = llvm::ConstantInt::get(returnType, 0);
    //     Builder->CreateRet(retVal);
    // }

    if (func->getBody()) func->getBody()->visit(this);
    ASTVisitorBase::visitFunctionDeclNode(func);
}

void
IRGen::visitScalarDeclNode (ScalarDeclNode* scalar) {
    if(!scalar) return;

    std::string varName = scalar->getIdent()->getName();
    llvm::Type* varType = convertType(scalar->getType());

    if (isGlobalContext(scalar)) {
        
        llvm::Constant* initVal = llvm::Constant::getNullValue(varType);
        llvm::GlobalVariable* globalVar = new llvm::GlobalVariable(
            *TheModule,
            varType,
            false,                             
            llvm::GlobalValue::CommonLinkage,   
            initVal,                            
            varName            
        );

        scalar->getRoot()->getVarTable()->setLLVMValue(varName, globalVar);
    } 
    else {
        
        llvm::Function* currentFunc = Builder->GetInsertBlock()->getParent();
        llvm::IRBuilder<> tmpBuilder(&currentFunc->getEntryBlock(),
                                     currentFunc->getEntryBlock().begin());
        llvm::AllocaInst* alloca = tmpBuilder.CreateAlloca(varType, nullptr, varName);
        
        ScopeNode* scope = dynamic_cast<ScopeNode*>(scalar->getParent());
        if (scope && scope->getVarTable()) {
            scope->getVarTable()->setLLVMValue(varName, alloca);
        }
    }

    ASTVisitorBase::visitScalarDeclNode(scalar);
}

void 
IRGen::visitExprNode (ExprNode* exp) {

    if(!exp) return;
    if (exp->getType()) exp->getType()->visit(this);

    ASTVisitorBase::visitExprNode(exp);
}

void 
IRGen::visitBinaryExprNode(BinaryExprNode* bin) {
    if(!bin) return;
    ExprNode* left = bin->getLeft();
    ExprNode* right = bin->getRight();
    if (left) left->visit(this);
    if (right) right->visit(this);

    llvm::Value* leftValue = left->getLLVMValue();
    llvm::Value* rightValue = right->getLLVMValue();
    llvm::Value* result = nullptr;

    switch (bin->getOpcode()) {
        case ExprNode::Addition:
            result = Builder->CreateAdd(leftValue, rightValue);
            break;
        case ExprNode::Subtraction:
            result = Builder->CreateSub(leftValue, rightValue);
            break;
        case ExprNode::Multiplication:
            result = Builder->CreateMul(leftValue, rightValue);
            break;
        case ExprNode::Division:
            result = Builder->CreateSDiv(leftValue, rightValue);
            break;
        case ExprNode::And:
            result = Builder->CreateAnd(leftValue, rightValue);
            break;
        case ExprNode::Or:
            result = Builder->CreateOr(leftValue, rightValue);
            break;
        case ExprNode::Equal:
            result = Builder->CreateICmpEQ(leftValue, rightValue);
            break;
        case ExprNode::NotEqual:
            result = Builder->CreateICmpNE(leftValue, rightValue);
            break;
        case ExprNode::LessThan:
            result = Builder->CreateICmpSLT(leftValue, rightValue);
            break;
        case ExprNode::LessorEqual:
            result = Builder->CreateICmpSLE(leftValue, rightValue);
            break;
        case ExprNode::Greater:
            result = Builder->CreateICmpSGT(leftValue, rightValue);
            break;
        case ExprNode::GreaterorEqual:
            result = Builder->CreateICmpSGE(leftValue, rightValue);
            break;
    }

    bin->setLLVMValue(result);

    ASTVisitorBase::visitBinaryExprNode(bin);
}

void 
IRGen::visitBoolExprNode (BoolExprNode* boolExpr) {
    if(!boolExpr) return;

    ExprNode* exp = boolExpr->getValue();
    if (exp) exp->visit(this);

    boolExpr->setLLVMValue(exp->getLLVMValue());

    ASTVisitorBase::visitBoolExprNode(boolExpr);
}

void
IRGen::visitCallExprNode (CallExprNode* call) {
    if(!call) return;

    IdentifierNode* functionIdent = call->getIdent();
    if (functionIdent) functionIdent->visit(this);

    std::string funcName = functionIdent->getName();
    llvm::Function* function = TheModule->getFunction(funcName);


    std::vector<llvm::Value*> args;
    for (auto* argNode : call->getArguments()) {
        if (argNode) {
            argNode->visit(this);
            args.push_back(argNode->getExpr()->getLLVMValue());
        }
    }

    llvm::Value* callInst = Builder->CreateCall(function, args);
    call->setLLVMValue(callInst);

    ASTVisitorBase::visitCallExprNode(call);
}

void 
IRGen::visitConstantExprNode(ConstantExprNode* constant) {
    ASTVisitorBase::visitConstantExprNode(constant);
}

void 
IRGen::visitBoolConstantNode(BoolConstantNode* boolConst) {

    if (!boolConst) return;

    bool value = (boolConst->getVal() != 0);

    llvm::ConstantInt* boolValue = llvm::ConstantInt::get(
        llvm::Type::getInt1Ty(*TheContext),  
        value,                              
        false
    );

    boolConst->setLLVMValue(boolValue);

    ASTVisitorBase::visitBoolConstantNode(boolConst);
}

void 
IRGen::visitIntConstantNode(IntConstantNode* intConst) {

    if (!intConst) return;

    int value = intConst->getVal();

    llvm::ConstantInt* intValue = llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(*TheContext),
        value,                     
        true                 
    );

    intConst->setLLVMValue(intValue);
    
    ASTVisitorBase::visitIntConstantNode(intConst);
}

void
IRGen::visitIntExprNode(IntExprNode* intExpr) {
    if(!intExpr) return;

    ExprNode* exp = intExpr->getValue();
    if (exp) exp->visit(this);

    intExpr->setLLVMValue(exp->getLLVMValue());

    ASTVisitorBase::visitIntExprNode(intExpr);
}

void
IRGen::visitReferenceExprNode(ReferenceExprNode* ref) {
    if(!ref) return;

    IdentifierNode* identNode = ref->getIdent();
    if (identNode) identNode->visit(this);
    IntExprNode* indexExpr = ref->getIndex();
    if (indexExpr) indexExpr->visit(this);

    std::string varName = identNode->getName();
    // llvm::Value* variable = ref->getLLVMValue();

    llvm::Value* variable = findTable(identNode)->get(varName).getValue();
    TypeNode* refType = findTable(identNode)->get(varName).getType();
    llvm::Value* loadedValue = nullptr;

    if (indexExpr) {
        
        llvm::Value* indexValue = indexExpr->getLLVMValue();

        // indexValue = Builder->CreateIntCast(indexValue, llvm::Type::getInt32Ty(*TheContext), false, "indexcast");

        llvm::Value* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0);
        std::vector<llvm::Value*> indices = {zero, indexValue};

        llvm::Value* elementPtr = Builder->CreateGEP(
            convertType(findTable(identNode)->get(varName).getType()),
            variable,                           
            indices,                            
            varName       
        );

        if (refType->getTypeEnum() == TypeNode::Int) {
            loadedValue = Builder->CreateLoad(llvm::Type::getInt32Ty(*TheContext), elementPtr);
        }
        else if (refType->getTypeEnum() == TypeNode::Bool) {
            loadedValue = Builder->CreateLoad(llvm::Type::getInt1Ty(*TheContext), elementPtr);
        }
        

    } else {
        if (refType->getTypeEnum() == TypeNode::Int) {
            loadedValue = Builder->CreateLoad(llvm::Type::getInt32Ty(*TheContext), variable);
        }
        else if (refType->getTypeEnum() == TypeNode::Bool) {
            loadedValue = Builder->CreateLoad(llvm::Type::getInt1Ty(*TheContext), variable);
        }
    }

    ref->setLLVMValue(loadedValue);

    ASTVisitorBase::visitReferenceExprNode(ref);
}

void 
IRGen::visitUnaryExprNode(UnaryExprNode* unary) {
    if(!unary) return;


    ExprNode* operand = unary->getOperand();
    if (operand) operand->visit(this);

    llvm::Value* operandValue = operand->getLLVMValue();
    llvm::Value* result = nullptr;

    switch (unary->getOpcode()) {
        case ExprNode::Minus:
            result = Builder->CreateNeg(operandValue);
            break;
        case ExprNode::Not:
            result = Builder->CreateNot(operandValue);
            break;
    }

    unary->setLLVMValue(result);

    ASTVisitorBase::visitUnaryExprNode(unary);
}

void 
IRGen::visitIdentifierNode(IdentifierNode* id) {
    ASTVisitorBase::visitIdentifierNode(id);
}

void
IRGen::visitParameterNode(ParameterNode* param) {
    if(!param) return;

    if (param->getType()) param->getType()->visit(this);
    if (param->getIdent()) param->getIdent()->visit(this);

    ASTVisitorBase::visitParameterNode(param);
}

void
IRGen::visitProgramNode(ProgramNode* prg) {

    TheContext = std::make_unique<LLVMContext>();
    TheModule = std::make_unique<Module>(ModuleName, *TheContext);
    Builder = std::make_unique<IRBuilder<>>(*TheContext);

    SymTable<FunctionEntry>* funcTable = prg->getFuncTable();

    for (auto& funcPair : *funcTable->getTable()) {
        const std::string& funcName = funcPair.first;
        FunctionEntry& funcEntry = funcPair.second;

        llvm::Type* returnType = convertType(funcEntry.returnType);

        std::vector<llvm::Type*> argTypes;        
        for (TypeNode* paramType : funcEntry.parameterTypes) {
            argTypes.push_back(convertType(paramType));
        }

        llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, argTypes, false);

        llvm::Function* llvmFunction = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            funcName,
            TheModule.get()
        );

        if (funcName == "writeBool" && llvmFunction->arg_size() == 1) {
            auto arg = llvmFunction->arg_begin(); 
            arg->addAttr(llvm::Attribute::ZExt);
        }

        funcEntry.value = llvmFunction;
    }

    ASTVisitorBase::visitProgramNode(prg);
}

void 
IRGen::visitStmtNode(StmtNode* stmt) {
    ASTVisitorBase::visitStmtNode(stmt);
}

void 
IRGen::visitAssignStmtNode(AssignStmtNode* assign) {
    if(!assign) return;

    ReferenceExprNode* target = assign->getTarget();
    ExprNode* valueExpr = assign->getValue();
    
    if (valueExpr) valueExpr->visit(this);

    llvm::Value* value = valueExpr->getLLVMValue();

    std::string targetName = target->getIdent()->getName();
    llvm::Value* targetValue = findTable(target->getIdent())->get(targetName).getValue();

    IntExprNode* indexExpr = target->getIndex();
    if (indexExpr) {
        
        indexExpr->visit(this);
        llvm::Value* index = indexExpr->getLLVMValue();

        // index = Builder->CreateIntCast(index, llvm::Type::getInt32Ty(*TheContext), false, "indexcast");

        llvm::Value* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0);
        std::vector<llvm::Value*> indices = {zero, index};

        llvm::Value* elementPtr = Builder->CreateGEP(
            convertType(findTable(target->getIdent())->get(targetName).getType()),
            targetValue,                       
            indices,                       
            targetName + "_elementptr"                 
        );
        
        Builder->CreateStore(value, elementPtr);
    } else {
        Builder->CreateStore(value, targetValue);
    }

    ASTVisitorBase::visitAssignStmtNode(assign);
}

void 
IRGen::visitExprStmtNode(ExprStmtNode* expr) {
    if(!expr) return;

    if (expr->getExpr()) expr->getExpr()->visit(this);

    ASTVisitorBase::visitExprStmtNode(expr);
}
void 
IRGen::visitIfStmtNode(IfStmtNode* ifStmt) {
    if(!ifStmt) return;

    // visit member nodes
    // ExprNode* condition = ifStmt->getCondition();
    // if (condition) condition->visit(this);
    // if (ifStmt->getThen()) ifStmt->getThen()->visit(this);
    // if (ifStmt->getElse()) ifStmt->getElse()->visit(this);

    llvm::Function* parentFunction = Builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(*TheContext, "then", parentFunction);
    llvm::BasicBlock* elseBlock = ifStmt->getHasElse() ? llvm::BasicBlock::Create(*TheContext, "else", parentFunction) : nullptr;
    llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(*TheContext, "merge", parentFunction);

    ExprNode* conditionExpr = ifStmt->getCondition();
    if (conditionExpr) {
        conditionExpr->visit(this);
    }

    llvm::Value* conditionValue = conditionExpr->getLLVMValue();

    if (elseBlock) {
        Builder->CreateCondBr(conditionValue, thenBlock, elseBlock);
    } else {
        Builder->CreateCondBr(conditionValue, thenBlock, mergeBlock);
    }

    Builder->SetInsertPoint(thenBlock);
    StmtNode* thenStmt = ifStmt->getThen();
    if (thenStmt) {
        thenStmt->visit(this);
    }
    Builder->CreateBr(mergeBlock);

    if (elseBlock) {
        Builder->SetInsertPoint(elseBlock);
        StmtNode* elseStmt = ifStmt->getElse();
        if (elseStmt) {
            elseStmt->visit(this);
        }
        Builder->CreateBr(mergeBlock);
    }

    Builder->SetInsertPoint(mergeBlock);

    ASTVisitorBase::visitIfStmtNode(ifStmt);
}

void 
IRGen::visitReturnStmtNode(ReturnStmtNode* ret) {
    if(!ret) return;

    ExprNode* retExpr = ret->getReturn();
    if (retExpr) {
        retExpr->visit(this);

        llvm::Value* retValue = retExpr->getLLVMValue();

        Builder->CreateRet(retValue);
    }
    else {
        Builder->CreateRetVoid();
    }

    ASTVisitorBase::visitReturnStmtNode(ret);
}

void 
IRGen::visitScopeNode(ScopeNode* scope) {
    if(!scope) return;

    std::vector<DeclNode*> decls = scope->getDeclarations();
    for (DeclNode* decl : decls) decl->visit(this);

    ASTVisitorBase::visitScopeNode(scope);
}

void 
IRGen::visitWhileStmtNode(WhileStmtNode* whileStmt) {
    if(!whileStmt) return;

    llvm::Function* parentFunction = Builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* conditionBlock = llvm::BasicBlock::Create(*TheContext, "while.cond", parentFunction);
    llvm::BasicBlock* bodyBlock = llvm::BasicBlock::Create(*TheContext, "while.body", parentFunction);
    llvm::BasicBlock* exitBlock = llvm::BasicBlock::Create(*TheContext, "while.exit", parentFunction);

    Builder->CreateBr(conditionBlock);

    Builder->SetInsertPoint(conditionBlock);

    ExprNode* conditionExpr = whileStmt->getCondition();
    if (conditionExpr) {
        conditionExpr->visit(this);
    }

    llvm::Value* conditionValue = conditionExpr->getLLVMValue();

    Builder->CreateCondBr(conditionValue, bodyBlock, exitBlock);

    Builder->SetInsertPoint(bodyBlock);

    StmtNode* bodyStmt = whileStmt->getBody();
    if (bodyStmt) {
        bodyStmt->visit(this);
    }

    Builder->CreateBr(conditionBlock);

    Builder->SetInsertPoint(exitBlock);

    ASTVisitorBase::visitWhileStmtNode(whileStmt);
}

void 
IRGen::visitTypeNode(TypeNode* type) {
    ASTVisitorBase::visitTypeNode(type);
}

void 
IRGen::visitPrimitiveTypeNode(PrimitiveTypeNode* type) {
    ASTVisitorBase::visitPrimitiveTypeNode(type);
}

void 
IRGen::visitArrayTypeNode(ArrayTypeNode* type) {
    ASTVisitorBase::visitArrayTypeNode(type);
}

} // namespace smallc
