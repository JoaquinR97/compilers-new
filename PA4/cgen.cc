
//**************************************************************
//
// Code generator SKELETON
//
// Read the comments carefully. Make sure to
//    initialize the base class tags in
//       `CgenClassTable::CgenClassTable'
//
//    Add the label for the dispatch tables to
//       `IntEntry::code_def'
//       `StringEntry::code_def'
//       `BoolConst::code_def'
//
//    Add code to emit everyting else that is needed
//       in `CgenClassTable::code'
//
//
// The files as provided will produce code to begin the code
// segments, declare globals, and emit constants.  You must
// fill in the rest.
//
//**************************************************************

#include <cassert>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <stdexcept>

#include "cgen.h"
#include "cgen_gc.h"

extern void emit_string_constant(ostream& str, char* s);
extern int  cgen_debug;

static int g_label = 0;

static CgenClassTable* g_table = NULL;

//
// Three symbols from the semantic analyzer (semant.cc) are used.
// If e : No_type, then no code is generated for e.
// Special code is generated for new SELF_TYPE.
// The name "self" also generates code different from other references.
//
//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
Symbol arg, arg2, Bool, concat, cool_abort, copy,
       Int, in_int, in_string, IO, length, Main,
       main_meth, No_class, No_type, Object,
       out_int, out_string, prim_slot, self, SELF_TYPE,
       Str, str_field, substr, type_name, val;

//
// Initializing the predefined symbols.
//
static void initialize_constants() {
    arg        = idtable.add_string("arg");
    arg2       = idtable.add_string("arg2");
    Bool       = idtable.add_string("Bool");
    concat     = idtable.add_string("concat");
    cool_abort = idtable.add_string("abort");
    copy       = idtable.add_string("copy");
    Int        = idtable.add_string("Int");
    in_int     = idtable.add_string("in_int");
    in_string  = idtable.add_string("in_string");
    IO         = idtable.add_string("IO");
    length     = idtable.add_string("length");
    Main       = idtable.add_string("Main");
    main_meth  = idtable.add_string("main");
//   _no_class is a symbol that can't be the name of any 
//   user-defined class.
    No_class   = idtable.add_string("_no_class");
    No_type    = idtable.add_string("_no_type");
    Object     = idtable.add_string("Object");
    out_int    = idtable.add_string("out_int");
    out_string = idtable.add_string("out_string");
    prim_slot  = idtable.add_string("_prim_slot");
    self       = idtable.add_string("self");
    SELF_TYPE  = idtable.add_string("SELF_TYPE");
    Str        = idtable.add_string("String");
    str_field  = idtable.add_string("_str_field");
    substr     = idtable.add_string("substr");
    type_name  = idtable.add_string("type_name");
    val        = idtable.add_string("_val");
}

static const char* gc_init_names[]    = { "_NoGC_Init",    "_GenGC_Init",    "_ScnGC_Init"    };
static const char* gc_collect_names[] = { "_NoGC_Collect", "_GenGC_Collect", "_ScnGC_Collect" };

//  BoolConst is a class that implements code generation for operations
//  on the two booleans, which are given global names here.
BoolConst falsebool(FALSE);
BoolConst truebool(TRUE);

static void xload (const char* dst, int off, const char* base, ostream& s)
    { s << LW  << dst << " " << off * WORD_SIZE << "(" << base << ")\n"; }

static void xstore(const char* src, int off, const char* base, ostream& s)
    { s << SW  << src << " " << off * WORD_SIZE << "(" << base << ")\n"; }

static void xli  (const char* dst, int imm,          ostream& s)
    { s << LI  << dst << " " << imm << "\n"; }

static void xla  (const char* dst, const char* addr, ostream& s)
    { s << LA  << dst << " " << addr << "\n"; }

static void xmove(const char* dst, const char* src,  ostream& s)
    { s << MOVE << dst << " " << src << "\n"; }

static void xneg (const char* dst, const char* src,  ostream& s)
    { s << NEG << dst << " " << src << "\n"; }

static void xadd (const char* dst, const char* a, const char* b, ostream& s)
    { s << ADD << dst << " " << a << " " << b << "\n"; }

static void xsub (const char* dst, const char* a, const char* b, ostream& s)
    { s << SUB << dst << " " << a << " " << b << "\n"; }

static void xmul (const char* dst, const char* a, const char* b, ostream& s)
    { s << MUL << dst << " " << a << " " << b << "\n"; }

static void xdiv (const char* dst, const char* a, const char* b, ostream& s)
    { s << DIV << dst << " " << a << " " << b << "\n"; }

static void xaddiu(const char* dst, const char* src, int imm,   ostream& s)
    { s << ADDIU << dst << " " << src << " " << imm << "\n"; }

static void xsll (const char* dst, const char* src, int sh,     ostream& s)
    { s << SLL << dst << " " << src << " " << sh << "\n"; }

static void xjal (const char* target,                            ostream& s)
    { s << JAL  << target << "\n"; }

static void xjalr(const char* reg,                               ostream& s)
    { s << JALR << "\t" << reg << "\n"; }

static void xret (ostream& s)
    { s << RET << "\n"; }

static void xlabel(int n, ostream& s)
    { s << "label" << n << ":\n"; }

static void xbranch(int n, ostream& s)
    { s << BRANCH << "label" << n << "\n"; }

static void xbeq(const char* a, const char* b, int n, ostream& s)
    { s << BEQ << a << " " << b << " label" << n << "\n"; }

static void xbne(const char* a, const char* b, int n, ostream& s)
    { s << BNE << a << " " << b << " label" << n << "\n"; }

static void xblt(const char* a, const char* b, int n, ostream& s)
    { s << BLT << a << " " << b << " label" << n << "\n"; }

static void xble(const char* a, const char* b, int n, ostream& s)
    { s << BLEQ << a << " " << b << " label" << n << "\n"; }

static void push_acc(ostream& s) {
    xstore(ACC, 0, SP, s);
    xaddiu(SP, SP, -4, s);
}

static void push_reg(const char* reg, ostream& s) {
    xstore(reg, 0, SP, s);
    xaddiu(SP, SP, -4, s);
}

static void pop_into(const char* reg, ostream& s) {
    xaddiu(SP, SP, 4, s);
    xload(reg, 0, SP, s);
}

static void discard(int n, ostream& s) {
    if (n > 0) xaddiu(SP, SP, n * 4, s);
}

static void unbox_int(const char* dst, const char* obj, ostream& s)
    { xload(dst, DEFAULT_OBJFIELDS, obj, s); }

static void rebox_int(const char* src, const char* obj, ostream& s)
    { xstore(src, DEFAULT_OBJFIELDS, obj, s); }

static std::string disptab_name(Symbol cls)
    { return std::string(cls->get_string()) + DISPTAB_SUFFIX; }

static std::string protobj_name(Symbol cls)
    { return std::string(cls->get_string()) + PROTOBJ_SUFFIX; }

static std::string initfn_name(Symbol cls)
    { return std::string(cls->get_string()) + CLASSINIT_SUFFIX; }

static std::string method_name(Symbol cls, Symbol meth)
    { return std::string(cls->get_string()) + METHOD_SEP + meth->get_string(); }

static void load_bool_const(const char* dst, const BoolConst& b, ostream& s) {
    s << LA << dst << " ";
    b.code_ref(s);
    s << "\n";
}

static void load_string_const(const char* dst, StringEntry* e, ostream& s) {
    s << LA << dst << " ";
    e->code_ref(s);
    s << "\n";
}

static void load_int_const(const char* dst, IntEntry* e, ostream& s) {
    s << LA << dst << " ";
    e->code_ref(s);
    s << "\n";
}

///////////////////////////////////////////////////////////////////////////////
//
// coding strings, ints, and booleans
//
// Cool has three kinds of constants: strings, ints, and booleans.
// This section defines code generation for each type.
//
// All string constants are listed in the global "stringtable" and have
// type StringEntry.  StringEntry methods are defined both for String
// constant definitions and references.
//
// All integer constants are listed in the global "inttable" and have
// type IntEntry.  IntEntry methods are defined for Int
// constant definitions and references.
//
// Since there are only two Bool values, there is no need for a table.
// The two booleans are represented by instances of the class BoolConst,
// which defines the definition and reference methods for Bools.
//
///////////////////////////////////////////////////////////////////////////////
void StringEntry::code_ref(ostream& s)  { s << STRCONST_PREFIX  << index; }
void IntEntry::code_ref   (ostream& s)  { s << INTCONST_PREFIX  << index; }
void BoolConst::code_ref  (ostream& s) const { s << BOOLCONST_PREFIX << val; }

BoolConst::BoolConst(int i) : val(i) { assert(i == 0 || i == 1); }

void StringEntry::code_def(ostream& s, int tag) {
    IntEntryP lensym = inttable.add_int(len);
    s << WORD << "-1\n";
    code_ref(s);
    s << LABEL
      << WORD << tag                                             << "\n"
      << WORD << (DEFAULT_OBJFIELDS + STRING_SLOTS + (len+4)/4) << "\n"
      << WORD << Str << DISPTAB_SUFFIX                          << "\n"
      << WORD; lensym->code_ref(s); s << "\n";
    emit_string_constant(s, str);
    s << ALIGN;
}

void IntEntry::code_def(ostream& s, int tag) {
    s << WORD << "-1\n";
    code_ref(s);
    s << LABEL
      << WORD << tag                              << "\n"
      << WORD << (DEFAULT_OBJFIELDS + INT_SLOTS)  << "\n"
      << WORD << Int << DISPTAB_SUFFIX            << "\n"
      << WORD << str                              << "\n";
}

void BoolConst::code_def(ostream& s, int tag) {
    s << WORD << "-1\n";
    code_ref(s);
    s << LABEL
      << WORD << tag                              << "\n"
      << WORD << (DEFAULT_OBJFIELDS + BOOL_SLOTS) << "\n"
      << WORD << Bool << DISPTAB_SUFFIX           << "\n"
      << WORD << val                              << "\n";
}

void StrTable::code_string_table(ostream& s, int tag) {
    for (List<StringEntry>* l = tbl; l; l = l->tl()) l->hd()->code_def(s, tag);
}

void IntTable::code_string_table(ostream& s, int tag) {
    for (List<IntEntry>* l = tbl; l; l = l->tl()) l->hd()->code_def(s, tag);
}

struct FrameTracker {
    CgenNode*           class_node;
    std::vector<Symbol> stack_syms;
    std::vector<Symbol> params;
    int                 depth;

    FrameTracker() : class_node(NULL), depth(0) {}

    int push_var(Symbol name) {
        stack_syms.push_back(name);
        depth++;
        return depth;
    }

    void pop_vars(int n) {
        for (int i = 0; i < n; ++i) stack_syms.pop_back();
        depth -= n;
    }

    void add_param(Symbol name) { params.push_back(name); }

    int lookup_local(Symbol name) const {
        for (int i = (int)stack_syms.size() - 1; i >= 0; --i) {
            if (stack_syms[i] == name) {

                return (int)stack_syms.size() - i;
            }
        }
        return -1;
    }

    int lookup_param(Symbol name) const {
        for (int i = 0; i < (int)params.size(); ++i) {
            if (params[i] == name) {
                return (int)params.size() - i + 2;
            }
        }
        return -1;
    }

    int lookup_attrib(Symbol name) const {
        if (!class_node) return -1;
        const std::map<Symbol, int>& tab = class_node->m_attrib_idx_tab;
        std::map<Symbol, int>::const_iterator it = tab.find(name);
        return it == tab.end() ? -1 : it->second;
    }
};

///////////////////////////////////////////////////////////////////////
//
// CgenNode methods
//
///////////////////////////////////////////////////////////////////////
CgenNode::CgenNode(Class_ c, Basicness b, CgenClassTableP ct)
    : class__class(static_cast<const class__class&>(*c)),
      parentnd(NULL), children(NULL), basic_status(b)
{
    stringtable.add_string(name->get_string());
}

void CgenNode::add_child(CgenNodeP p)    { children = new List<CgenNode>(p, children); }
void CgenNode::set_parentnd(CgenNodeP p) { assert(!parentnd); assert(p); parentnd = p; }

std::vector<CgenNode*> CgenNode::all_ancestors() const {
    std::vector<CgenNode*> chain;
    CgenNode* cur = const_cast<CgenNode*>(this);
    while (cur && cur->name != No_class) {
        chain.push_back(cur);
        cur = cur->parentnd;
    }
    std::reverse(chain.begin(), chain.end());
    return chain;
}

void CgenNode::build_attr_layout() {
    if (!m_full_attribs.empty()) return;
    std::vector<CgenNode*> ancestors = all_ancestors();
    for (std::vector<CgenNode*>::iterator ai = ancestors.begin(); ai != ancestors.end(); ++ai) {
        CgenNode* anc = *ai;
        Features feats = anc->features;
        for (int i = feats->first(); feats->more(i); i = feats->next(i)) {
            Feature f = feats->nth(i);
            if (!f->IsMethod()) {
                m_full_attribs.push_back(static_cast<attr_class*>(f));
                m_attrib_idx_tab[f->GetName()] = (int)m_full_attribs.size() - 1;
            }
        }
    }
}

std::vector<attr_class*> CgenNode::own_attrs() {
    std::vector<attr_class*> result;
    for (int i = features->first(); features->more(i); i = features->next(i)) {
        Feature f = features->nth(i);
        if (!f->IsMethod()) result.push_back(static_cast<attr_class*>(f));
    }
    return result;
}

void CgenNode::build_dispatch_layout() {
    if (!m_full_methods.empty()) return;
    std::vector<CgenNode*> ancestors = all_ancestors();
    for (std::vector<CgenNode*>::iterator ai = ancestors.begin(); ai != ancestors.end(); ++ai) {
        CgenNode* anc = *ai;
        Features feats = anc->features;
        for (int i = feats->first(); feats->more(i); i = feats->next(i)) {
            Feature f = feats->nth(i);
            if (f->IsMethod()) {
                Symbol mname = f->GetName();
                method_class* mf = static_cast<method_class*>(f);
                std::map<Symbol, int>::iterator it = m_dispatch_idx.find(mname);
                if (it == m_dispatch_idx.end()) {
                    m_dispatch_idx[mname]   = (int)m_full_methods.size();
                    m_dispatch_owner[mname] = anc->name;
                    m_full_methods.push_back(mf);
                } else {

                    m_full_methods[it->second] = mf;
                    m_dispatch_owner[mname]    = anc->name;
                }
            }
        }
    }
}

std::vector<method_class*> CgenNode::own_methods() {
    std::vector<method_class*> result;
    for (int i = features->first(); features->more(i); i = features->next(i)) {
        Feature f = features->nth(i);
        if (f->IsMethod()) result.push_back(static_cast<method_class*>(f));
    }
    return result;
}

void CgenNode::emit_protobj(ostream& s) {
    build_attr_layout();
    s << WORD << "-1\n";
    s << name << PROTOBJ_SUFFIX << LABEL;
    s << WORD << class_tag    << "\t# class tag\n";
    s << WORD << (DEFAULT_OBJFIELDS + (int)m_full_attribs.size()) << "\t# object size\n";
    s << WORD << name << DISPTAB_SUFFIX << "\n";

    for (std::vector<attr_class*>::iterator ai = m_full_attribs.begin(); ai != m_full_attribs.end(); ++ai) {
        attr_class* a = *ai;

        if (a->name == val) {
            if (name == Str) {
                s << WORD; inttable.lookup_string("0")->code_ref(s); s << "\n";
            } else {
                s << WORD << "0\n";
            }
        } else if (a->name == str_field) {
            s << WORD << "0\n";
        } else {
            Symbol t = a->GetDeclType();
            if      (t == Int)  { s << WORD; inttable.lookup_string("0")->code_ref(s); s << "\n"; }
            else if (t == Bool) { s << WORD; falsebool.code_ref(s); s << "\n"; }
            else if (t == Str)  { s << WORD; stringtable.lookup_string("")->code_ref(s); s << "\n"; }
            else                  s << WORD << "0\n";
        }
    }
}

void CgenNode::emit_disptab(ostream& s) {
    build_dispatch_layout();
    s << name << DISPTAB_SUFFIX << LABEL;
    for (std::vector<method_class*>::iterator mi = m_full_methods.begin(); mi != m_full_methods.end(); ++mi) {
        method_class* m = *mi;
        Symbol owner = m_dispatch_owner.at(m->name);
        s << WORD << method_name(owner, m->name) << "\n";
    }
}

void CgenNode::emit_init(ostream& s) {
    build_attr_layout();

    s << name << CLASSINIT_SUFFIX << LABEL;

    xaddiu(SP, SP, -12, s);
    xstore(FP,   3, SP, s);
    xstore(SELF, 2, SP, s);
    xstore(RA,   1, SP, s);
    xaddiu(FP, SP, 4, s);
    xmove(SELF, ACC, s);

    if (parentnd && parentnd->name != No_class) {
        std::string pini = initfn_name(parentnd->name);
        xjal(pini.c_str(), s);
    }

    std::vector<attr_class*> attrs = own_attrs();
    for (std::vector<attr_class*>::iterator ai = attrs.begin(); ai != attrs.end(); ++ai) {
        attr_class* a = *ai;
        int slot = DEFAULT_OBJFIELDS + m_attrib_idx_tab.at(a->name);
        if (a->GetInitExpr()->IsEmpty()) {

            if (a->GetDeclType() == Int) {
                load_int_const(ACC, inttable.lookup_string("0"), s);
                xstore(ACC, slot, SELF, s);
            } else if (a->GetDeclType() == Bool) {
                load_bool_const(ACC, falsebool, s);
                xstore(ACC, slot, SELF, s);
            } else if (a->GetDeclType() == Str) {
                load_string_const(ACC, stringtable.lookup_string(""), s);
                xstore(ACC, slot, SELF, s);
            }

        } else {
            FrameTracker ft;
            ft.class_node = this;
            a->GetInitExpr()->code(s, ft);
            xstore(ACC, slot, SELF, s);
            if (cgen_Memmgr == 1) {
                xaddiu(A1, SELF, slot * 4, s);
                xjal("_GenGC_Assign", s);
            }
        }
    }

    xmove(ACC, SELF, s);

    xaddiu(SP, FP, -4, s);
    xload(FP,   3, SP, s);
    xload(SELF, 2, SP, s);
    xload(RA,   1, SP, s);
    xaddiu(SP, SP, 12, s);
    xret(s);
    s << "\n";
}

static void emit_method_body(method_class* m, CgenNode* cls, ostream& s) {
    s << method_name(cls->name, m->name) << LABEL;

    xaddiu(SP, SP, -12, s);
    xstore(FP,   3, SP, s);
    xstore(SELF, 2, SP, s);
    xstore(RA,   1, SP, s);
    xaddiu(FP, SP, 4, s);
    xmove(SELF, ACC, s);

    FrameTracker ft;
    ft.class_node = cls;

    Formals formals = m->GetFormals();
    for (int i = formals->first(); formals->more(i); i = formals->next(i))
        ft.add_param(formals->nth(i)->GetName());

    m->GetBodyExpr()->code(s, ft);

    xaddiu(SP, FP, -4, s);
    xload(FP,   3, SP, s);
    xload(SELF, 2, SP, s);
    xload(RA,   1, SP, s);
    xaddiu(SP, SP, 12, s);

    int nargs = 0;
    for (int i = formals->first(); formals->more(i); i = formals->next(i)) ++nargs;
    discard(nargs, s);
    xret(s);
    s << "\n";
}

void CgenNode::emit_methods(ostream& s) {
    std::vector<method_class*> methods = own_methods();
    for (std::vector<method_class*>::iterator mi = methods.begin(); mi != methods.end(); ++mi)
        emit_method_body(*mi, this, s);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CgenClassTable methods
//
//////////////////////////////////////////////////////////////////////////////
CgenClassTable::CgenClassTable(Classes classes, ostream& s)
    : nds(NULL), str(s),
      stringclasstag(-1), intclasstag(-1), boolclasstag(-1)
{
    enterscope();
    install_basic_classes();
    install_classes(classes);
    build_inheritance_tree();

    assign_class_tags();

    stringclasstag = m_class_tags.at(Str);
    intclasstag    = m_class_tags.at(Int);
    boolclasstag   = m_class_tags.at(Bool);
}

void CgenClassTable::assign_class_tags() {
    std::vector<CgenNode*> queue;
    queue.push_back(probe(Object));
    int next_tag = 0;

    while (!queue.empty()) {
        CgenNode* cur = queue.front(); queue.erase(queue.begin());
        cur->class_tag = next_tag++;
        m_class_tags[cur->name] = cur->class_tag;
        m_ordered_nodes.push_back(cur);

        std::vector<CgenNode*> kids;
        for (List<CgenNode>* l = cur->get_children(); l; l = l->tl())
            kids.push_back(l->hd());
        std::reverse(kids.begin(), kids.end());
        for (std::vector<CgenNode*>::iterator ki = kids.begin(); ki != kids.end(); ++ki) queue.push_back(*ki);
    }
}

CgenNode* CgenClassTable::lookup_class(Symbol name) {
    std::map<Symbol, int>::iterator it = m_class_tags.find(name);
    if (it == m_class_tags.end()) return NULL;
    return m_ordered_nodes[it->second];
}

void CgenClassTable::code() {

    emit_data_preamble();
    emit_gc_selector();
    emit_constant_objects();
    emit_name_table();
    emit_obj_table();
    emit_dispatch_tables();
    emit_prototype_objects();

    emit_text_preamble();

    emit_initializers();
    emit_all_methods();
}

void CgenClassTable::emit_data_preamble() {
    Symbol main_sym    = idtable.lookup_string(MAINNAME);
    Symbol string_sym  = idtable.lookup_string(STRINGNAME);
    Symbol integer_sym = idtable.lookup_string(INTNAME);
    Symbol boolc_sym   = idtable.lookup_string(BOOLNAME);

    str << "\t.data\n" << ALIGN;
    str << GLOBAL << CLASSNAMETAB << "\n";
    str << GLOBAL << CLASSOBJTAB  << "\n";
    str << GLOBAL; emit_protobj(main_sym,    str); str << "\n";
    str << GLOBAL; emit_protobj(integer_sym, str); str << "\n";
    str << GLOBAL; emit_protobj(string_sym,  str); str << "\n";
    str << GLOBAL; falsebool.code_ref(str);        str << "\n";
    str << GLOBAL; truebool.code_ref(str);         str << "\n";
    str << GLOBAL << INTTAG    << "\n";
    str << GLOBAL << BOOLTAG   << "\n";
    str << GLOBAL << STRINGTAG << "\n";

    str << INTTAG    << LABEL << WORD << intclasstag    << "\n";
    str << BOOLTAG   << LABEL << WORD << boolclasstag   << "\n";
    str << STRINGTAG << LABEL << WORD << stringclasstag << "\n";
}

void CgenClassTable::emit_gc_selector() {
    str << GLOBAL << "_MemMgr_INITIALIZER\n"
        << "_MemMgr_INITIALIZER:\n" << WORD << gc_init_names[cgen_Memmgr] << "\n";
    str << GLOBAL << "_MemMgr_COLLECTOR\n"
        << "_MemMgr_COLLECTOR:\n"   << WORD << gc_collect_names[cgen_Memmgr] << "\n";
    str << GLOBAL << "_MemMgr_TEST\n"
        << "_MemMgr_TEST:\n"        << WORD << (cgen_Memmgr_Test == GC_TEST) << "\n";
}

//********************************************************
//
// Emit code to reserve space for and initialize all of
// the constants.  Class names should have been added to
// the string table (in the supplied code, is is done
// during the construction of the inheritance graph), and
// code for emitting string constants as a side effect adds
// the string's length to the integer table.  The constants
// are emmitted by running through the stringtable and inttable
// and producing code for each entry.
//
//********************************************************
void CgenClassTable::emit_constant_objects() {
    stringtable.add_string("");
    inttable.add_string("0");
    stringtable.code_string_table(str, stringclasstag);
    inttable.code_string_table(str, intclasstag);
    falsebool.code_def(str, boolclasstag);
    truebool.code_def(str, boolclasstag);
}

void CgenClassTable::emit_name_table() {
    str << CLASSNAMETAB << LABEL;
    for (std::vector<CgenNode*>::iterator ni = m_ordered_nodes.begin(); ni != m_ordered_nodes.end(); ++ni) {
        CgenNode* nd = *ni;
        StringEntry* se = stringtable.lookup_string(nd->name->get_string());
        str << WORD; se->code_ref(str); str << "\n";
    }
}

void CgenClassTable::emit_obj_table() {
    str << CLASSOBJTAB << LABEL;
    for (std::vector<CgenNode*>::iterator ni = m_ordered_nodes.begin(); ni != m_ordered_nodes.end(); ++ni) {
        CgenNode* nd = *ni;
        str << WORD << protobj_name(nd->name)  << "\n";
        str << WORD << initfn_name(nd->name)   << "\n";
    }
}

void CgenClassTable::emit_dispatch_tables() {
    for (std::vector<CgenNode*>::iterator ni = m_ordered_nodes.begin(); ni != m_ordered_nodes.end(); ++ni)
        (*ni)->emit_disptab(str);
}

void CgenClassTable::emit_prototype_objects() {
    for (std::vector<CgenNode*>::iterator ni = m_ordered_nodes.begin(); ni != m_ordered_nodes.end(); ++ni)
        (*ni)->emit_protobj(str);
}

void CgenClassTable::emit_text_preamble() {
    str << GLOBAL << HEAP_START << "\n"
        << HEAP_START << LABEL << WORD << 0 << "\n"
        << "\t.text\n";
    str << GLOBAL; emit_init_ref(idtable.add_string("Main"),   str); str << "\n";
    str << GLOBAL; emit_init_ref(idtable.add_string("Int"),    str); str << "\n";
    str << GLOBAL; emit_init_ref(idtable.add_string("String"), str); str << "\n";
    str << GLOBAL; emit_init_ref(idtable.add_string("Bool"),   str); str << "\n";
    str << GLOBAL;
    emit_method_ref(idtable.add_string("Main"), idtable.add_string("main"), str);
    str << "\n";
}

void CgenClassTable::emit_init_ref(Symbol s, ostream& os) {
    os << s << CLASSINIT_SUFFIX;
}
void CgenClassTable::emit_method_ref(Symbol cls, Symbol meth, ostream& os) {
    os << cls << METHOD_SEP << meth;
}
void CgenClassTable::emit_protobj(Symbol s, ostream& os) {
    os << s << PROTOBJ_SUFFIX;
}

void CgenClassTable::emit_initializers() {
    for (std::vector<CgenNode*>::iterator ni = m_ordered_nodes.begin(); ni != m_ordered_nodes.end(); ++ni)
        (*ni)->emit_init(str);
}

void CgenClassTable::emit_all_methods() {
    for (std::vector<CgenNode*>::iterator ni = m_ordered_nodes.begin(); ni != m_ordered_nodes.end(); ++ni)
        if (!(*ni)->basic()) (*ni)->emit_methods(str);
}

CgenNodeP CgenClassTable::root() { return probe(Object); }

void CgenClassTable::Execute() { code(); exitscope(); }

void CgenClassTable::install_class(CgenNodeP nd) {
    Symbol n = nd->get_name();
    if (probe(n)) return;
    nds = new List<CgenNode>(nd, nds);
    addid(n, nd);
}

void CgenClassTable::install_classes(Classes cs) {
    for (int i = cs->first(); cs->more(i); i = cs->next(i))
        install_class(new CgenNode(cs->nth(i), NotBasic, this));
}

void CgenClassTable::build_inheritance_tree() {
    for (List<CgenNode>* l = nds; l; l = l->tl()) {
        CgenNode* nd  = l->hd();
        CgenNode* par = probe(nd->get_parent());
        nd->set_parentnd(par);
        par->add_child(nd);
    }
}

void CgenClassTable::install_basic_classes() {
    Symbol fname = stringtable.add_string("<basic class>");

    addid(No_class,  new CgenNode(class_(No_class,  No_class, nil_Features(), fname), Basic, this));
    addid(SELF_TYPE, new CgenNode(class_(SELF_TYPE, No_class, nil_Features(), fname), Basic, this));
    addid(prim_slot, new CgenNode(class_(prim_slot, No_class, nil_Features(), fname), Basic, this));

    install_class(new CgenNode(
        class_(Object, No_class,
            append_Features(append_Features(
                single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
                single_Features(method(type_name,  nil_Formals(), Str,    no_expr()))),
                single_Features(method(copy,       nil_Formals(), SELF_TYPE, no_expr()))),
            fname), Basic, this));

    install_class(new CgenNode(
        class_(IO, Object,
            append_Features(append_Features(append_Features(
                single_Features(method(out_string, single_Formals(formal(arg, Str)),
                                       SELF_TYPE, no_expr())),
                single_Features(method(out_int,    single_Formals(formal(arg, Int)),
                                       SELF_TYPE, no_expr()))),
                single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
                single_Features(method(in_int,    nil_Formals(), Int, no_expr()))),
            fname), Basic, this));

    install_class(new CgenNode(
        class_(Int, Object,
            single_Features(attr(val, prim_slot, no_expr())),
            fname), Basic, this));

    install_class(new CgenNode(
        class_(Bool, Object,
            single_Features(attr(val, prim_slot, no_expr())),
            fname), Basic, this));

    install_class(new CgenNode(
        class_(Str, Object,
            append_Features(append_Features(append_Features(append_Features(
                single_Features(attr(val, Int, no_expr())),
                single_Features(attr(str_field, prim_slot, no_expr()))),
                single_Features(method(length, nil_Formals(), Int, no_expr()))),
                single_Features(method(concat, single_Formals(formal(arg, Str)), Str, no_expr()))),
                single_Features(method(substr,
                    append_Formals(single_Formals(formal(arg, Int)),
                                   single_Formals(formal(arg2, Int))),
                    Str, no_expr()))),
            fname), Basic, this));
}

//*********************************************************
//
// Define method for code generation
//
// This is the method called by the compiler driver
// `cgtest.cc'. cgen takes an `ostream' to which the assembly will be
// emmitted, and it passes this and the class list of the
// code generator tree to the constructor for `CgenClassTable'.
// That constructor performs all of the work of the code
// generator.
//
//*********************************************************
void program_class::cgen(ostream& os) {
    os << "# start of generated code\n";
    initialize_constants();
    g_table = new CgenClassTable(classes, os);
    g_table->Execute();
    os << "# end of generated code\n";
}

//******************************************************************
//
//   Fill in the following methods to produce code for the
//   appropriate expression.  You may add or remove parameters
//   as you wish, but if you do, remember to change the parameters
//   of the declarations in `cool-tree.h'  Sample code for
//   constant integers, strings, and booleans are provided.
//
//*****************************************************************
void assign_class::code(ostream& s, FrameTracker ft) {

    expr->code(s, ft);

    int idx;
    if ((idx = ft.lookup_local(name)) != -1) {

        xstore(ACC, idx, SP, s);
        if (cgen_Memmgr == 1) { xaddiu(A1, SP, idx*4, s); xjal("_GenGC_Assign", s); }
    } else if ((idx = ft.lookup_param(name)) != -1) {

        xstore(ACC, idx, FP, s);
        if (cgen_Memmgr == 1) { xaddiu(A1, FP, idx*4, s); xjal("_GenGC_Assign", s); }
    } else if ((idx = ft.lookup_attrib(name)) != -1) {

        xstore(ACC, DEFAULT_OBJFIELDS + idx, SELF, s);
        if (cgen_Memmgr == 1) {
            xaddiu(A1, SELF, (DEFAULT_OBJFIELDS+idx)*4, s);
            xjal("_GenGC_Assign", s);
        }
    }

}

void static_dispatch_class::code(ostream& s, FrameTracker ft) {

    for (int i = actual->first(); actual->more(i); i = actual->next(i)) {
        actual->nth(i)->code(s, ft);
        push_acc(s);
        ft.push_var(No_class);
    }

    expr->code(s, ft);

    int ok_label = g_label++;
    xbne(ACC, ZERO, ok_label, s);
    xla(ACC, "str_const0", s);
    xli(T1,  get_line_number(), s);
    xjal("_dispatch_abort", s);
    xlabel(ok_label, s);

    CgenNode* target_node = g_table->lookup_class(type_name);
    target_node->build_dispatch_layout();
    int slot = target_node->m_dispatch_idx.at(name);

    std::string dtab = disptab_name(type_name);
    xla(T1, dtab.c_str(), s);
    xload(T1, slot, T1, s);
    xjalr(T1, s);
}

void dispatch_class::code(ostream& s, FrameTracker ft) {

    for (int i = actual->first(); actual->more(i); i = actual->next(i)) {
        actual->nth(i)->code(s, ft);
        push_acc(s);
        ft.push_var(No_class);
    }

    expr->code(s, ft);

    int ok_label = g_label++;
    xbne(ACC, ZERO, ok_label, s);
    xla(ACC, "str_const0", s);
    xli(T1, get_line_number(), s);
    xjal("_dispatch_abort", s);
    xlabel(ok_label, s);

    Symbol static_type = expr->get_type();
    if (static_type == SELF_TYPE) static_type = ft.class_node->name;

    CgenNode* target_node = g_table->lookup_class(static_type);
    target_node->build_dispatch_layout();
    int slot = target_node->m_dispatch_idx.at(name);

    xload(T1, DISPTABLE_OFFSET, ACC, s);
    xload(T1, slot, T1, s);
    xjalr(T1, s);
}

void cond_class::code(ostream& s, FrameTracker ft) {
    int lbl_else   = g_label++;
    int lbl_finish = g_label++;

    pred->code(s, ft);
    unbox_int(T1, ACC, s);
    xbeq(T1, ZERO, lbl_else, s);

    then_exp->code(s, ft);
    xbranch(lbl_finish, s);

    xlabel(lbl_else, s);
    else_exp->code(s, ft);

    xlabel(lbl_finish, s);
}

void loop_class::code(ostream& s, FrameTracker ft) {
    int lbl_top    = g_label++;
    int lbl_finish = g_label++;

    xlabel(lbl_top, s);
    pred->code(s, ft);
    unbox_int(T1, ACC, s);
    xbeq(T1, ZERO, lbl_finish, s);

    body->code(s, ft);
    xbranch(lbl_top, s);

    xlabel(lbl_finish, s);
    xmove(ACC, ZERO, s);
}

void block_class::code(ostream& s, FrameTracker ft) {
    for (int i = body->first(); body->more(i); i = body->next(i))
        body->nth(i)->code(s, ft);
}

void let_class::code(ostream& s, FrameTracker ft) {

    init->code(s, ft);
    if (init->IsEmpty()) {
        if      (type_decl == Int)  load_int_const(ACC, inttable.lookup_string("0"), s);
        else if (type_decl == Bool) load_bool_const(ACC, falsebool, s);
        else if (type_decl == Str)  load_string_const(ACC, stringtable.lookup_string(""), s);

    }

    push_acc(s);
    ft.push_var(identifier);

    body->code(s, ft);

    discard(1, s);
    ft.pop_vars(1);
}

void typcase_class::code(ostream& s, FrameTracker ft) {
    expr->code(s, ft);

    int not_void_lbl = g_label++;
    xbne(ACC, ZERO, not_void_lbl, s);
    xla(ACC, "str_const0", s);
    xli(T1, get_line_number(), s);
    xjal("_case_abort2", s);
    xlabel(not_void_lbl, s);

    push_acc(s);
    ft.push_var(No_class);

    xload(T1, TAG_OFFSET, ACC, s);

    std::vector<branch_class*> branches;
    for (int i = cases->first(); cases->more(i); i = cases->next(i))
        branches.push_back(static_cast<branch_class*>(cases->nth(i)));

    int num_branches = (int)branches.size();
    int base_label   = g_label;
    g_label += num_branches + 1;
    int finish_label = base_label + num_branches;

    std::vector<int> branch_order;
    for (int i = 0; i < num_branches; ++i) branch_order.push_back(i);
    for (int i = 0; i < num_branches; ++i) {
        for (int j = i + 1; j < num_branches; ++j) {
            int ti = g_table->m_class_tags[branches[branch_order[i]]->type_decl];
            int tj = g_table->m_class_tags[branches[branch_order[j]]->type_decl];
            if (tj > ti) {
                int tmp = branch_order[i];
                branch_order[i] = branch_order[j];
                branch_order[j] = tmp;
            }
        }
    }

    for (std::vector<int>::iterator bo = branch_order.begin(); bo != branch_order.end(); ++bo) {
        int bi = *bo;
        std::vector<int> tags;
        std::vector<CgenNode*> worklist;
        CgenNode* root_nd = g_table->lookup_class(branches[bi]->type_decl);
        if (root_nd) worklist.push_back(root_nd);
        while (!worklist.empty()) {
            CgenNode* cur = worklist.back(); worklist.pop_back();
            tags.push_back(cur->class_tag);
            for (List<CgenNode>* l = cur->get_children(); l; l = l->tl())
                worklist.push_back(l->hd());
        }
        for (std::vector<int>::iterator ti = tags.begin(); ti != tags.end(); ++ti) {
            xli(T2, *ti, s);
            xbeq(T1, T2, base_label + bi, s);
        }
    }

    xjal("_case_abort", s);
    xbranch(finish_label, s);

    for (int bi = 0; bi < num_branches; ++bi) {
        xlabel(base_label + bi, s);
        branch_class* br = branches[bi];

        xload(ACC, 1, SP, s);
        push_acc(s);
        ft.push_var(br->name);
        br->expr->code(s, ft);
        discard(1, s);
        ft.pop_vars(1);
        xbranch(finish_label, s);
    }

    xlabel(finish_label, s);

    discard(1, s);
    ft.pop_vars(1);
}

static void binop_setup(Expression e1, Expression e2, ostream& s, FrameTracker& ft) {
    e1->code(s, ft);
    push_acc(s);
    ft.push_var(No_class);

    e2->code(s, ft);
    xjal("Object.copy", s);

    xaddiu(SP, SP, 4, s);
    xload(T1, 0, SP, s);
    xmove(T2, ACC, s);
    ft.pop_vars(1);

    unbox_int(T1, T1, s);
    unbox_int(T2, T2, s);
}

void plus_class::code(ostream& s, FrameTracker ft) {
    binop_setup(e1, e2, s, ft);
    xadd(T3, T1, T2, s);
    rebox_int(T3, ACC, s);
}

void sub_class::code(ostream& s, FrameTracker ft) {
    binop_setup(e1, e2, s, ft);
    xsub(T3, T1, T2, s);
    rebox_int(T3, ACC, s);
}

void mul_class::code(ostream& s, FrameTracker ft) {
    binop_setup(e1, e2, s, ft);
    xmul(T3, T1, T2, s);
    rebox_int(T3, ACC, s);
}

void divide_class::code(ostream& s, FrameTracker ft) {
    binop_setup(e1, e2, s, ft);
    xdiv(T3, T1, T2, s);
    rebox_int(T3, ACC, s);
}

void neg_class::code(ostream& s, FrameTracker ft) {
    e1->code(s, ft);
    xjal("Object.copy", s);
    unbox_int(T1, ACC, s);
    xneg(T1, T1, s);
    rebox_int(T1, ACC, s);
}

void lt_class::code(ostream& s, FrameTracker ft) {
    e1->code(s, ft);
    push_acc(s); ft.push_var(No_class);
    e2->code(s, ft);
    xaddiu(SP, SP, 4, s); xload(T1, 0, SP, s); xmove(T2, ACC, s);
    ft.pop_vars(1);
    unbox_int(T1, T1, s); unbox_int(T2, T2, s);

    int yes_lbl = g_label++;
    load_bool_const(ACC, truebool,  s);
    xblt(T1, T2, yes_lbl, s);
    load_bool_const(ACC, falsebool, s);
    xlabel(yes_lbl, s);
}

void leq_class::code(ostream& s, FrameTracker ft) {
    e1->code(s, ft);
    push_acc(s); ft.push_var(No_class);
    e2->code(s, ft);
    xaddiu(SP, SP, 4, s); xload(T1, 0, SP, s); xmove(T2, ACC, s);
    ft.pop_vars(1);
    unbox_int(T1, T1, s); unbox_int(T2, T2, s);

    int yes_lbl = g_label++;
    load_bool_const(ACC, truebool,  s);
    xble(T1, T2, yes_lbl, s);
    load_bool_const(ACC, falsebool, s);
    xlabel(yes_lbl, s);
}

void eq_class::code(ostream& s, FrameTracker ft) {
    e1->code(s, ft);
    push_acc(s); ft.push_var(No_class);
    e2->code(s, ft);
    xaddiu(SP, SP, 4, s); xload(T1, 0, SP, s); xmove(T2, ACC, s);
    ft.pop_vars(1);

    if (e1->get_type() == Int  || e1->get_type() == Str  || e1->get_type() == Bool ||
        e2->get_type() == Int  || e2->get_type() == Str  || e2->get_type() == Bool) {
        load_bool_const(ACC, truebool,  s);
        load_bool_const(A1,  falsebool, s);
        xjal("equality_test", s);
    } else {
        int eq_lbl = g_label++;
        load_bool_const(ACC, truebool,  s);
        xbeq(T1, T2, eq_lbl, s);
        load_bool_const(ACC, falsebool, s);
        xlabel(eq_lbl, s);
    }
}

void comp_class::code(ostream& s, FrameTracker ft) {

    e1->code(s, ft);
    unbox_int(T1, ACC, s);

    int done_lbl = g_label++;
    load_bool_const(ACC, truebool, s);
    xbeq(T1, ZERO, done_lbl, s);
    load_bool_const(ACC, falsebool, s);
    xlabel(done_lbl, s);
}

void int_const_class::code(ostream& s, FrameTracker ft) {
    load_int_const(ACC, inttable.lookup_string(token->get_string()), s);
}

void string_const_class::code(ostream& s, FrameTracker ft) {
    load_string_const(ACC, stringtable.lookup_string(token->get_string()), s);
}

void bool_const_class::code(ostream& s, FrameTracker ft) {
    load_bool_const(ACC, BoolConst(val), s);
}

void new__class::code(ostream& s, FrameTracker ft) {
    if (type_name == SELF_TYPE) {

        xla(T1, "class_objTab", s);
        xload(T2, TAG_OFFSET, SELF, s);
        xsll(T2, T2, LOG_WORD_SIZE + 1, s);
        s << ADD << T1 << " " << T1 << " " << T2 << "\n";
        push_reg(T1, s);
        xload(ACC, 0, T1, s);
        xjal("Object.copy", s);
        pop_into(T1, s);
        xload(T1, 1, T1, s);
        xjalr(T1, s);
    } else {
        std::string po = protobj_name(type_name);
        std::string in = initfn_name(type_name);
        xla(ACC, po.c_str(), s);
        xjal("Object.copy", s);
        xjal(in.c_str(), s);
    }
}

void isvoid_class::code(ostream& s, FrameTracker ft) {
    e1->code(s, ft);
    xmove(T1, ACC, s);
    int not_void_lbl = g_label++;
    load_bool_const(ACC, truebool, s);
    xbeq(T1, ZERO, not_void_lbl, s);
    load_bool_const(ACC, falsebool, s);
    xlabel(not_void_lbl, s);
}

void no_expr_class::code(ostream& s, FrameTracker ft) {
    xmove(ACC, ZERO, s);
}

void object_class::code(ostream& s, FrameTracker ft) {
    if (name == self) {
        xmove(ACC, SELF, s);
        return;
    }

    int idx;
    if ((idx = ft.lookup_local(name)) != -1) {
        xload(ACC, idx, SP, s);
    } else if ((idx = ft.lookup_param(name)) != -1) {
        xload(ACC, idx, FP, s);
    } else if ((idx = ft.lookup_attrib(name)) != -1) {
        xload(ACC, DEFAULT_OBJFIELDS + idx, SELF, s);
    } else {

        s << "# ERROR: unresolved identifier " << name << "\n";
    }
}