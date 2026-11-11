#ifndef SEMANT_H_
#define SEMANT_H_

#include <assert.h>
#include <iostream>
#include <map>
#include <set>
#include <list>
#include <vector>
#include "cool-tree.h"
#include "stringtab.h"
#include "symtab.h"
#include "list.h"

#define TRUE  1
#define FALSE 0

class ClassTable;
typedef ClassTable *ClassTableP;

// This is a structure that may be used to contain the semantic
// information such as the inheritance graph.  You may use it or not as
// you like: it is only here to provide a container for the supplied
// methods.

class ClassTable {
private:
    int       semant_errors;
    ostream&  error_stream;

    std::map<Symbol, Class_> registry_;

    std::map<Symbol, Symbol> parent_of_;

    std::map<Symbol, std::map<Symbol, method_class*> > flat_methods_;
    std::map<Symbol, std::map<Symbol, Symbol> >         flat_attr_types_;

    void install_basic_classes();
    void bfs_collect(Symbol root);
    bool dfs_cycle(Symbol n,
                   std::map<Symbol,int>& colour);

public:
    explicit ClassTable(Classes classes);

    int      errors()        { return semant_errors; }
    ostream& semant_error();
    ostream& semant_error(Class_ c);
    ostream& semant_error(Symbol filename, tree_node *t);

    bool register_user_classes(Classes classes);
    bool link_inheritance_chain();
    bool no_cycles();
    bool validate_main();

    void build_all_features();

    bool    known(Symbol name)  const;
    Symbol  parent_of(Symbol n) const;
    Class_  get_class(Symbol n) const;

    bool   conforms(Symbol t1, Symbol t2, Symbol self_cls) const;

    Symbol lub(Symbol t1, Symbol t2, Symbol self_cls) const;

    method_class* find_method(Symbol cls, Symbol mname) const;

    Symbol find_attr_type(Symbol cls, Symbol aname) const;

    const std::map<Symbol, Class_>& all_classes() const { return registry_; }
};

#endif