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
%expect 17

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
%token <token> A E X F G U
%token <token> DEADLOCK TRUE FALSE
%token <token> LPAREN RPAREN
%token <token> AND OR NOT
%token <token> EQUAL NEQUAL LESS LESSEQUAL GREATER GREATEREQUAL
%token <token> PLUS MINUS MULTIPLY
%token <token> COMMA
%token <token> TOKENCOUNT QUESTIONMARK FIREABLE

/* Terminal associativity */
%left AND OR
%right NOT

/* Nonterminal type definition */
%type <expr> expr term factor
%type <cond> state_formula path_formula path_formula_not_state path_formula_not_state_lower compare
%type <ids> id_list

/* Operator precedence, more possibly coming */
%right MINUS

%start query

%%

query	: state_formula	            { query = Condition_ptr($1); }
        | path_formula_not_state    { query = Condition_ptr($1); } /* This one is not actually allowed in CTL* */
		| error						{ yyerrok; }
		;

state_formula : state_formula AND state_formula	    { $$ = new AndCondition(Condition_ptr($1), Condition_ptr($3)); }
              | state_formula OR state_formula		{ $$ = new OrCondition(Condition_ptr($1), Condition_ptr($3)); }
              | NOT state_formula				    { $$ = new NotCondition(Condition_ptr($2)); }
              | LPAREN state_formula RPAREN		    { $$ = $2; }
              | TRUE				            	{ $$ = new BooleanCondition(true);}
              | FALSE			            		{ $$ = new BooleanCondition(false);}
              | A path_formula        { $$ = new ACondition(Condition_ptr($2)); }
              | E path_formula        { $$ = new ECondition(Condition_ptr($2)); }
              | compare				            { $$ = $1; }
              | DEADLOCK		            	    { $$ = new DeadlockCondition();}
              ;


path_formula : state_formula                    { $$ = $1; }
             | path_formula_not_state           { $$ = $1; }


path_formula_not_state :
               path_formula AND path_formula	{ $$ = new AndCondition(Condition_ptr($1), Condition_ptr($3)); }
             | path_formula OR path_formula    { $$ = new OrCondition(Condition_ptr($1), Condition_ptr($3)); }
             | X path_formula                    { $$ = new XCondition(Condition_ptr($2)); }
             | F path_formula                    { $$ = new FCondition(Condition_ptr($2)); }
             | G path_formula                    { $$ = new GCondition(Condition_ptr($2)); }
             | LPAREN path_formula U path_formula RPAREN      { $$ = new UntilCondition(Condition_ptr($2), Condition_ptr($4)); }
             | path_formula_not_state_lower    { $$ = $1; }
             ;

path_formula_not_state_lower :
               NOT path_formula_not_state_lower		{ $$ = new NotCondition(Condition_ptr($2)); }
             | LPAREN path_formula RPAREN       { $$ = $2; }
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
		            std::vector<Condition_ptr> a;
                    for (auto& name : *ids) {
                        a.push_back(std::make_shared<FireableCondition>(name));
                    }
		            $$ = new AndCondition(a);
		        }
		    }
		| ID QUESTIONMARK           { $$ = new FireableCondition(*$1); }
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
		| ID			{ $$ = new IdentifierExpr(*$1); delete $1; }
        | TOKENCOUNT LPAREN id_list RPAREN
            {
                $$ = nullptr;
                {
                    auto ids = $3;
                    std::vector<Expr_ptr> a;
                    for (auto& name : *ids) {
                        a.push_back(std::make_shared<IdentifierExpr>(name));
                    }
                    $$ = new PlusExpr(std::move(a), true);
                }
            }
		;

id_list : ID { $$ = new std::vector<std::string>(); $$->push_back(*$1); }
        | id_list COMMA ID { $$ = $1; $$->push_back(*$3); }
        ;