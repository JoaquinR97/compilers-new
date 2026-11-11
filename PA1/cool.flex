/*
 *  The scanner definition for COOL.
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>
#include <string>

#define yylval cool_yylval
#define yylex  cool_yylex

#define MAX_STR_CONST 1025
#define YY_NO_UNPUT

extern FILE *fin;

#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
    if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
        YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST];
char *string_buf_ptr;
static int comment_layer = 0;

extern int curr_lineno;
extern int verbose_flag;
extern YYSTYPE cool_yylval;

// This is a simple helper function to check if the string is full
bool str_buf_full() {
    return (string_buf_ptr - string_buf) >= (MAX_STR_CONST - 1);
}
%}

/*
 *  Add Your own definitions here
 */
DARROW          =>
ASSIGN          <-
LE              <=
DIGIT           [0-9]
ALPHA           [a-zA-Z_]
ALPHANUM        [a-zA-Z0-9_]
WHITESPACE      [ \t\r\f\v]

/* Start conditions (kindaof like "modes" the scanner can be in) */
%Start COMMENT
%Start STRING
%Start STR_ERR

%%

 /*  Always track newlines so curr_lineno stays correct.*/

\n              { curr_lineno++; }
{WHITESPACE}+   { /* ignore horizontal whitespace */ }


 /* Single-line comments  */

"--"[^\n]*      { /* consume and ignore */ }


 /* Block comments  (* ... *) which can be nested */

"(*"            {
                    comment_layer++;
                    BEGIN(COMMENT);
                }

<COMMENT>"(*"   { comment_layer++; }   /* nested open  */
<COMMENT>"*)"   {
                    comment_layer--;
                    if (comment_layer == 0)
                        BEGIN(INITIAL);   /* back to normal after incrementing all parantheses */
                }
<COMMENT>\n     { curr_lineno++; }
<COMMENT>.      { /* consume any other char inside comment */ }
<COMMENT><<EOF>> {
                    BEGIN(INITIAL);
                    cool_yylval.error_msg = "EOF in comment";
                    return ERROR;
                }

 /* Unmatched *) outside any comment */
"*)"            {
                    cool_yylval.error_msg = "Unmatched *)";
                    return ERROR;
                }


 /* String constants   " ... " */

\"              {
                    BEGIN(STRING);
                    string_buf_ptr = string_buf;  /* reset buffer */
                }

<STRING>\"      {
                    BEGIN(INITIAL);
                    *string_buf_ptr = '\0';      
                    cool_yylval.symbol =
                        stringtable.add_string(string_buf);
                    return STR_CONST;
                }

<STRING>\n      {
                    curr_lineno++;
                    BEGIN(INITIAL);
                    cool_yylval.error_msg = "Unterminated string constant";
                    return ERROR;
                }

<STRING><<EOF>> {
                    BEGIN(INITIAL);
                    cool_yylval.error_msg = "EOF in string constant";
                    return ERROR;
                }

<STRING>\0      {
                    BEGIN(STR_ERR);   /* null skip to end of this string */
                    cool_yylval.error_msg = "String contains null character";
                    return ERROR;
                }

 /* Escape sequences inside strings */
<STRING>\\n     {
                    if (str_buf_full()) {
                        BEGIN(STR_ERR);
                        cool_yylval.error_msg = "String constant too long";
                        return ERROR;
                    }
                    *string_buf_ptr++ = '\n';
                }
<STRING>\\t     {
                    if (str_buf_full()) {
                        BEGIN(STR_ERR);
                        cool_yylval.error_msg = "String constant too long";
                        return ERROR;
                    }
                    *string_buf_ptr++ = '\t';
                }
<STRING>\\b     {
                    if (str_buf_full()) {
                        BEGIN(STR_ERR);
                        cool_yylval.error_msg = "String constant too long";
                        return ERROR;
                    }
                    *string_buf_ptr++ = '\b';
                }
<STRING>\\f     {
                    if (str_buf_full()) {
                        BEGIN(STR_ERR);
                        cool_yylval.error_msg = "String constant too long";
                        return ERROR;
                    }
                    *string_buf_ptr++ = '\f';
                }
<STRING>\\\n    {
                    curr_lineno++;
                    if (str_buf_full()) {
                        BEGIN(STR_ERR);
                        cool_yylval.error_msg = "String constant too long";
                        return ERROR;
                    }
                    *string_buf_ptr++ = '\n';
                }
<STRING>\\.     {
                    if (str_buf_full()) {
                        BEGIN(STR_ERR);
                        cool_yylval.error_msg = "String constant too long";
                        return ERROR;
                    }
                    *string_buf_ptr++ = yytext[1];
                }
<STRING>.       {
                    if (str_buf_full()) {
                        BEGIN(STR_ERR);
                        cool_yylval.error_msg = "String constant too long";
                        return ERROR;
                    }
                    *string_buf_ptr++ = yytext[0];
                }

 /* STR_ERR: we already returned an error; now silently eat until end of string */
<STR_ERR>\"     { BEGIN(INITIAL); }
<STR_ERR>\\\n   { curr_lineno++; }
<STR_ERR>\n     { curr_lineno++; BEGIN(INITIAL); }
<STR_ERR><<EOF>>{ BEGIN(INITIAL); }
<STR_ERR>.      { /* consume */ }

 /* Keywords + Integers */
(?i:class)      { return CLASS; }
(?i:else)       { return ELSE; }
(?i:fi)         { return FI; }
(?i:if)         { return IF; }
(?i:in)         { return IN; }
(?i:inherits)   { return INHERITS; }
(?i:isvoid)     { return ISVOID; }
(?i:let)        { return LET; }
(?i:loop)       { return LOOP; }
(?i:pool)       { return POOL; }
(?i:then)       { return THEN; }
(?i:while)      { return WHILE; }
(?i:case)       { return CASE; }
(?i:esac)       { return ESAC; }
(?i:new)        { return NEW; }
(?i:of)         { return OF; }
(?i:not)        { return NOT; }

 /* Bool constants: true/false must START with lowercase */
t(?i:rue)       {
                    cool_yylval.boolean = 1;
                    return BOOL_CONST;
                }
f(?i:alse)      {
                    cool_yylval.boolean = 0;
                    return BOOL_CONST;
                }

{DIGIT}+        {
                    cool_yylval.symbol = inttable.add_string(yytext);
                    return INT_CONST;
                }


 /* Identifiers Type IDs start with uppercase; object IDs start with lowercase. */
[A-Z]{ALPHANUM}*  {
                    cool_yylval.symbol = idtable.add_string(yytext);
                    return TYPEID;
                  }

[a-z]{ALPHANUM}*  {
                    cool_yylval.symbol = idtable.add_string(yytext);
                    return OBJECTID;
                  }


 /* Operators and single-character tokens */

{DARROW}        { return DARROW; }
{ASSIGN}        { return ASSIGN; }
{LE}            { return LE; }

"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
"<"             { return '<'; }
"="             { return '='; }
"~"             { return '~'; }
"{"             { return '{'; }
"}"             { return '}'; }
"("             { return '('; }
")"             { return ')'; }
";"             { return ';'; }
":"             { return ':'; }
","             { return ','; }
"."             { return '.'; }
"@"             { return '@'; }


 /* Catch-all invalid characters */

.               {
                    cool_yylval.error_msg = yytext;
                    return ERROR;
                }

%%