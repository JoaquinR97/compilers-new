#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <set>
#include <map>
#include <list>
#include <vector>
#include <queue>
#include "semant.h"
#include "utilities.h"

extern int   semant_debug;
extern char *curr_filename;

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////

static Symbol
    S_arg, S_arg2,
    S_Bool, S_concat, S_abort, S_copy,
    S_Int, S_in_int, S_in_string,
    S_IO, S_length,
    S_Main, S_main,
    S_No_class, S_No_type,
    S_Object,
    S_out_int, S_out_string,
    S_prim_slot,
    S_self, S_SELF_TYPE,
    S_String, S_str_field,
    S_substr, S_type_name, S_val;
//
// Initializing the predefined symbols.
//
static void initialize_constants() {
    S_arg        = idtable.add_string("arg");
    S_arg2       = idtable.add_string("arg2");
    S_Bool       = idtable.add_string("Bool");
    S_concat     = idtable.add_string("concat");
    S_abort      = idtable.add_string("abort");
    S_copy       = idtable.add_string("copy");
    S_Int        = idtable.add_string("Int");
    S_in_int     = idtable.add_string("in_int");
    S_in_string  = idtable.add_string("in_string");
    S_IO         = idtable.add_string("IO");
    S_length     = idtable.add_string("length");
    S_Main       = idtable.add_string("Main");
    S_main       = idtable.add_string("main");
    //   _no_class is a symbol that can't be the name of any 
    //   user-defined class.
    S_No_class   = idtable.add_string("_no_class");
    S_No_type    = idtable.add_string("_no_type");
    S_Object     = idtable.add_string("Object");
    S_out_int    = idtable.add_string("out_int");
    S_out_string = idtable.add_string("out_string");
    S_prim_slot  = idtable.add_string("_prim_slot");
    S_self       = idtable.add_string("self");
    S_SELF_TYPE  = idtable.add_string("SELF_TYPE");
    S_String     = idtable.add_string("String");
    S_str_field  = idtable.add_string("_str_field");
    S_substr     = idtable.add_string("substr");
    S_type_name  = idtable.add_string("type_name");
    S_val        = idtable.add_string("_val");
}

static Symbol                       g_cls;
static ClassTable                  *g_ct;
static SymbolTable<Symbol, Symbol> *g_env;

ClassTable::ClassTable(Classes )
    : semant_errors(0), error_stream(cerr)
{
    install_basic_classes();
}

void ClassTable::install_basic_classes() {
    
    // The tree package uses these globals to annotate the classes built below.
    // curr_lineno  = 0;
    Symbol fn = stringtable.add_string("<basic class>");

    // The following demonstrates how to create dummy parse trees to
    // refer to basic Cool classes.  There's no need for method
    // bodies -- these are already built into the runtime system.
    
    // IMPORTANT: The results of the following expressions are
    // stored in local variables.  You will want to do something
    // with those variables at the end of this method to make this
    // code meaningful.

    // 
    // The Object class has no parent class. Its methods are
    //        abort() : Object    aborts the program
    //        type_name() : Str   returns a string representation of class name
    //        copy() : SELF_TYPE  returns a copy of the object
    //
    // There is no need for method bodies in the basic classes---these
    // are already built in to the runtime system.

    Class_ C_Object = class_(S_Object, S_No_class,
        append_Features(
            append_Features(
                single_Features(method(S_abort,     nil_Formals(), S_Object,    no_expr())),
                single_Features(method(S_type_name, nil_Formals(), S_String,    no_expr()))
            ),
            single_Features(method(S_copy, nil_Formals(), S_SELF_TYPE, no_expr()))
        ), fn);

    // 
    // The IO class inherits from Object. Its methods are
    //        out_string(Str) : SELF_TYPE       writes a string to the output
    //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
    //        in_string() : Str                 reads a string from the input
    //        in_int() : Int                      "   an int     "  "     "
    //
    Class_ C_IO = class_(S_IO, S_Object,
        append_Features(
            append_Features(
                append_Features(
                    single_Features(method(S_out_string,
                        single_Formals(formal(S_arg, S_String)), S_SELF_TYPE, no_expr())),
                    single_Features(method(S_out_int,
                        single_Formals(formal(S_arg, S_Int)),    S_SELF_TYPE, no_expr()))
                ),
                single_Features(method(S_in_string, nil_Formals(), S_String, no_expr()))
            ),
            single_Features(method(S_in_int, nil_Formals(), S_Int, no_expr()))
        ), fn);

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ C_Int = class_(S_Int, S_Object,
        single_Features(attr(S_val, S_prim_slot, no_expr())), fn);

    Class_ C_Bool = class_(S_Bool, S_Object,
        single_Features(attr(S_val, S_prim_slot, no_expr())), fn);

    Class_ C_String = class_(S_String, S_Object,
        append_Features(
            append_Features(
                append_Features(
                    append_Features(
                        single_Features(attr(S_val,       S_Int,       no_expr())),
                        single_Features(attr(S_str_field, S_prim_slot, no_expr()))
                    ),
                    single_Features(method(S_length, nil_Formals(), S_Int, no_expr()))
                ),
                single_Features(method(S_concat,
                    single_Formals(formal(S_arg, S_String)), S_String, no_expr()))
            ),
            single_Features(method(S_substr,
                append_Formals(
                    single_Formals(formal(S_arg,  S_Int)),
                    single_Formals(formal(S_arg2, S_Int))
                ), S_String, no_expr()))
        ), fn);

    registry_[S_Object] = C_Object;
    registry_[S_IO]     = C_IO;
    registry_[S_Int]    = C_Int;
    registry_[S_Bool]   = C_Bool;
    registry_[S_String] = C_String;

    parent_of_[S_Object] = S_No_class;
    parent_of_[S_IO]     = S_Object;
    parent_of_[S_Int]    = S_Object;
    parent_of_[S_Bool]   = S_Object;
    parent_of_[S_String] = S_Object;
}

bool ClassTable::register_user_classes(Classes classes) {
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        Class_ c    = classes->nth(i);
        Symbol name = c->GetName();

        if (name == S_SELF_TYPE || name == S_Object || name == S_IO  ||
            name == S_Int        || name == S_Bool   || name == S_String)
        {
            semant_error(c) << "Redefinition of class " << name
                            << " is not allowed.\n";
            return false;
        }

        if (registry_.count(name)) {
            semant_error(c) << "Class " << name << " already defined.\n";
            return false;
        }

        registry_[name] = c;
    }
    return true;
}

bool ClassTable::link_inheritance_chain() {
    for (std::map<Symbol, Class_>::const_iterator it_reg = registry_.begin(); it_reg != registry_.end(); ++it_reg) {
        Symbol cls_name = it_reg->first;

        if (parent_of_.count(cls_name))
            continue;

        Class_  cls = it_reg->second;
        Symbol  par = cls->GetParent();

        if (par == S_Int || par == S_Bool || par == S_String ||
            par == S_SELF_TYPE)
        {
            semant_error(cls)
                << "Class " << cls_name
                << " cannot inherit from " << par << ".\n";
            return false;
        }

        if (!registry_.count(par)) {
            semant_error(cls)
                << "Class " << cls_name
                << " inherits from undefined class " << par << ".\n";
            return false;
        }

        parent_of_[cls_name] = par;
    }
    return true;
}

bool ClassTable::dfs_cycle(Symbol n, std::map<Symbol,int>& colour) {
    colour[n] = 1;

    for (std::map<Symbol, Class_>::const_iterator it_reg = registry_.begin(); it_reg != registry_.end(); ++it_reg) {
        Symbol child = it_reg->first;
        if (!parent_of_.count(child) || parent_of_[child] != n)
            continue;

        if (colour[child] == 1) {
            semant_error() << "Circular inheritance detected involving "
                           << child << ".\n";
            return false;
        }
        if (colour[child] == 0 && !dfs_cycle(child, colour))
            return false;
    }

    colour[n] = 2;
    return true;
}

bool ClassTable::no_cycles() {
    std::map<Symbol,int> colour;
    for (std::map<Symbol, Class_>::const_iterator it_reg = registry_.begin(); it_reg != registry_.end(); ++it_reg)
        colour[it_reg->first] = 0;
    return dfs_cycle(S_Object, colour);
}

bool ClassTable::validate_main() {
    if (!registry_.count(S_Main)) {
        semant_error() << "Class Main is not defined.\n";
        return false;
    }
    return true;
}

void ClassTable::bfs_collect(Symbol root) {
    std::queue<Symbol> q;
    q.push(root);

    while (!q.empty()) {
        Symbol cls_name = q.front();
        q.pop();

        Class_   cls      = registry_[cls_name];
        Features features = cls->GetFeatures();

        if (parent_of_.count(cls_name) && parent_of_[cls_name] != S_No_class) {
            Symbol par = parent_of_[cls_name];
            flat_methods_[cls_name]    = flat_methods_[par];
            flat_attr_types_[cls_name] = flat_attr_types_[par];
        } else {
            flat_methods_[cls_name]    = std::map<Symbol, method_class*>();
            flat_attr_types_[cls_name] = std::map<Symbol, Symbol>();
        }

        for (int i = features->first(); features->more(i); i = features->next(i)) {
            Feature f = features->nth(i);

            if (f->IsMethod()) {
                method_class *m    = static_cast<method_class*>(f);
                Symbol        mnam = m->GetName();
                Symbol        mret = m->GetType();

                if (flat_methods_[cls_name].count(mnam)) {
                    method_class *pm  = flat_methods_[cls_name][mnam];
                    Symbol        prt = pm->GetType();

                    if (mret != prt) {
                        semant_error(cls)
                            << "Return type of overriding method " << mnam
                            << " (" << mret << ") differs from parent ("
                            << prt << ").\n";
                    }

                    Formals mf = m->GetFormals();
                    Formals pf = pm->GetFormals();
                    int mc=0, pc=0;
                    for (int j=mf->first(); mf->more(j); j=mf->next(j)) mc++;
                    for (int j=pf->first(); pf->more(j); j=pf->next(j)) pc++;

                    if (mc != pc) {
                        semant_error(cls)
                            << "Overriding method " << mnam
                            << " has different arity than parent definition.\n";
                    } else {
                        int mi=mf->first(), pi=pf->first();
                        for (; mf->more(mi) && pf->more(pi);
                               mi=mf->next(mi), pi=pf->next(pi))
                        {
                            Symbol mt = mf->nth(mi)->GetType();
                            Symbol pt = pf->nth(pi)->GetType();
                            if (mt != pt) {
                                semant_error(cls)
                                    << "Parameter type mismatch in override of "
                                    << mnam << ": " << mt
                                    << " vs parent's " << pt << ".\n";
                            }
                        }
                    }
                }
                flat_methods_[cls_name][mnam] = m;
            } else {
                attr_class *a    = static_cast<attr_class*>(f);
                Symbol      anam = a->GetName();

                Symbol atyp = a->GetType();

                if (flat_attr_types_[cls_name].count(anam)) {
                    semant_error(cls)
                        << "Attribute " << anam
                        << " already defined in a parent class.\n";
                } else {
                    flat_attr_types_[cls_name][anam] = atyp;
                }
            }
        }

        for (std::map<Symbol, Class_>::const_iterator it_reg = registry_.begin(); it_reg != registry_.end(); ++it_reg) {
            if (parent_of_.count(it_reg->first) && parent_of_[it_reg->first] == cls_name)
                q.push(it_reg->first);
        }
    }
}

void ClassTable::build_all_features() {
    bfs_collect(S_Object);
}

bool ClassTable::known(Symbol name) const {
    return registry_.count(name) > 0;
}

Symbol ClassTable::parent_of(Symbol n) const {
    std::map<Symbol, Symbol>::const_iterator it = parent_of_.find(n);
    return it == parent_of_.end() ? S_No_class : it->second;
}

Class_ ClassTable::get_class(Symbol n) const {
    std::map<Symbol, Class_>::const_iterator it = registry_.find(n);
    return it == registry_.end() ? (Class_)0 : it->second;
}

method_class* ClassTable::find_method(Symbol cls, Symbol mname) const {
    std::map<Symbol, std::map<Symbol, method_class*> >::const_iterator it = flat_methods_.find(cls);
    if (it == flat_methods_.end()) return 0;
    std::map<Symbol, method_class*>::const_iterator jt = it->second.find(mname);
    return jt == it->second.end() ? 0 : jt->second;
}

Symbol ClassTable::find_attr_type(Symbol cls, Symbol aname) const {
    std::map<Symbol, std::map<Symbol, Symbol> >::const_iterator it = flat_attr_types_.find(cls);
    if (it == flat_attr_types_.end()) return 0;
    std::map<Symbol, Symbol>::const_iterator jt = it->second.find(aname);
    return jt == it->second.end() ? 0 : jt->second;
}

bool ClassTable::conforms(Symbol t1, Symbol t2, Symbol self_cls) const {
    if (t1 == S_No_type)
        return true;
    if (t1 == S_SELF_TYPE && t2 == S_SELF_TYPE)
        return true;
    if (t2 == S_SELF_TYPE)
        return false;
    if (t1 == S_SELF_TYPE)
        t1 = self_cls;

    for (Symbol c = t1; c != S_No_class; ) {
        if (c == t2) return true;
        std::map<Symbol, Symbol>::const_iterator it = parent_of_.find(c);
        if (it == parent_of_.end()) break;
        c = it->second;
    }
    return false;
}

Symbol ClassTable::lub(Symbol t1, Symbol t2, Symbol self_cls) const {
    if (t1 == S_SELF_TYPE && t2 == S_SELF_TYPE)
        return S_SELF_TYPE;
    if (t1 == S_SELF_TYPE) t1 = self_cls;
    if (t2 == S_SELF_TYPE) t2 = self_cls;

    std::set<Symbol> t2_anc;
    for (Symbol c = t2; c != S_No_class; ) {
        t2_anc.insert(c);
        std::map<Symbol, Symbol>::const_iterator it = parent_of_.find(c);
        if (it == parent_of_.end()) break;
        c = it->second;
    }

    for (Symbol c = t1; c != S_No_class; ) {
        if (t2_anc.count(c)) return c;
        std::map<Symbol, Symbol>::const_iterator it = parent_of_.find(c);
        if (it == parent_of_.end()) break;
        c = it->second;
    }
    return S_Object;
}

////////////////////////////////////////////////////////////////////
//
// semant_error is an overloaded function for reporting errors
// during semantic analysis.  There are three versions:
//
//    ostream& ClassTable::semant_error()                
//
//    ostream& ClassTable::semant_error(Class_ c)
//       print line number and filename for `c'
//
//    ostream& ClassTable::semant_error(Symbol filename, tree_node *t)  
//       print a line number and filename
//
///////////////////////////////////////////////////////////////////
ostream& ClassTable::semant_error(Class_ c) {
    return semant_error(c->get_filename(), c);
}

ostream& ClassTable::semant_error(Symbol filename, tree_node *t) {
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& ClassTable::semant_error() {
    semant_errors++;
    return error_stream;
}

static void push_attr_scope(Symbol cls_name) {
    g_env->enterscope();

    g_env->addid(S_self, new Symbol(S_SELF_TYPE));

    std::vector<Symbol> chain;
    for (Symbol c = cls_name; c != S_No_class; c = g_ct->parent_of(c))
        chain.push_back(c);

    for (int k = (int)chain.size() - 1; k >= 0; k--) {
        Symbol   anc   = chain[k];
        Class_   acls  = g_ct->get_class(anc);
        Features feats = acls->GetFeatures();

        for (int i = feats->first(); feats->more(i); i = feats->next(i)) {
            Feature f = feats->nth(i);
            if (f->IsMethod()) continue;

            attr_class *a    = static_cast<attr_class*>(f);
            Symbol      anam = a->GetName();
            if (anam == S_self) continue;

            Symbol atyp = g_ct->find_attr_type(cls_name, anam);
            if (atyp != 0)
                g_env->addid(anam, new Symbol(atyp));
        }
    }
}

Symbol int_const_class::CheckExprType() {
    set_type(S_Int);    return S_Int;
}
Symbol bool_const_class::CheckExprType() {
    set_type(S_Bool);   return S_Bool;
}
Symbol string_const_class::CheckExprType() {
    set_type(S_String); return S_String;
}

Symbol no_expr_class::CheckExprType() {
    set_type(S_No_type); return S_No_type;
}

Symbol object_class::CheckExprType() {
    if (name == S_self) {
        set_type(S_SELF_TYPE);
        return S_SELF_TYPE;
    }

    Symbol *t = g_env->lookup(name);
    if (!t) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "Identifier " << name << " not declared.\n";
        set_type(S_Object);
        return S_Object;
    }
    set_type(*t);
    return *t;
}

Symbol new__class::CheckExprType() {
    if (type_name != S_SELF_TYPE && !g_ct->known(type_name)) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "'new' applied to undefined type " << type_name << ".\n";
        set_type(S_Object);
        return S_Object;
    }
    set_type(type_name);
    return type_name;
}

Symbol isvoid_class::CheckExprType() {
    e1->CheckExprType();
    set_type(S_Bool);
    return S_Bool;
}

static Symbol check_arith_op(Expression left, Expression right,
                               const char *op_name)
{
    Symbol l = left->CheckExprType();
    Symbol r = right->CheckExprType();
    if (l != S_Int || r != S_Int) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "Operator '" << op_name
            << "' requires Int operands; got " << l << " and " << r << ".\n";
        return S_Object;
    }
    return S_Int;
}

Symbol plus_class::CheckExprType() {
    Symbol t = check_arith_op(e1, e2, "+"); set_type(t); return t;
}
Symbol sub_class::CheckExprType() {
    Symbol t = check_arith_op(e1, e2, "-"); set_type(t); return t;
}
Symbol mul_class::CheckExprType() {
    Symbol t = check_arith_op(e1, e2, "*"); set_type(t); return t;
}
Symbol divide_class::CheckExprType() {
    Symbol t = check_arith_op(e1, e2, "/"); set_type(t); return t;
}

Symbol neg_class::CheckExprType() {
    Symbol t = e1->CheckExprType();
    if (t != S_Int) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "'~' requires an Int operand; got " << t << ".\n";
        set_type(S_Object); return S_Object;
    }
    set_type(S_Int); return S_Int;
}

Symbol lt_class::CheckExprType() {
    Symbol l = e1->CheckExprType();
    Symbol r = e2->CheckExprType();
    if (l != S_Int || r != S_Int)
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "'<' requires Int operands; got " << l << " and " << r << ".\n";
    set_type(S_Bool); return S_Bool;
}

Symbol leq_class::CheckExprType() {
    Symbol l = e1->CheckExprType();
    Symbol r = e2->CheckExprType();
    if (l != S_Int || r != S_Int)
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "'<=' requires Int operands; got " << l << " and " << r << ".\n";
    set_type(S_Bool); return S_Bool;
}

Symbol eq_class::CheckExprType() {
    Symbol l = e1->CheckExprType();
    Symbol r = e2->CheckExprType();

    bool lprim = (l == S_Int || l == S_Bool || l == S_String);
    bool rprim = (r == S_Int || r == S_Bool || r == S_String);
    if ((lprim || rprim) && l != r) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "Illegal comparison: cannot use '=' to compare "
            << l << " with " << r << ".\n";
    }
    set_type(S_Bool); return S_Bool;
}

Symbol comp_class::CheckExprType() {
    Symbol t = e1->CheckExprType();
    if (t != S_Bool) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "'not' requires a Bool operand; got " << t << ".\n";
    }
    set_type(S_Bool); return S_Bool;
}

Symbol assign_class::CheckExprType() {
    if (name == S_self) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "Cannot assign to 'self'.\n";

        Symbol t = expr->CheckExprType();
        set_type(t); return t;
    }

    Symbol *declared = g_env->lookup(name);
    if (!declared) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "Assignment to undeclared identifier " << name << ".\n";
        set_type(S_Object); return S_Object;
    }

    Symbol rhs = expr->CheckExprType();
    if (!g_ct->conforms(rhs, *declared, g_cls)) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "Assigned type " << rhs
            << " does not conform to declared type " << *declared
            << " of " << name << ".\n";
    }
    set_type(rhs); return rhs;
}

Symbol block_class::CheckExprType() {
    Symbol last = S_Object;
    for (int i = body->first(); body->more(i); i = body->next(i))
        last = body->nth(i)->CheckExprType();
    set_type(last); return last;
}

Symbol cond_class::CheckExprType() {
    Symbol pt = pred->CheckExprType();
    if (pt != S_Bool)
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "Condition of 'if' must be Bool; got " << pt << ".\n";

    Symbol tt = then_exp->CheckExprType();
    Symbol et = else_exp->CheckExprType();
    Symbol rt = g_ct->lub(tt, et, g_cls);
    set_type(rt); return rt;
}

Symbol loop_class::CheckExprType() {
    Symbol pt = pred->CheckExprType();
    if (pt != S_Bool)
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "Condition of 'while' must be Bool; got " << pt << ".\n";
    body->CheckExprType();
    set_type(S_Object); return S_Object;
}

Symbol let_class::CheckExprType() {
    if (identifier == S_self) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "'self' cannot be bound in a 'let'.\n";
    }
    if (type_decl != S_SELF_TYPE && !g_ct->known(type_decl)) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "Let-bound identifier " << identifier
            << " has undefined type " << type_decl << ".\n";
    }

    Symbol init_t = init->CheckExprType();
    if (init_t != S_No_type && !g_ct->conforms(init_t, type_decl, g_cls)) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "Let initialiser has type " << init_t
            << " which does not conform to declared type " << type_decl
            << " for identifier " << identifier << ".\n";
    }

    g_env->enterscope();
    g_env->addid(identifier, new Symbol(type_decl));
    Symbol bt = body->CheckExprType();
    g_env->exitscope();

    set_type(bt); return bt;
}

Symbol branch_class::CheckBranchType() {
    if (name == S_self)
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "'self' cannot be bound in a case branch.\n";

    g_env->enterscope();
    g_env->addid(name, new Symbol(type_decl));
    Symbol bt = expr->CheckExprType();
    g_env->exitscope();

    return bt;
}

Symbol typcase_class::CheckExprType() {
    expr->CheckExprType();

    std::set<Symbol> seen;
    Symbol result = S_No_type;

    for (int i = cases->first(); cases->more(i); i = cases->next(i)) {
        branch_class *br   = static_cast<branch_class*>(cases->nth(i));
        Symbol        decl = br->GetTypeDecl();

        if (seen.count(decl)) {
            g_ct->semant_error(g_ct->get_class(g_cls))
                << "Duplicate branch type " << decl
                << " in case expression.\n";
        }
        seen.insert(decl);

        Symbol bt = br->CheckBranchType();
        result = (result == S_No_type) ? bt : g_ct->lub(result, bt, g_cls);
    }

    set_type(result); return result;
}

static Symbol do_dispatch(Symbol lookup_cls, Symbol recv_type,
                           Symbol mname, Expressions actuals,
                           tree_node * )
{
    method_class *mdef = g_ct->find_method(lookup_cls, mname);
    if (!mdef) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "Dispatch to undefined method " << mname
            << " on class " << lookup_cls << ".\n";
        return S_Object;
    }

    Formals decl_f = mdef->GetFormals();

    int dc=0, ac=0;
    for (int j=decl_f->first();  decl_f->more(j);  j=decl_f->next(j))  dc++;
    for (int j=actuals->first(); actuals->more(j); j=actuals->next(j)) ac++;

    if (dc != ac) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "Method " << mname << " called with " << ac
            << " argument(s) but expects " << dc << ".\n";
    }

    int di=decl_f->first(), ai=actuals->first();
    for (; decl_f->more(di) && actuals->more(ai);
           di=decl_f->next(di), ai=actuals->next(ai))
    {
        Symbol at = actuals->nth(ai)->CheckExprType();
        Symbol dt = decl_f->nth(di)->GetType();
        if (!g_ct->conforms(at, dt, g_cls)) {
            g_ct->semant_error(g_ct->get_class(g_cls))
                << "Argument " << decl_f->nth(di)->GetName()
                << " of dispatch to " << mname
                << " has type " << at
                << " which does not conform to declared type " << dt << ".\n";
        }
    }

    for (; actuals->more(ai); ai=actuals->next(ai))
        actuals->nth(ai)->CheckExprType();

    Symbol ret = mdef->GetType();
    return (ret == S_SELF_TYPE) ? recv_type : ret;
}

Symbol dispatch_class::CheckExprType() {
    Symbol et = expr->CheckExprType();

    if (et != S_SELF_TYPE && !g_ct->known(et)) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "Dispatch on expression of unknown type " << et << ".\n";
        set_type(S_Object); return S_Object;
    }

    Symbol lookup = (et == S_SELF_TYPE) ? g_cls : et;
    Symbol rt     = do_dispatch(lookup, et, name, actual, this);
    set_type(rt); return rt;
}

Symbol static_dispatch_class::CheckExprType() {
    Symbol et = expr->CheckExprType();

    if (!g_ct->known(type_name)) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "Static dispatch on undefined class " << type_name << ".\n";
        set_type(S_Object); return S_Object;
    }

    if (!g_ct->conforms(et, type_name, g_cls)) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "Receiver type " << et
            << " does not conform to static dispatch type " << type_name << ".\n";
    }

    Symbol rt = do_dispatch(type_name, et, name, actual, this);
    set_type(rt); return rt;
}

void method_class::CheckFeatureType() {
    if (return_type != S_SELF_TYPE && !g_ct->known(return_type))
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "Method " << name << " has unknown return type "
            << return_type << ".\n";

    g_env->enterscope();
    std::set<Symbol> seen;

    for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
        Formal  f     = formals->nth(i);
        Symbol  fname = f->GetName();
        Symbol  ftype = f->GetType();

        if (fname == S_self) {
            g_ct->semant_error(g_ct->get_class(g_cls))
                << "'self' cannot be a formal parameter of method " << name << ".\n";
        } else if (seen.count(fname)) {
            g_ct->semant_error(g_ct->get_class(g_cls))
                << "Duplicate formal parameter " << fname
                << " in method " << name << ".\n";
        } else {
            seen.insert(fname);
            if (!g_ct->known(ftype))
                g_ct->semant_error(g_ct->get_class(g_cls))
                    << "Formal " << fname << " in method " << name
                    << " has unknown type " << ftype << ".\n";
            else
                g_env->addid(fname, new Symbol(ftype));
        }
    }

    Symbol body_t = expr->CheckExprType();
    if (!g_ct->conforms(body_t, return_type, g_cls)) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "Body of method " << name << " has type " << body_t
            << " which does not conform to declared return type "
            << return_type << ".\n";
    }

    g_env->exitscope();
}

void attr_class::CheckFeatureType() {
    if (name == S_self) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "'self' cannot be the name of an attribute.\n";
        return;
    }

    if (type_decl != S_SELF_TYPE && !g_ct->known(type_decl)) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "Attribute " << name
            << " has unknown declared type " << type_decl << ".\n";
        return;
    }

    Symbol it = init->CheckExprType();
    if (it == S_No_type) return;

    if (!g_ct->conforms(it, type_decl, g_cls)) {
        g_ct->semant_error(g_ct->get_class(g_cls))
            << "Initialiser of attribute " << name
            << " has type " << it
            << " which does not conform to declared type " << type_decl << ".\n";
    }
}

void method_class::AddMethodToTable(Symbol ) {  }
void method_class::AddAttribToTable(Symbol ) {        }
void attr_class::AddMethodToTable(Symbol )   {      }
void attr_class::AddAttribToTable(Symbol )   { }

void program_class::semant() {
    initialize_constants();

    /* ClassTable constructor may do some semantic analysis */
    g_ct = new ClassTable(classes);

    if (!g_ct->register_user_classes(classes)) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }

    if (!g_ct->link_inheritance_chain()) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }

    if (!g_ct->no_cycles()) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }

    if (!g_ct->validate_main()) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }

    if (g_ct->errors()) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }

    g_ct->build_all_features();

    if (g_ct->errors()) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }

    g_env = new SymbolTable<Symbol, Symbol>();

    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        Class_  cls      = classes->nth(i);
        Symbol  cls_name = cls->GetName();
        g_cls            = cls_name;

        push_attr_scope(cls_name);

        Features feats = cls->GetFeatures();
        for (int j = feats->first(); feats->more(j); j = feats->next(j))
            feats->nth(j)->CheckFeatureType();

        g_env->exitscope();
    }

    if (g_ct->errors()) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }
}