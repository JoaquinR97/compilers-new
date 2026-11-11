#ifndef CGEN_H
#define CGEN_H

#include <assert.h>
#include <stdio.h>
#include <vector>
#include <map>
#include <algorithm>
#include <numeric>

#include "emit.h"
#include "cool-tree.h"
#include "symtab.h"

enum Basicness { Basic, NotBasic };
#define TRUE  1
#define FALSE 0

extern Symbol No_class;

class CgenClassTable;
typedef CgenClassTable *CgenClassTableP;

class CgenNode;
typedef CgenNode       *CgenNodeP;

class BoolConst {
private:
    int val;
public:
    BoolConst(int i);
    void code_def(ostream&, int boolclasstag);
    void code_ref(ostream&) const;
};

class CgenNode : public class__class {
private:
    CgenNodeP       parentnd;
    List<CgenNode> *children;
    Basicness       basic_status;

public:
    CgenNode(Class_ c, Basicness bstatus, CgenClassTableP class_table);

    void      add_child(CgenNodeP child);
    void      set_parentnd(CgenNodeP p);
    CgenNodeP get_parentnd()        { return parentnd; }
    List<CgenNode>* get_children()  { return children; }
    int       basic()               { return (basic_status == Basic); }

    int class_tag;

    std::vector<attr_class*>  m_full_attribs;
    std::map<Symbol, int>     m_attrib_idx_tab;

    std::vector<method_class*> m_full_methods;
    std::map<Symbol, int>      m_dispatch_idx;
    std::map<Symbol, Symbol>   m_dispatch_owner;

    std::vector<CgenNode*> all_ancestors() const;

    void build_attr_layout();

    std::vector<attr_class*> own_attrs();

    void build_dispatch_layout();

    std::vector<method_class*> own_methods();

    void emit_protobj(ostream& s);
    void emit_disptab(ostream& s);
    void emit_init   (ostream& s);
    void emit_methods(ostream& s);
};

class CgenClassTable : public SymbolTable<Symbol, CgenNode> {
private:
    List<CgenNode> *nds;
    ostream&        str;

    int stringclasstag;
    int intclasstag;
    int boolclasstag;

// The following creates an inheritance graph from
// a list of classes.  The graph is implemented as
// a tree of `CgenNode', and class names are placed
// in the base class symbol table.

    void install_basic_classes();
    void install_class(CgenNodeP nd);
    void install_classes(Classes cs);
    void build_inheritance_tree();

public:

    std::vector<CgenNode*>  m_ordered_nodes;
    std::map<Symbol, int>   m_class_tags;

    CgenClassTable(Classes, ostream& str);

    void Execute();

    void code();

    CgenNodeP root();

    CgenNode* lookup_class(Symbol name);

    void assign_class_tags();
    void emit_data_preamble();
    void emit_gc_selector();
    void emit_constant_objects();
    void emit_name_table();
    void emit_obj_table();
    void emit_dispatch_tables();
    void emit_prototype_objects();
    void emit_text_preamble();
    void emit_initializers();
    void emit_all_methods();

    void emit_init_ref  (Symbol cls,             ostream& os);
    void emit_method_ref(Symbol cls, Symbol meth, ostream& os);
    void emit_protobj   (Symbol cls,             ostream& os);
};

#endif