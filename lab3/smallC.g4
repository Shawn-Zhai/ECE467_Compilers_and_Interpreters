//
//  smallC.g4
//  ECE467 Lab 3
//
//  Created by Tarek Abdelrahman on 2023-09-13.
//  Based on code written by Ao (Dino) Li for ECE467.
//  Copyright © 2023 Tarek Abdelrahman. All rights reserved.
//
//  Permission is hereby granted to use this code in ECE467 at
//  the University of Toronto. It is prohibited to distribute
//  this code, either publicly or to third parties.

grammar smallC;

@header {
#include "ASTNodes.h"
#include <iostream>
#include <string>
}

program
returns [smallc::ProgramNode *prg]
@init {
    $prg = new smallc::ProgramNode();
    $prg->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
}
: (preamble {$prg->setIo(true);}
| ) (decls
{
   for(unsigned int i = 0; i < $decls.declarations.size();i++) {
        $decls.declarations[i]->setRoot($prg);
        $prg->addChild($decls.declarations[i]);
   }
}
)* EOF ;

preamble:  '#include' '"scio.h"';

decls 
returns [std::vector<smallc::DeclNode*> declarations]
:
    scalarDeclList
    {
        for(unsigned int i = 0; i < $scalarDeclList.scalars.size();i++)
        $declarations.push_back($scalarDeclList.scalars[i]);
    }
    | arrDeclList
    {
        for(unsigned int i = 0; i < $arrDeclList.arrs.size();i++)
        $declarations.push_back($arrDeclList.arrs[i]);
    }
    | fcnProto
    {
        $declarations.push_back($fcnProto.fcnProtoNode);
    }
    | fcnDecl
    {
        $declarations.push_back($fcnDecl.fcnDeclNode);
    }
;

scalarDeclList
returns [std::vector<smallc::ScalarDeclNode*> scalars]
    :
    scalarDecl
    {
        $scalars.push_back($scalarDecl.decl);
    }
    | scalarDecl
    {
        $scalars.push_back($scalarDecl.decl);
    }
    scalarDeclList
    {
        for(unsigned int i = 0; i < $scalarDeclList.scalars.size(); i++)
            $scalars.push_back($scalarDeclList.scalars[i]);
    }
;

scalarDecl 
returns [smallc::ScalarDeclNode *decl]
@init {
    $decl = new smallc::ScalarDeclNode();
    $decl->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
}
:
varType
{
    $decl->setType($varType.varTypeNode);
} 
varName
{
    $decl->setName($varName.varNameNode);
} 
';'
;

arrDeclList
returns [std::vector<smallc::ArrayDeclNode*> arrs]
:
    arrDecl
    {
        $arrs.push_back($arrDecl.decl);
    }
    | arrDecl
    {
        $arrs.push_back($arrDecl.decl);
    }
    arrDeclList
    {
        for(unsigned int i = 0; i < $arrDeclList.arrs.size(); i++)
            $arrs.push_back($arrDeclList.arrs[i]);
    }
;

arrDecl 
returns [smallc::ArrayDeclNode *decl]
locals [smallc::ArrayTypeNode *arrTypeNode]
@init {
    $decl = nullptr;
}
:
    varType arrName '[' intConst ']' ';'
    {
        $arrTypeNode = new smallc::ArrayTypeNode($varType.varTypeNode, std::stoi($intConst.text));
        $decl = new smallc::ArrayDeclNode($arrTypeNode, $arrName.arrNameNode);
        $decl->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
;

fcnProto 
returns [smallc::FunctionDeclNode *fcnProtoNode]
@init {
    $fcnProtoNode = new smallc::FunctionDeclNode();
    $fcnProtoNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
}
:
    retType fcnName '(' params ')' ';'
    {
        $fcnProtoNode->setProto(true);
        $fcnProtoNode->setName($fcnName.fcnNameNode);
        $fcnProtoNode->setRetType($retType.retTypeNode);
        $fcnProtoNode->setParameter($params.pars);
    }
;

fcnDecl 
returns [smallc::FunctionDeclNode *fcnDeclNode]
@init {
    $fcnDeclNode = new smallc::FunctionDeclNode();
    $fcnDeclNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
}
:
    retType fcnName '(' params ')' scope
    {
        $fcnDeclNode->setProto(false);
        $fcnDeclNode->setName($fcnName.fcnNameNode);
        $fcnDeclNode->setRetType($retType.retTypeNode);
        $fcnDeclNode->setParameter($params.pars);
        $fcnDeclNode->setBody($scope.scope_);
    }
;

varType 
returns [smallc::PrimitiveTypeNode *varTypeNode]
@init {
    $varTypeNode = new smallc::PrimitiveTypeNode();
    $varTypeNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
}
:
    'bool'
    {
        $varTypeNode->setType(smallc::TypeNode::Bool);
    }
    | 'int'
    {
        $varTypeNode->setType(smallc::TypeNode::Int);
    }
;

retType 
returns [smallc::PrimitiveTypeNode *retTypeNode]
@init {
    $retTypeNode = new smallc::PrimitiveTypeNode();
    $retTypeNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
}
:
    'void'
    {
        $retTypeNode->setType(smallc::TypeNode::Void);
    }
    | varType
    {
        $retTypeNode = $varType.varTypeNode;
    }
;

constant 
returns [smallc::ConstantExprNode *constantNode]
@init {
    $constantNode = nullptr;
}
:
    boolConst
    {
        $constantNode = $boolConst.boolConstNode;
        $constantNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
    | intConst
    {
        $constantNode = $intConst.intConstNode;
        $constantNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
;

boolConst 
returns [smallc::BoolConstantNode *boolConstNode]
@init {
    $boolConstNode = nullptr;
}
:
    BOOL
    {
        $boolConstNode = new smallc::BoolConstantNode($BOOL.text);
        $boolConstNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
        $boolConstNode->setTypeBool();
    }
;

scope
returns[smallc::ScopeNode* scope_]
@init{
    $scope_ = new smallc::ScopeNode();
    $scope_->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
}
:
    '{' (scalarDecl
    {$scope_->addDeclaration($scalarDecl.decl);}
    |arrDecl
    {$scope_->addDeclaration($arrDecl.decl);}
    )* (stmt
    {$scope_->addChild($stmt.stmtNode);}
    )* '}'
;

stmt 
returns [smallc::StmtNode* stmtNode]
locals [smallc::ExprStmtNode *tempExprStmtNode]
@init {
    $stmtNode = nullptr;
}
:
    expr ';'
    {
        $tempExprStmtNode = new smallc::ExprStmtNode($expr.exprNode);
        $stmtNode = $tempExprStmtNode;
        $stmtNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
    | assignStmt
    {
        $stmtNode = $assignStmt.assignStmtNode;
    }
    | ifStmt
    {
        $stmtNode = $ifStmt.ifStmtNode;
    }
    | whileStmt
    {
        $stmtNode = $whileStmt.whileStmtNode;
    }
    | retStmt
    {
        $stmtNode = $retStmt.retStmtNode;
    }
    | scope
    {
        $stmtNode = $scope.scope_;
    }
;

assignStmt 
returns [smallc::AssignStmtNode* assignStmtNode]
@init {
    $assignStmtNode = nullptr;
}:
    var '=' expr ';'
    {
        $assignStmtNode = new smallc::AssignStmtNode($var.varNode, $expr.exprNode);
        $assignStmtNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
;

ifStmt 
returns [smallc::IfStmtNode* ifStmtNode]
@init {
    $ifStmtNode = nullptr;
}:
    'if' '(' expr ')' stmt
    {
        $ifStmtNode = new smallc::IfStmtNode($expr.exprNode, $stmt.stmtNode);
        $ifStmtNode->setHasElse(false);
        $ifStmtNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());

        $expr.exprNode->setParent($ifStmtNode);
        $stmt.stmtNode->setParent($ifStmtNode);
    }
    | 'if' '(' expr ')' then=stmt 'else' e=stmt
    {
        $ifStmtNode = new smallc::IfStmtNode($expr.exprNode, $then.stmtNode, $e.stmtNode);
        $ifStmtNode->setHasElse(true);
        $ifStmtNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());

        $expr.exprNode->setParent($ifStmtNode);
        $then.stmtNode->setParent($ifStmtNode);
        $e.stmtNode->setParent($ifStmtNode);
    }
;

whileStmt 
returns [smallc::WhileStmtNode* whileStmtNode]
@init {
    $whileStmtNode = nullptr;
}
:
    'while' '(' expr ')' stmt
    {
        $whileStmtNode = new smallc::WhileStmtNode($expr.exprNode, $stmt.stmtNode);
        $whileStmtNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
;

retStmt 
returns [smallc::ReturnStmtNode* retStmtNode]
@init {
    $retStmtNode = nullptr;
}
:
    'return' expr ';'
    {
        $retStmtNode = new smallc::ReturnStmtNode($expr.exprNode);
        $retStmtNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
    |'return' ';'
    {
        $retStmtNode = new smallc::ReturnStmtNode();
        $retStmtNode->returnVoid();
        $retStmtNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
;

expr
returns [smallc::ExprNode* exprNode]
locals [smallc::CallExprNode *tempCallExprNode, smallc::UnaryExprNode *tempUnaryExprNode, smallc::BinaryExprNode *tempBinaryExprNode, smallc::BoolExprNode *tempBoolExprNode]
@init {
    $exprNode = nullptr;
}
:
    intExpr
    {
        $exprNode = $intExpr.intExprNode;
    }
    | '(' expr ')'
    {
        $exprNode = $expr.exprNode;
    }
    | fcnName '(' args ')'
    {
        $tempCallExprNode = new smallc::CallExprNode();
        $tempCallExprNode->setIdent($fcnName.fcnNameNode);
        $tempCallExprNode->setArguments($args.arguments);
        $tempCallExprNode->setTypeVoid();

        $exprNode = $tempCallExprNode;
        $exprNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
    | op=('!' | '-') expr
    {
        $tempUnaryExprNode = new smallc::UnaryExprNode($expr.exprNode);
        $tempUnaryExprNode->setOpcode($op.text);
        if ($op.text == "!") $tempUnaryExprNode->setTypeBool();
        else $tempUnaryExprNode->setTypeInt();

        $tempBoolExprNode = new smallc::BoolExprNode($tempUnaryExprNode);
        $tempBoolExprNode->setTypeBool();

        $exprNode = $tempUnaryExprNode;
        $exprNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
    | l=expr op=('<' | '<=' | '>' | '>=') r=expr
    {
        $tempBinaryExprNode = new smallc::BinaryExprNode($l.exprNode, $r.exprNode);
        $tempBinaryExprNode->setOpcode($op.text);
        $tempBinaryExprNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());

        $tempBoolExprNode = new smallc::BoolExprNode($tempBinaryExprNode);
        $tempBoolExprNode->setTypeBool();
        
        $exprNode = $tempBoolExprNode;
        $exprNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
    | l=expr op=('==' | '!=') r=expr
    {
        $tempBinaryExprNode = new smallc::BinaryExprNode($l.exprNode, $r.exprNode);
        $tempBinaryExprNode->setOpcode($op.text);
        $tempBinaryExprNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());

        $tempBoolExprNode = new smallc::BoolExprNode($tempBinaryExprNode);
        $tempBoolExprNode->setTypeBool();
        
        $exprNode = $tempBoolExprNode;
        $exprNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
    | l=expr op='||' r=expr
    {
        $tempBinaryExprNode = new smallc::BinaryExprNode($l.exprNode, $r.exprNode);
        $tempBinaryExprNode->setOpcode($op.text);
        $tempBinaryExprNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());

        $tempBoolExprNode = new smallc::BoolExprNode($tempBinaryExprNode);
        $tempBoolExprNode->setTypeBool();
        
        $exprNode = $tempBoolExprNode;
        $exprNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
    | l=expr op='&&' r=expr
    {
        $tempBinaryExprNode = new smallc::BinaryExprNode($l.exprNode, $r.exprNode);
        $tempBinaryExprNode->setOpcode($op.text);
        $tempBinaryExprNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());

        $tempBoolExprNode = new smallc::BoolExprNode($tempBinaryExprNode);
        $tempBoolExprNode->setTypeBool();
        
        $exprNode = $tempBoolExprNode;
        $exprNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
;

intExpr 
returns [smallc::IntExprNode* intExprNode]
locals [smallc::BinaryExprNode *tempBinaryExprNode]
@init {
    $intExprNode = nullptr;
}
:
    var
    {
        $intExprNode = new smallc::IntExprNode($var.varNode);
        $intExprNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
    | constant
    {
        $intExprNode = new smallc::IntExprNode($constant.constantNode);
        $intExprNode->setType($constant.constantNode->getType());
        $intExprNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
    | l=intExpr op=('*' | '/' ) r=intExpr
    {
        $tempBinaryExprNode = new smallc::BinaryExprNode($l.intExprNode, $r.intExprNode);
        $tempBinaryExprNode->setOpcode($op.text);
        $tempBinaryExprNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());

        $intExprNode = new smallc::IntExprNode($tempBinaryExprNode);
        $intExprNode->setTypeInt();
        $intExprNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
    | l=intExpr op=('+' | '-' ) r=intExpr
    {
        $tempBinaryExprNode = new smallc::BinaryExprNode($l.intExprNode, $r.intExprNode);
        $tempBinaryExprNode->setOpcode($op.text);
        $tempBinaryExprNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());

        $intExprNode = new smallc::IntExprNode($tempBinaryExprNode);
        $intExprNode->setTypeInt();
        $intExprNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
    | '(' intExpr ')'
    {
        $intExprNode = $intExpr.intExprNode;
    }
;

var 
returns [smallc::ReferenceExprNode* varNode]
@init {
    $varNode = new smallc::ReferenceExprNode();
    $varNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
}
:
    varName
    {
        $varNode->setIdent($varName.varNameNode);
    }
    | arrName '[' intExpr ']'
    {
        $varNode->setIdent($arrName.arrNameNode);
        $varNode->setIndex($intExpr.intExprNode);
    }
;

params 
returns [std::vector<smallc::ParameterNode*> pars]
@init {
    $pars = {};
}
:
    paramList
    {
        $pars = $paramList.pars;
    }
    | ;

paramEntry 
returns [smallc::ParameterNode *paramEntryNode]
locals [smallc::ArrayTypeNode *tempArrayTypeNode]
@init {
    $paramEntryNode = nullptr;
}
:
    varType varName
    {
        $paramEntryNode = new smallc::ParameterNode($varType.varTypeNode, $varName.varNameNode);
        $paramEntryNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
    | varType arrName '[]'
    {
        $tempArrayTypeNode = new smallc::ArrayTypeNode($varType.varTypeNode);
        $paramEntryNode = new smallc::ParameterNode($tempArrayTypeNode, $arrName.arrNameNode);
        $paramEntryNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
;

paramList 
returns [std::vector<smallc::ParameterNode*> pars]
:
    paramEntry
    {
        $pars.push_back($paramEntry.paramEntryNode);
    }
    | paramEntry  ',' paramList
    {
        $pars.push_back($paramEntry.paramEntryNode);
        for(unsigned int i = 0; i < $paramList.pars.size(); i++)
            $pars.push_back($paramList.pars[i]);
    }
;

args 
returns [std::vector<smallc::ArgumentNode*> arguments]
@init {
    $arguments = {};
}
:
    argList
    {
        $arguments = $argList.arguments;
    }
    |
;

argEntry 
returns [smallc::ArgumentNode *argEntryNode]
@init {
    $argEntryNode = nullptr;
}
:
    expr
    {
        $argEntryNode = new smallc::ArgumentNode($expr.exprNode);
        $argEntryNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
;

argList 
returns [std::vector<smallc::ArgumentNode*> arguments]
:
    argEntry
    {
        $arguments.push_back($argEntry.argEntryNode);
    }
    | argEntry ',' argList
    {
        $arguments.push_back($argEntry.argEntryNode);
        for(unsigned int i = 0; i < $argList.arguments.size(); i++)
            $arguments.push_back($argList.arguments[i]);
    }
;

varName
returns [smallc::IdentifierNode *varNameNode]
@init {
    $varNameNode = nullptr;
}
: 
    ID
    {
        $varNameNode = new smallc::IdentifierNode($ID.text);
        $varNameNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
;

arrName 
returns [smallc::IdentifierNode *arrNameNode]
@init {
    $arrNameNode = nullptr;
}
: 
    ID
    {
        $arrNameNode = new smallc::IdentifierNode($ID.text);
        $arrNameNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
;

fcnName 
returns [smallc::IdentifierNode *fcnNameNode]
@init {
    $fcnNameNode = nullptr;
}
: 
    ID
    {
        $fcnNameNode = new smallc::IdentifierNode($ID.text);
        $fcnNameNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
    }
;

intConst 
returns [smallc::IntConstantNode *intConstNode]
@init {
    $intConstNode = nullptr;
}
: 
    INT
    {
        $intConstNode = new smallc::IntConstantNode($INT.text);
        $intConstNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
        $intConstNode->setTypeInt();
    }
| '-' INT
    {
        $intConstNode = new smallc::IntConstantNode("-"+$INT.text);
        $intConstNode->setLocation($ctx->start->getLine(), $ctx->start->getCharPositionInLine());
        $intConstNode->setTypeInt();
    }
;

BOOL: 'true' | 'false';
ID: [a-zA-Z][a-zA-Z0-9_]* ;
INT:    [0] | ([1-9][0-9]*);
WS:     [ \t\r\n]+ -> skip;
COMMENT: '//' (~[\r\n])* -> skip;
