//
//  SemanticAnalyzer.cpp
//  ECE467 Lab 3
//
//  Created by Tarek Abdelrahman on 2023-09-13.
//  Based on code written by Ao (Dino) Li for ECE467.
//  Copyright © 2023 Tarek Abdelrahman. All rights reserved.
//
//  Permission is hereby granted to use this code in ECE467 at
//  the University of Toronto. It is prohibited to distribute
//  this code, either publicly or to third parties.

#include <iostream>
using namespace std;

#include "SemanticAnalyzer.h"
#include "SymTable.h"
#include "assert.h"
using namespace smallc;

namespace smallc {

SemaError::SemaError (ErrorEnum code_, std::pair<unsigned int, unsigned int> location_) : code(code_), location(location_), msg() { }

SemaError::SemaError(ErrorEnum code_, std::pair<unsigned int, unsigned int> location_, std::string msg_) : code(code_), location(location_), msg(msg_) { }

// Constructor
SemanticAnalyzer::SemanticAnalyzer (): ASTVisitorBase(), prog(nullptr), errors() {}

// Print all the error messages at once
void
SemanticAnalyzer::printErrorMsgs () {
    for (const auto& e : errors) {
        std::cout << "sema: " << e.location.first
                  << ":" << e.location.second << " : ";
        switch (e.code) {
            case SemaError::IdentReDefined:
                std::cout << "redefinition of";
                break;
            case SemaError::IdentUnDefined:
                std::cout << "use of undefined identifier";
                break;
            case SemaError::NoMatchingDef:
                std::cout << "no matching definition for";
                break;
            case SemaError::MisMatchedReturn:
                std::cout << "mismatched return statement";
                break;
            case SemaError::InconsistentDef:
                std::cout << "definition inconsistent with earlier definition of";
                break;
            case SemaError::InvalidCond:
                std::cout << "invalid condition in";
                break;
            case SemaError::TypeMisMatch:
                std::cout << "type mismatch";
                break;
            case SemaError::InvalidArraySize:
                std::cout << "size cannot be negative for array";
                break;
            case SemaError::InvalidAccess:
                std::cout << "invalid use of identifier";
                break;
            default:
                std::cout << "unknown error number";
                break;
        }
        std::cout << " " << e.msg << std::endl;
    }
}

// Add an error to the list of errors
void
SemanticAnalyzer::addError (const SemaError& err) {
    errors.push_back(err);
}

// ECE467 STUDENT: implement the visitor classes, perfoming
// semantic analysis
void SemanticAnalyzer::visitASTNode(ASTNode *node) {
    ASTVisitorBase::visitASTNode(node);
}

void SemanticAnalyzer::visitArgumentNode(ArgumentNode *arg) {
    if (!arg) return;
    if (arg->getExpr()) arg->getExpr()->visit(this);

    ExprNode* argExp;
    IntExprNode* argIntExp;
    ExprNode* argIntExpChild;
    ReferenceExprNode* argIntExpRef;

    argExp = arg->getExpr();
    argIntExp = dynamic_cast<IntExprNode*>(argExp);
    if (argIntExp) argIntExpChild = argIntExp->getValue();
    argIntExpRef = dynamic_cast<ReferenceExprNode*>(argIntExpChild);

    if (argIntExpRef) {

        SymTable<VariableEntry>* currentScopeTable = nullptr;
        ASTNode* immediateParentWithVarST = argIntExpRef;
        IdentifierNode* refID = argIntExpRef->getIdent();

        TypeNode* STentryType = nullptr;

        while (immediateParentWithVarST) {
            
            if (immediateParentWithVarST->hasVarTable()) {

                ProgramNode* immediateParentIsPrg = dynamic_cast<ProgramNode*>(immediateParentWithVarST);
                ScopeNode* immediateParentIsScope = nullptr;
                if (immediateParentIsPrg) {

                    currentScopeTable = immediateParentIsPrg->getVarTable();

                    if (currentScopeTable->contains(refID->getName())) {
                        STentryType = currentScopeTable->get(refID->getName()).getType();
                        break;
                    }
                }
                else{

                    immediateParentIsScope = dynamic_cast<ScopeNode*>(immediateParentWithVarST);
                    currentScopeTable = immediateParentIsScope->getVarTable();

                    if (currentScopeTable->contains(refID->getName())) {
                        STentryType = currentScopeTable->get(refID->getName()).getType();
                        break;
                    }
                }
            }

            immediateParentWithVarST = immediateParentWithVarST->getParent();
        }

        ArrayTypeNode* arrTypeNode = dynamic_cast<ArrayTypeNode*>(STentryType);
        if (arrTypeNode && !argIntExpRef->getIndex())arg->setArray();

        // ArrayTypeNode* arr = dynamic_cast<ArrayTypeNode*>(argIntExpRef->getType());

        // if (arr) arg->setArray();

        // cout << "arg type: " << argExp->getType()->getTypeEnum() << endl;
        // if (arg->isArray()) cout << "is arr\n";
        // else cout << "is not arr\n";
    }

    ASTVisitorBase::visitArgumentNode(arg);
}

void SemanticAnalyzer::visitDeclNode(DeclNode *decl) {
    ASTVisitorBase::visitDeclNode(decl);
}

void SemanticAnalyzer::visitArrayDeclNode(ArrayDeclNode *array) {

    if(!array) return;

    // find var symtable of immediate parent
    SymTable<VariableEntry>* currentScopeTable = nullptr;
    ASTNode* immediateParentWithVarST = nullptr;

    do {
        immediateParentWithVarST = array->getParent();
    } while (immediateParentWithVarST && !immediateParentWithVarST->hasVarTable());

    ProgramNode* immediateParentIsPrg = dynamic_cast<ProgramNode*>(immediateParentWithVarST);
    ScopeNode* immediateParentIsScope = nullptr;
    if (immediateParentIsPrg) {
        currentScopeTable = immediateParentIsPrg->getVarTable();
    }
    else{
        immediateParentIsScope = dynamic_cast<ScopeNode*>(immediateParentWithVarST);
        currentScopeTable = immediateParentIsScope->getVarTable();
    }

    // check if variable is redefined in the current scope
    std::string arrayVarName = array->getIdent()->getName();
    ArrayTypeNode* arrayType = dynamic_cast<ArrayTypeNode*>(array->getType());
    if (currentScopeTable->contains(arrayVarName)) {
        addError(SemaError(SemaError::IdentReDefined, array->getLocation(), arrayVarName));
        return;
    } 
    // Check if array size is a positive integer
    else if (arrayType && arrayType->getSize() <= 0) {
        addError(SemaError(SemaError::InvalidArraySize, array->getLocation(), array->getIdent()->getName()));
    }
    else {
        VariableEntry varEntry(array->getType());
        currentScopeTable->insert(arrayVarName, varEntry);
    }
    ASTVisitorBase::visitArrayDeclNode(array);
}

void SemanticAnalyzer::visitFunctionDeclNode(FunctionDeclNode *func) {

    if(!func) return;
    
    // find var symtable of immediate parent
    SymTable<FunctionEntry>* currentScopeTable = func->getRoot()->getFuncTable();
    
    if (currentScopeTable) {

        // check if function is redefined in the current scope
        std::string funcName = func->getIdent()->getName();
        if (currentScopeTable->contains(funcName)) {
            FunctionEntry existingEntry = currentScopeTable->get(funcName);
            if (!existingEntry.proto && !func->getProto()) {
                addError(SemaError(SemaError::IdentReDefined, func->getLocation(), funcName));
            }
            // Check if function definition matches prototype
            else {
                PrimitiveTypeNode* returnType = func->getRetType();
                std::vector<ParameterNode*> params = func->getParams();
                std::vector<TypeNode*> paramTypes;
                for (auto param : params) {
                    paramTypes.push_back(param->getType());
                }

                if (existingEntry.returnType->getTypeEnum() != returnType->getTypeEnum() ||
                    existingEntry.parameterTypes.size() != paramTypes.size()) {
                    addError(SemaError(SemaError::InconsistentDef, func->getLocation(), funcName));
                } 
                else {
                    bool inconsistent = false;
                    for (size_t i = 0; i < paramTypes.size(); ++i) {
                        if (existingEntry.parameterTypes[i]->getTypeEnum() != paramTypes[i]->getTypeEnum()) {
                            addError(SemaError(SemaError::InconsistentDef, func->getLocation(), funcName));
                            inconsistent = true;
                            break;
                        }
                    }
                    if (!inconsistent) {
                        std::vector<TypeNode*> paraTypes;
                        for (auto param : func->getParams()) {

                            TypeNode * type_ = param->getType();
                            PrimitiveTypeNode* primTypeNode = dynamic_cast<PrimitiveTypeNode*>(type_);
                            ArrayTypeNode* arrTypeNode = nullptr;

                            if (primTypeNode) {
                                paraTypes.push_back(primTypeNode);
                            }
                            else {
                                ArrayTypeNode* arrTypeNode = dynamic_cast<ArrayTypeNode*>(type_);
                                paraTypes.push_back(arrTypeNode);
                            }
                        }

                        FunctionEntry funcEntry(func->getRetType(), paraTypes);
                        if (func->getProto()) funcEntry.proto = true;
                        else funcEntry.proto = false;
                        currentScopeTable->insert(funcName, funcEntry);
                    }
                }
            }
        } 
        // insert
        else {
            std::vector<TypeNode*> paraTypes;
            for (auto param : func->getParams()) {

                TypeNode * type_ = param->getType();
                PrimitiveTypeNode* primTypeNode = dynamic_cast<PrimitiveTypeNode*>(type_);
                ArrayTypeNode* arrTypeNode = nullptr;

                if (primTypeNode) {
                    paraTypes.push_back(primTypeNode);
                }
                else {
                    ArrayTypeNode* arrTypeNode = dynamic_cast<ArrayTypeNode*>(type_);
                    paraTypes.push_back(arrTypeNode);
                }
            }

            FunctionEntry funcEntry(func->getRetType(), paraTypes);
            if (func->getProto()) funcEntry.proto = true;
            else funcEntry.proto = false;
            currentScopeTable->insert(funcName, funcEntry);
        }
    }

    // Push function parameters into its variable scope
    SymTable<VariableEntry>* funcVarTable = func->getBody() ? func->getBody()->getVarTable() : nullptr;
    if (funcVarTable) {
        for (auto param : func->getParams()) {

            std::string paramName = param->getIdent()->getName();
            PrimitiveTypeNode* primTypeNode = dynamic_cast<PrimitiveTypeNode*>(param->getType());
            ArrayTypeNode* arrTypeNode = dynamic_cast<ArrayTypeNode*>(param->getType());
            VariableEntry varEntry = primTypeNode ? VariableEntry(primTypeNode) : VariableEntry(arrTypeNode);

            // reference solution does not check for redifinition in func parameters
            // if (funcVarTable->contains(paramName)) {
            //     addError(SemaError(SemaError::IdentReDefined, param->getLocation(), paramName));
            funcVarTable->insert(paramName, varEntry);
        }
    }

    // visit member nodes
    if (func->getBody()) func->getBody()->visit(this);
    ASTVisitorBase::visitFunctionDeclNode(func);
}

void SemanticAnalyzer::visitScalarDeclNode(ScalarDeclNode *scalar) {

    if(!scalar) return;

    // find var symtable of immediate parent
    SymTable<VariableEntry>* currentScopeTable = nullptr;
    ASTNode* immediateParentWithVarST = nullptr;

    do {
        immediateParentWithVarST = scalar->getParent();
    } while (immediateParentWithVarST && !immediateParentWithVarST->hasVarTable());

    ProgramNode* immediateParentIsPrg = dynamic_cast<ProgramNode*>(immediateParentWithVarST);
    ScopeNode* immediateParentIsScope = nullptr;
    if (immediateParentIsPrg) {
        currentScopeTable = immediateParentIsPrg->getVarTable();
    }
    else{
        immediateParentIsScope = dynamic_cast<ScopeNode*>(immediateParentWithVarST);
        currentScopeTable = immediateParentIsScope->getVarTable();
    }

    // check if variable is redefined in the current scope
    std::string scalarVarName = scalar->getIdent()->getName();
    if (currentScopeTable->contains(scalarVarName)) {
        addError(SemaError(SemaError::IdentReDefined, scalar->getLocation(), scalarVarName));
    } else {
        VariableEntry varEntry(scalar->getType());
        currentScopeTable->insert(scalarVarName, varEntry);
    }
    ASTVisitorBase::visitScalarDeclNode(scalar);
}

void SemanticAnalyzer::visitExprNode(ExprNode *exp) {

    if(!exp) return;

    // visit member nodes
    if (exp->getType()) exp->getType()->visit(this);

    ASTVisitorBase::visitExprNode(exp);
}

void SemanticAnalyzer::visitBinaryExprNode(BinaryExprNode *bin) {

    if(!bin) return;

    // visit member nodes
    ExprNode* left = bin->getLeft();
    ExprNode* right = bin->getRight();
    if (left) left->visit(this);
    if (right) right->visit(this);
    
    // check arithmatic op
    ExprNode::Opcode op = bin->getOpcode();
    if (op == ExprNode::Addition || op == ExprNode::Subtraction || 
        op == ExprNode::Multiplication || op == ExprNode::Division || op == ExprNode::Minus) {

        BoolExprNode* leftBool = dynamic_cast<BoolExprNode*>(left);
        BoolExprNode* rightBool = dynamic_cast<BoolExprNode*>(right);

        if (leftBool && leftBool->getType() && leftBool->getType()->getTypeEnum() != TypeNode::Int) {
            addError(SemaError(SemaError::TypeMisMatch, bin->getLocation()));
            bin->setTypeVoid();
        }

        else if (rightBool && rightBool->getType() && rightBool->getType()->getTypeEnum() != TypeNode::Int) {
            addError(SemaError(SemaError::TypeMisMatch, bin->getLocation()));
            bin->setTypeVoid();
        }

        // type dismatch if any operand is bool
        else if (left->getType() && right->getType()) {

            if (left->getType()->getTypeEnum() != TypeNode::Int || right->getType()->getTypeEnum() != TypeNode::Int) {
                addError(SemaError(SemaError::TypeMisMatch, bin->getLocation()));
                bin->setTypeVoid();
            }  
        }
    }

    // check comparison op
    if (op == ExprNode::Equal || op == ExprNode::NotEqual ||
        op == ExprNode::LessThan || op == ExprNode::LessorEqual ||
        op == ExprNode::Greater || op == ExprNode::GreaterorEqual) {

        // type dismatch if operands are different types
        if (left->getType() && right->getType()) {

            if (left->getType()->getTypeEnum() && right->getType()->getTypeEnum() &&
                left->getType()->getTypeEnum() != right->getType()->getTypeEnum() ||
                left->getType()->getTypeEnum() == TypeNode::Void || right->getType()->getTypeEnum() == TypeNode::Void) {

                addError(SemaError(SemaError::TypeMisMatch, bin->getLocation()));
                bin->setTypeVoid();
            }
        }
    }

    // check logical op
    if (op == ExprNode::And || op == ExprNode::Or) {

        IntExprNode* leftInt = dynamic_cast<IntExprNode*>(left);
        IntExprNode* rightInt = dynamic_cast<IntExprNode*>(right);

        if (leftInt && leftInt->getType() && leftInt->getType()->getTypeEnum() != TypeNode::Bool) {
            addError(SemaError(SemaError::TypeMisMatch, bin->getLocation()));
            bin->setTypeVoid();
        }

        else if (rightInt && rightInt->getType() && rightInt->getType()->getTypeEnum() != TypeNode::Bool) {
            addError(SemaError(SemaError::TypeMisMatch, bin->getLocation()));
            bin->setTypeVoid();
        }

        else if (left->getType() && right->getType()) {

            if (left->getType()->getTypeEnum() != TypeNode::Bool || right->getType()->getTypeEnum() != TypeNode::Bool) {
                addError(SemaError(SemaError::TypeMisMatch, bin->getLocation()));
                bin->setTypeVoid();
            }
        } 
    }

    ASTVisitorBase::visitBinaryExprNode(bin);
}

void SemanticAnalyzer::visitBoolExprNode(BoolExprNode *boolExpr) {

    if(!boolExpr) return;

    // visit member nodes
    ExprNode* exp = boolExpr->getValue();
    if (exp) exp->visit(this);

    UnaryExprNode* uexp = dynamic_cast<UnaryExprNode*>(exp);
    if (uexp) boolExpr->setType(uexp->getType());

    BinaryExprNode* binexp = dynamic_cast<BinaryExprNode*>(exp);
    if (binexp) boolExpr->setType(binexp->getType());

    ASTVisitorBase::visitBoolExprNode(boolExpr);
}

void SemanticAnalyzer::visitCallExprNode(CallExprNode *call) {

    if(!call) return;

    // visit member nodes
    if (call->getIdent()) call->getIdent()->visit(this);
    std::vector<ArgumentNode *> args = call->getArguments();
    for (ArgumentNode * arg : args) arg->visit(this);

    // check if the function is defined
    std::string funcName = call->getIdent()->getName();
    ASTNode* prg = call;
    while (prg->getParent()) prg = prg->getParent();
    ProgramNode* prog= dynamic_cast<ProgramNode*>(prg);

    if (!prog) return;
    SymTable<FunctionEntry>* funcTable = prog->getFuncTable();

    bool validInvocation = true;

    if (!funcTable->contains(funcName)) {
        addError(SemaError(SemaError::IdentUnDefined, call->getLocation(), funcName));
    } 

    // if 
    else {
        FunctionEntry funcEntry = funcTable->get(funcName);
        std::vector<TypeNode*> expectedParams = funcEntry.getParameterTypes();

        // check if the number of arguments matches
        if (expectedParams.size() != args.size()) {
            validInvocation = false;
        } 
        else {
            // check if argument types match
            for (size_t i = 0; i < expectedParams.size(); ++i) {

                if (args[i]->getExpr()->getType()->getTypeEnum() != expectedParams[i]->getTypeEnum()) {
                    validInvocation = false;
                }
                else if (args[i]->getExpr()->getType()->getTypeEnum() == TypeNode::Void){
                    validInvocation = false;
                }
                else if (args[i]->isArray() != expectedParams[i]->isArray()){
                    validInvocation = false;
                }
            }
        }
        // If the invocation is correct, assign the return type of the function to the call expression
        if (validInvocation) {
            call->setType(funcEntry.getReturnType());
        } else {

            addError(SemaError(SemaError::NoMatchingDef, call->getLocation(), funcName));
            call->setTypeVoid();
        }
    }

    ASTVisitorBase::visitCallExprNode(call);
}

void SemanticAnalyzer::visitConstantExprNode(ConstantExprNode *constant) {
    ASTVisitorBase::visitConstantExprNode(constant);
}

void SemanticAnalyzer::visitBoolConstantNode(BoolConstantNode *boolConst) {
    ASTVisitorBase::visitBoolConstantNode(boolConst);
}

void SemanticAnalyzer::visitIntConstantNode(IntConstantNode *intConst) {
    ASTVisitorBase::visitIntConstantNode(intConst);
}

void SemanticAnalyzer::visitIntExprNode(IntExprNode *intExpr) {

    if(!intExpr) return;

    // visit member nodes
    ExprNode* exp = intExpr->getValue();
    if (exp) exp->visit(this);

    UnaryExprNode* uexp = dynamic_cast<UnaryExprNode*>(exp);
    if (uexp) intExpr->setType(uexp->getType());

    BinaryExprNode* binexp = dynamic_cast<BinaryExprNode*>(exp);
    if (binexp) intExpr->setType(binexp->getType());

    ReferenceExprNode* refexp = dynamic_cast<ReferenceExprNode*>(exp);
    if (refexp) intExpr->setType(refexp->getType());

    ASTVisitorBase::visitIntExprNode(intExpr);
}

void SemanticAnalyzer::visitReferenceExprNode(ReferenceExprNode *ref) {

    if(!ref) return;

    // visit member nodes
    if (ref->getIdent()) ref->getIdent()->visit(this);
    if (ref->getIndex()) ref->getIndex()->visit(this);

    IdentifierNode * refID = ref->getIdent();
    IntExprNode* refIdx = ref->getIndex();
    ArgumentNode* arg = dynamic_cast<ArgumentNode*>(ref->getParent()->getParent());

    // check if defined
    SymTable<VariableEntry>* currentScopeTable = nullptr;
    ASTNode* immediateParentWithVarST = ref;

    bool defined = false;
    TypeNode* STentryType = nullptr;

    while (immediateParentWithVarST) {
        
        if (immediateParentWithVarST->hasVarTable()) {

            ProgramNode* immediateParentIsPrg = dynamic_cast<ProgramNode*>(immediateParentWithVarST);
            ScopeNode* immediateParentIsScope = nullptr;
            if (immediateParentIsPrg) {

                currentScopeTable = immediateParentIsPrg->getVarTable();

                if (currentScopeTable->contains(refID->getName())) {
                    defined = true;
                    STentryType = currentScopeTable->get(refID->getName()).getType();
                    break;
                }
            }
            else{

                immediateParentIsScope = dynamic_cast<ScopeNode*>(immediateParentWithVarST);
                currentScopeTable = immediateParentIsScope->getVarTable();

                if (currentScopeTable->contains(refID->getName())) {
                    defined = true;
                    STentryType = currentScopeTable->get(refID->getName()).getType();
                    break;
                }
            }
        }

        immediateParentWithVarST = immediateParentWithVarST->getParent();
    }

    if (!defined) {
        addError(SemaError(SemaError::IdentUnDefined, ref->getLocation(), refID->getName()));
        ref->setTypeVoid();
    }

    // check is int is used as arr or vise versa
    else {

        PrimitiveTypeNode* primTypeNode = dynamic_cast<PrimitiveTypeNode*>(STentryType);
        ArrayTypeNode* arrTypeNode = dynamic_cast<ArrayTypeNode*>(STentryType);
        if (primTypeNode) {
            
            if(refIdx) {
                addError(SemaError(SemaError::InvalidAccess, ref->getLocation(), refID->getName()));

                // set void for error chaining
                ref->setTypeVoid();
            }
            else {
                ref->setType(primTypeNode);
            }
        }
        else {
            
            if(!refIdx && !arg) {
                addError(SemaError(SemaError::InvalidAccess, ref->getLocation(), refID->getName()));
                
                // set void for error chaining
                // arrTypeNode->setType(smallc::TypeNode::TypeEnum::Void);
                PrimitiveTypeNode* temp = new PrimitiveTypeNode(smallc::TypeNode::TypeEnum::Void);
                ref->setType(temp);
            }
            else {
                PrimitiveTypeNode* temp = new PrimitiveTypeNode(arrTypeNode->getTypeEnum());
                ref->setType(temp);
            }
        }
    }

    ASTVisitorBase::visitReferenceExprNode(ref);
}

void SemanticAnalyzer::visitUnaryExprNode(UnaryExprNode *unary) {

    if(!unary) return;

    // visit member nodes
    ExprNode* operand = unary->getOperand();
    if (operand) operand->visit(this);

    ExprNode::Opcode op = unary->getOpcode();
    if (op == ExprNode::Not && operand && operand->getType() && operand->getType()->getTypeEnum() != TypeNode::Bool) {
        addError(SemaError(SemaError::TypeMisMatch, {unary->getLocation().first, unary->getLocation().second + 1}));
        unary->setTypeVoid();
    } 
    else if (op == ExprNode::Minus && operand && operand->getType() && operand->getType()->getTypeEnum() != TypeNode::Int) {
        addError(SemaError(SemaError::TypeMisMatch, {unary->getLocation().first, unary->getLocation().second + 1}));
        unary->setTypeVoid();
    }

    ASTVisitorBase::visitUnaryExprNode(unary);
}

void SemanticAnalyzer::visitIdentifierNode(IdentifierNode *id) {
    ASTVisitorBase::visitIdentifierNode(id);
}

void SemanticAnalyzer::visitParameterNode(ParameterNode *param) {

    if(!param) return;

    // visit member nodes
    if (param->getType()) param->getType()->visit(this);
    if (param->getIdent()) param->getIdent()->visit(this);

    ASTVisitorBase::visitParameterNode(param);
}

void SemanticAnalyzer::visitProgramNode(ProgramNode *prg) {
    
    SymTable<FunctionEntry> *funcST;
    std::vector<TypeNode*> paramTypeVec;
    FunctionEntry* libFunc;
    PrimitiveTypeNode* retType;
    PrimitiveTypeNode* paramType;

    if (prg->useIo()){
        funcST = prg->getFuncTable();

        //readBool
        retType = new PrimitiveTypeNode(TypeNode::TypeEnum::Bool);
        libFunc = new FunctionEntry(retType, paramTypeVec);
        libFunc->proto = true;
        funcST->insert("readBool", *libFunc);

        //readInt
        retType = new PrimitiveTypeNode(TypeNode::TypeEnum::Int);
        libFunc = new FunctionEntry(retType, paramTypeVec);
        libFunc->proto = true;
        funcST->insert("readInt", *libFunc);

        //newLine
        retType = new PrimitiveTypeNode(TypeNode::TypeEnum::Void);
        libFunc = new FunctionEntry(retType, paramTypeVec);
        libFunc->proto = true;
        funcST->insert("newLine", *libFunc);

        //writeBool
        retType = new PrimitiveTypeNode(TypeNode::TypeEnum::Void);
        paramType = new PrimitiveTypeNode(TypeNode::TypeEnum::Bool);
        paramTypeVec.clear();
        paramTypeVec.push_back(paramType);
        libFunc = new FunctionEntry(retType, paramTypeVec);
        libFunc->proto = true;
        funcST->insert("writeBool", *libFunc);

        //writeInt
        retType = new PrimitiveTypeNode(TypeNode::TypeEnum::Void);
        paramType = new PrimitiveTypeNode(TypeNode::TypeEnum::Int);
        paramTypeVec.clear();
        paramTypeVec.push_back(paramType);
        libFunc = new FunctionEntry(retType, paramTypeVec);
        libFunc->proto = true;
        funcST->insert("writeInt", *libFunc);
    }

    ASTVisitorBase::visitProgramNode(prg);
}

void SemanticAnalyzer::visitStmtNode(StmtNode *stmt) {
    ASTVisitorBase::visitStmtNode(stmt);
}

void SemanticAnalyzer::visitAssignStmtNode(AssignStmtNode *assign) {

    if(!assign) return;

    // visit member nodes
    ReferenceExprNode* target = assign->getTarget();
    ExprNode* value = assign->getValue();
    if (target) target->visit(this);
    if (value) value->visit(this);

    // Type check for assignment
    if ((target && target->getType()) && (value && value->getType())) {

        if (target->getType()->getTypeEnum() != value->getType()->getTypeEnum()) {
            addError(SemaError(SemaError::TypeMisMatch, assign->getLocation()));
        }

        else if (target->getType()->getTypeEnum() == TypeNode::TypeEnum::Void) {
            addError(SemaError(SemaError::TypeMisMatch, assign->getLocation()));
        }

        else if (value->getType()->getTypeEnum() == TypeNode::TypeEnum::Void) {
            addError(SemaError(SemaError::TypeMisMatch, assign->getLocation()));
        }  
    }

    ASTVisitorBase::visitAssignStmtNode(assign);
}

void SemanticAnalyzer::visitExprStmtNode(ExprStmtNode *expr) {

    if(!expr) return;

    // visit member nodes
    if (expr->getExpr()) expr->getExpr()->visit(this);

    ASTVisitorBase::visitExprStmtNode(expr);
}

void SemanticAnalyzer::visitIfStmtNode(IfStmtNode *ifStmt) {

    if(!ifStmt) return;

    // visit member nodes
    ExprNode* condition = ifStmt->getCondition();
    if (condition) condition->visit(this);
    if (ifStmt->getThen()) ifStmt->getThen()->visit(this);
    if (ifStmt->getElse()) ifStmt->getElse()->visit(this);

    // Check if condition is of boolean type
    if (condition && condition->getType() && condition->getType()->getTypeEnum() != TypeNode::Bool) {
        addError(SemaError(SemaError::InvalidCond, ifStmt->getLocation(), "if statement"));
    }

    ASTVisitorBase::visitIfStmtNode(ifStmt);
}

void SemanticAnalyzer::visitReturnStmtNode(ReturnStmtNode *ret) {

    if(!ret) return;

    // visit member nodes
    if (ret->getReturn()) ret->getReturn()->visit(this);

    FunctionDeclNode* funcNode = ret->getFunction();
    if (!funcNode) return;

    PrimitiveTypeNode* funcReturnType = funcNode->getRetType();
    ExprNode* returnExpr = ret->getReturn();

    // void function should not have a return expression
    if (funcReturnType->getTypeEnum() == TypeNode::Void) {
        if (returnExpr) {
            addError(SemaError(SemaError::MisMatchedReturn, ret->getLocation()));
        }
    } 
    else { // non-void function must have a return expression that matches the return type
        if (!returnExpr) {
            addError(SemaError(SemaError::MisMatchedReturn, ret->getLocation()));
        } 
        // unmatched return type
        else if (returnExpr->getType() && returnExpr->getType()->getTypeEnum() != funcReturnType->getTypeEnum()){
            addError(SemaError(SemaError::MisMatchedReturn, ret->getLocation()));
        }
    }

    ASTVisitorBase::visitReturnStmtNode(ret);
}

void SemanticAnalyzer::visitScopeNode(ScopeNode *scope) {

    if(!scope) return;

    // visit member nodes
    std::vector<DeclNode*> decls = scope->getDeclarations();
    for (DeclNode* decl : decls) decl->visit(this);

    ASTVisitorBase::visitScopeNode(scope);
}

void SemanticAnalyzer::visitWhileStmtNode(WhileStmtNode *whileStmt) {

    if(!whileStmt) return;

    // visit member nodes
    ExprNode* condition = whileStmt->getCondition();
    if (condition) condition->visit(this);
    if (whileStmt->getBody()) whileStmt->getBody()->visit(this);

    // Check if condition is of boolean type
    if (condition && condition->getType() && condition->getType()->getTypeEnum() != TypeNode::Bool) {
        addError(SemaError(SemaError::InvalidCond, whileStmt->getLocation(), "while statement"));
    }

    ASTVisitorBase::visitWhileStmtNode(whileStmt);
}

void SemanticAnalyzer::visitTypeNode(TypeNode *type) {
    ASTVisitorBase::visitTypeNode(type);
}

void SemanticAnalyzer::visitPrimitiveTypeNode(PrimitiveTypeNode *type) {
    ASTVisitorBase::visitPrimitiveTypeNode(type);
}

void SemanticAnalyzer::visitArrayTypeNode(ArrayTypeNode *type) {

    if(!type) return;

    // visit member nodes
    // if (type->getCondition()) type->getCondition()->visit(this);

    ASTVisitorBase::visitArrayTypeNode(type);
}
} // namespace
