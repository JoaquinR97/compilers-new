/*
 *  cool.y
 *  Parser definition for the COOL language.
 *  Original implementation - PA3
 */

%{
  #include <iostream>
  #include "cool-tree.h"
  #include "stringtab.h"
  #include "utilities.h"

  extern char *curr_filename;

  /* Locations */
  #define YYLTYPE int              /* the type of locations */
  #define cool_yylloc curr_lineno  /* use the curr_lineno from the lexer
  for the location of tokens */

    extern int node_lineno;          /* set before constructing a tree node
    to whatever you want the line number
    for the tree node to be */

  #define YYLLOC_DEFAULT(Current, Rhs, N)  \
      Current = Rhs[1];                    \
      node_lineno = Current;

  #define SET_NODELOC(Current) \
      node_lineno = Current;

    /* IMPORTANT NOTE ON LINE NUMBERS
    *********************************
    * The above definitions and macros cause every terminal in your grammar to
    * have the line number supplied by the lexer. The only task you have to
    * implement for line numbers to work correctly, is to use SET_NODELOC()
    * before constructing any constructs from non-terminals in your grammar.
    * Example: Consider you are matching on the following very restrictive
    * (fictional) construct that matches a plus between two integer constants.
    * (SUCH A RULE SHOULD NOT BE  PART OF YOUR PARSER):

    plus_consts	: INT_CONST '+' INT_CONST

    * where INT_CONST is a terminal for an integer constant. Now, a correct
    * action for this rule that attaches the correct line number to plus_const
    * would look like the following:

    plus_consts	: INT_CONST '+' INT_CONST
    {
      // Set the line number of the current non-terminal:
      // ***********************************************
      // You can access the line numbers of the i'th item with @i, just
      // like you acess the value of the i'th exporession with $i.
      //
      // Here, we choose the line number of the last INT_CONST (@3) as the
      // line number of the resulting expression (@$). You are free to pick
      // any reasonable line as the line number of non-terminals. If you
      // omit the statement @$=..., bison has default rules for deciding which
      // line number to use. Check the manual for details if you are interested.
      @$ = @3;


      // Observe that we call SET_NODELOC(@3); this will set the global variable
      // node_lineno to @3. Since the constructor call "plus" uses the value of
      // this global, the plus node will now have the correct line number.
      SET_NODELOC(@3);

      // construct the result node:
      $$ = plus(int_const($1), int_const($3));
    }

    */


  void yyerror(char *s);
  extern int yylex();

  /************************************************************************/
  /*                DONT CHANGE ANYTHING IN THIS SECTION                  */

  Program ast_root;	      /* the result of the parse  */
  Classes parse_results;        /* for use in semantic analysis */
  int omerrs = 0;               /* number of errors in lexing and parsing */
%}

/* Union of all possible semantic value types */
%union {
  Boolean      boolean;
  Symbol       symbol;
  Program      program;
  Class_       class_;
  Classes      classes;
  Feature      feature;
  Features     features;
  Formal       formal;
  Formals      formals;
  Case         case_;
  Cases        cases;
  Expression   expression;
  Expressions  expressions;
  char        *error_msg;
}

/*
Declare the terminals; a few have types for associated lexemes.
The token ERROR is never used in the parser; thus, it is a parse
error when the lexer returns it.

The integer following token declaration is the numeric constant used
to represent that token internally.  Typically, Bison generates these
on its own, but we give explicit numbers to prevent version parity
problems (bison 1.25 and earlier start at 258, later versions -- at
257)
*/

%token CLASS 258 ELSE 259 FI 260 IF 261 IN 262
%token INHERITS 263 LET 264 LOOP 265 POOL 266 THEN 267 WHILE 268
%token CASE 269 ESAC 270 OF 271 DARROW 272 NEW 273 ISVOID 274
%token <symbol>  STR_CONST 275 INT_CONST 276
%token <boolean> BOOL_CONST 277
%token <symbol>  TYPEID 278 OBJECTID 279
%token ASSIGN 280 NOT 281 LE 282 ERROR 283

/*  DON'T CHANGE ANYTHING ABOVE THIS LINE, OR YOUR PARSER WONT WORK       */
/**************************************************************************/

/* Non-terminal type declarations */
%type <program>     program
%type <classes>     class_list
%type <class_>      classdef

%type <features>    feature_list
%type <feature>     feature

%type <formals>     param_list
%type <formal>      param

%type <expression>  expr
%type <expression>  expr_or_empty
%type <expressions> arg_list
%type <expressions> block_body

%type <expression>  let_tail
%type <cases>       case_branches
%type <case_>       case_branch

/*
 * Operator precedence (low to high).
 * Follows the COOL Reference Manual section 11.1.
 */
%right    ASSIGN
%left     NOT
%nonassoc '<' '=' LE
%left     '+' '-'
%left     '*' '/'
%left     ISVOID
%left     '~'
%left     '@'
%left     '.'

%%

/* Top-level program */
program
    : class_list
        {
            @$ = @1;
            ast_root = program($1);
        }
    ;

class_list
    : classdef
        { $$ = single_Classes($1); }
    | class_list classdef
        { $$ = append_Classes($1, single_Classes($2)); }
    ;

/*
 * Class definition.
 * Optional inheritance; defaults to Object when omitted.
 * Error recovery: skip to the next ';' at class level.
 */
classdef
    : CLASS TYPEID '{' feature_list '}' ';'
        {
            $$ = class_($2,
                        idtable.add_string("Object"),
                        $4,
                        stringtable.add_string(curr_filename));
        }
    | CLASS TYPEID INHERITS TYPEID '{' feature_list '}' ';'
        {
            $$ = class_($2,
                        $4,
                        $6,
                        stringtable.add_string(curr_filename));
        }
    | error ';'
        { }
    ;

/* Entere feature list */
feature_list
    : /* empty */
        { $$ = nil_Features(); }
    | feature_list feature
        { $$ = append_Features($1, single_Features($2)); }
    ;

feature
    : OBJECTID '(' param_list ')' ':' TYPEID '{' expr '}' ';'
        {
            $$ = method($1, $3, $6, $8);
        }
    | OBJECTID ':' TYPEID ';'
        {
            $$ = attr($1, $3, no_expr());
        }
    | OBJECTID ':' TYPEID ASSIGN expr ';'
        {
            $$ = attr($1, $3, $5);
        }
    | error ';'
        { /* recover at the feature boundary */ }
    ;

param_list
    : /* empty */
        { $$ = nil_Formals(); }
    | param
        { $$ = single_Formals($1); }
    | param_list ',' param
        { $$ = append_Formals($1, single_Formals($3)); }
    ;

param
    : OBJECTID ':' TYPEID
        { $$ = formal($1, $3); }
    ;

/* Expressions */
expr_or_empty
    : expr      { $$ = $1; }
    | /* empty */ { $$ = no_expr(); }
    ;

arg_list
    : /* empty */
        { $$ = nil_Expressions(); }
    | expr
        { $$ = single_Expressions($1); }
    | arg_list ',' expr
        { $$ = append_Expressions($1, single_Expressions($3)); }
    ;

block_body
    : expr ';'
        { $$ = single_Expressions($1); }
    | block_body expr ';'
        { $$ = append_Expressions($1, single_Expressions($2)); }
    | error ';'
        { /* recover inside a block */ }
    ;

let_tail
    : OBJECTID ':' TYPEID IN expr
        {
            $$ = let($1, $3, no_expr(), $5);
        }
    | OBJECTID ':' TYPEID ASSIGN expr IN expr
        {
            $$ = let($1, $3, $5, $7);
        }
    | OBJECTID ':' TYPEID ',' let_tail
        {
            $$ = let($1, $3, no_expr(), $5);
        }
    | OBJECTID ':' TYPEID ASSIGN expr ',' let_tail
        {
            $$ = let($1, $3, $5, $7);
        }
    | error ',' let_tail
        {
            /* recover inside a let binding; yyerrok resumes error reporting */
            yyerrok;
            $$ = $3;
        }
    | error IN expr
        {
            yyerrok;
            $$ = $3;
        }
    ;

case_branches
    : case_branch
        { $$ = single_Cases($1); }
    | case_branches case_branch
        { $$ = append_Cases($1, single_Cases($2)); }
    ;

case_branch
    : OBJECTID ':' TYPEID DARROW expr ';'
        { $$ = branch($1, $3, $5); }
    ;

expr
    : OBJECTID ASSIGN expr
        { $$ = assign($1, $3); }

    | expr '@' TYPEID '.' OBJECTID '(' arg_list ')'
        { $$ = static_dispatch($1, $3, $5, $7); }

    | expr '.' OBJECTID '(' arg_list ')'
        { $$ = dispatch($1, $3, $5); }

    | OBJECTID '(' arg_list ')'
        { $$ = dispatch(object(idtable.add_string("self")), $1, $3); }

    | IF expr THEN expr ELSE expr FI
        { $$ = cond($2, $4, $6); }

    | WHILE expr LOOP expr_or_empty POOL
        { $$ = loop($2, $4); }

    | '{' block_body '}'
        { $$ = block($2); }

    | LET let_tail
        { $$ = $2; }

    | CASE expr OF case_branches ESAC
        { $$ = typcase($2, $4); }

    | NEW TYPEID
        { $$ = new_($2); }

    | ISVOID expr
        { $$ = isvoid($2); }

    | expr '+' expr   { $$ = plus($1, $3); }
    | expr '-' expr   { $$ = sub($1, $3); }
    | expr '*' expr   { $$ = mul($1, $3); }
    | expr '/' expr   { $$ = divide($1, $3); }
    | '~' expr        { $$ = neg($2); }

    | expr '<'  expr  { $$ = lt($1, $3); }
    | expr LE   expr  { $$ = leq($1, $3); }
    | expr '='  expr  { $$ = eq($1, $3); }

    | NOT expr        { $$ = comp($2); }

    | '(' expr ')'    { $$ = $2; }

    | OBJECTID        { $$ = object($1); }
    | INT_CONST       { $$ = int_const($1); }
    | STR_CONST       { $$ = string_const($1); }
    | BOOL_CONST      { $$ = bool_const($1); }
    ;

%%

/* Bison syntax error */
void yyerror(char *s)
{
    extern int curr_lineno;

    cerr << "\"" << curr_filename << "\", line " << curr_lineno << ": "
         << s << " at or near ";
    print_cool_token(yychar);
    cerr << endl;
    omerrs++;

    if (omerrs > 50) {
        fprintf(stdout, "More than 50 errors\n");
        exit(1);
    }
}
