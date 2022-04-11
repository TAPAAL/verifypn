%{
#include <stdio.h>
#include <memory>

#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/Expressions.h"

using namespace PetriEngine::PQL;

std::shared_ptr<PetriEngine::PQL::Condition> query;
extern int pqlqlex();
void pqlqerror(const char *s) {printf("ERROR: %s\n", s);}
%}

%name-prefix "pqlq"
%expect 2 // and + or, which is ok.

/* Possible data representation */
%union {
   PetriEngine::PQL::Expr* expr;
   PetriEngine::PQL::Condition* cond;
   std::string *string;
   int token;
   std::vector<std::string>* ids;
}

/* Terminal type definition */
%token <string> ID INT

%token <token> A E X F G U EF EG AF AG EX AX CONTROL
%token <token> DEADLOCK TRUE FALSE
%token <token> LPAREN RPAREN
%token <token> AND OR NOT
%token <token> EQUAL NEQUAL LESS LESSEQUAL GREATER GREATEREQUAL
%token <token> PLUS MINUS MULTIPLY
%token <token> COMMA COLON
%token <token> TOKENCOUNT QUESTIONMARK FIREABLE

/* Terminal associativity */
%left AND OR  EF EG AF AG EX AX F G A E
%right NOT

/* Nonterminal type definition */
%type <expr> expr term factor
%type <cond> path_formula state_formula_or
%type <cond> state_formula state_formula_and state_formula_quantifier atomic_formula compare query
%type <ids> id_list

/* Operator precedence, more possibly coming */
%right MINUS

%start query

%%

query : CONTROL COLON state_formula { query = Condition_ptr(new ControlCondition(Condition_ptr($3))); }
        | state_formula { query = Condition_ptr($1); }
        ;

state_formula : LPAREN state_formula RPAREN	{ $$ = $2; }
              | state_formula_or            { $$ = $1; }

state_formula_or : state_formula OR state_formula	   { $$ = new OrCondition(Condition_ptr($1), Condition_ptr($3)); }
                 | state_formula_and                   { $$ = $1; }
                 ;

state_formula_and : state_formula AND state_formula { $$ = new AndCondition(Condition_ptr($1), Condition_ptr($3)); }
                  | state_formula_quantifier        { $$ = $1; }
                  ;


state_formula_quantifier : A state_formula   { $$ = new ACondition(Condition_ptr($2)); }
                         | E state_formula   { $$ = new ECondition(Condition_ptr($2)); }
                         | EF state_formula  { $$ = new ECondition(std::make_shared<FCondition>(Condition_ptr($2))); }
                         | EG state_formula  { $$ = new ECondition(std::make_shared<GCondition>(Condition_ptr($2))); }
                         | AF state_formula  { $$ = new ACondition(std::make_shared<FCondition>(Condition_ptr($2))); }
                         | AG state_formula  { $$ = new ACondition(std::make_shared<GCondition>(Condition_ptr($2))); }
                         | EX state_formula  { $$ = new ECondition(std::make_shared<XCondition>(Condition_ptr($2))); }
                         | AX state_formula  { $$ = new ACondition(std::make_shared<XCondition>(Condition_ptr($2))); }
                         | NOT state_formula { $$ = new NotCondition(Condition_ptr($2)); }
                         | atomic_formula    { $$ = $1; }
                         | path_formula
                         ;

atomic_formula : TRUE				            	{ $$ = new BooleanCondition(true);}
               | FALSE			            	    { $$ = new BooleanCondition(false);}
               | DEADLOCK		            	    { $$ = new DeadlockCondition();}
               | compare				            { $$ = $1; }
               ;

path_formula : X state_formula                    { $$ = new XCondition(Condition_ptr($2)); }
             | F state_formula                    { $$ = new FCondition(Condition_ptr($2)); }
             | G state_formula                    { $$ = new GCondition(Condition_ptr($2)); }
             | LPAREN state_formula RPAREN U LPAREN state_formula RPAREN
               { $$ = new UntilCondition(Condition_ptr($2), Condition_ptr($6)); };
             ;

compare	: expr EQUAL expr			{ $$ = new EqualCondition(Expr_ptr($1), Expr_ptr($3)); }
		| expr NEQUAL expr			{ $$ = new NotEqualCondition(Expr_ptr($1), Expr_ptr($3)); }
		| expr LESS expr			{ $$ = new LessThanCondition(Expr_ptr($1), Expr_ptr($3)); }
		| expr LESSEQUAL expr 		{ $$ = new LessThanOrEqualCondition(Expr_ptr($1), Expr_ptr($3)); }
		| expr GREATER expr			{ $$ = new LessThanCondition(Expr_ptr($3), Expr_ptr($1)); }
		| expr GREATEREQUAL expr	{ $$ = new LessThanOrEqualCondition(Expr_ptr($3), Expr_ptr($1)); }
		| FIREABLE LPAREN id_list RPAREN
		    {
		        $$ = nullptr;
		        {
		            auto ids = $3;
		            if ((*ids).size() == 1) {
		                $$ = new FireableCondition(std::make_shared<const_string>((*ids)[0]));
		            } else {
		                std::vector<Condition_ptr> a;
                        for (auto& name : *ids) {
                            a.push_back(std::make_shared<FireableCondition>(std::make_shared<const_string>(name)));
                        }
                        $$ = new OrCondition(a);
		            }
		        }
		    }
		| ID QUESTIONMARK           { $$ = new FireableCondition(std::make_shared<const_string>(*$1)); }
		;

expr	: expr PLUS term			{ $$ = new PlusExpr(std::vector<Expr_ptr>({Expr_ptr($1), Expr_ptr($3)})); }
		| expr MINUS term			{ $$ = new SubtractExpr(std::vector<Expr_ptr>({Expr_ptr($1), Expr_ptr($3)})); }
		| term						{ $$ = $1; }
		;

term	: term MULTIPLY factor	{ $$ = new MultiplyExpr(std::vector<Expr_ptr>({Expr_ptr($1), Expr_ptr($3)})); }
		| MINUS factor			{ $$ = new MinusExpr(Expr_ptr($2)); }
		| factor				{ $$ = $1; }
		;

factor	: LPAREN expr RPAREN	{ $$ = $2; }
		| INT			{ $$ = new LiteralExpr(atol($1->c_str())); delete $1; }
		| ID			{ $$ = new IdentifierExpr(std::make_shared<const_string>(*$1)); delete $1; }
        | TOKENCOUNT LPAREN id_list RPAREN
            {
                $$ = nullptr;
                {
                    auto ids = $3;
                    std::vector<Expr_ptr> a;
                    for (auto& name : *ids) {
                        a.push_back(std::make_shared<IdentifierExpr>(std::make_shared<const_string>(name)));
                    }
                    $$ = new PlusExpr(std::move(a));
                }
            }
		;

id_list : ID { $$ = new std::vector<std::string>(); $$->push_back(*$1); }
        | id_list COMMA ID { $$ = $1; $$->push_back(*$3); }
        ;