"""Counts the declarations under include/stately/ by doc-comment coverage, from clang's JSON AST.

Companion of MRDOCS_FOR_WEBCPP.md. Usage, from a directory holding a copy of include/ and a TU
src/all.cpp that includes every header under include/stately/:

    env -u CPATH clang++ -std=c++20 -fsyntax-only -isystem <xstate-cpp>/deps/boost -Iinclude \
        -Xclang -ast-dump=json -Xclang -ast-dump-filter=stately src/all.cpp > ast.jsonl
    python3 MRDOCS_FOR_WEBCPP_COUNT_DOCS.py ast.jsonl [--list]

A declaration counts as documented when clang attached a doc comment to it (a FullComment node), the
attachment MrDocs reads too. Public means at namespace scope, or a public member of a public class.
A declaration under any namespace named detail is counted apart. Out-of-line definitions merge into
the declaration they define. Implicit declarations are skipped.
"""
import json, sys, collections

FUNCS = {'FunctionDecl', 'CXXMethodDecl', 'CXXConstructorDecl', 'CXXDestructorDecl', 'CXXConversionDecl',
         'CXXDeductionGuideDecl'}
RECORD = 'CXXRecordDecl'
ALIASES = {'TypedefDecl', 'TypeAliasDecl', 'TypeAliasTemplateDecl'}


def load(path):
    text = open(path).read()
    dec = json.JSONDecoder()
    i, out = 0, []
    while i < len(text):
        j = text.find('{', i)
        if j < 0:
            break
        obj, i = dec.raw_decode(text, j)
        out.append(obj)
    return out


def comment(node):
    for c in node.get('inner', []):
        if c.get('kind') == 'FullComment':
            return c
    return None


def walk_comment(c):
    yield c
    for k in c.get('inner', []):
        yield from walk_comment(k)


def file_of(node, last):
    f = node.get('loc', {}).get('file') or node.get('loc', {}).get('spellingLoc', {}).get('file')
    return f or last


rows = collections.Counter()
seen = set()
examples = collections.defaultdict(list)


def record(area, kind, what, name):
    rows[(area, kind, what)] += 1
    if len(examples[(area, kind, what)]) < 400:
        examples[(area, kind, what)].append(name)


functions = {}
record_names = {}


def count_function(fn, tmpl, path, area, friend, f):
    """Collects a function; redeclarations (an out-of-line definition) merge into one entry."""
    owner = record_names.get(fn.get('parentDeclContextId')) if fn.get('previousDecl') else None
    name = '::'.join((owner.split('::') if owner else path) + [fn.get('name', '?')])
    key = (name, fn.get('type', {}).get('qualType'), friend, area)
    entry = functions.setdefault(key, {'kind': fn['kind'], 'comments': [], 'params': None})
    fc = comment(fn) or (comment(tmpl) if tmpl else None)
    if fc:
        entry['comments'].append(fc)
    params = [p for p in fn.get('inner', []) if p.get('kind') == 'ParmVarDecl']
    if entry['params'] is None or (fc and params):
        entry['params'] = params


def tally_functions():
    for (name, qual_type, friend, area), entry in functions.items():
        kind = 'friend function' if friend else 'function'
        record(area, kind, 'total', name)
        params = entry['params'] or []
        named = [p['name'] for p in params if p.get('name')]
        if not entry['comments']:
            record(area, kind, 'no doc comment', name)
            if params:
                record(area, kind, 'no doc comment, has parameters', name)
            continue
        documented_params, returns = set(), False
        for fc in entry['comments']:
            for c in walk_comment(fc):
                if c.get('kind') == 'ParamCommandComment':
                    documented_params.add(c.get('param'))
                if c.get('kind') == 'BlockCommandComment' and c.get('name') in ('return', 'returns', 'result'):
                    returns = True
        if params:
            record(area, kind, 'documented, has parameters', name)
            if any(n not in documented_params for n in named) or len(named) < len(params):
                record(area, kind, 'documented, a parameter without @param', name)
            for n in named:
                if n not in documented_params:
                    record(area, kind, 'parameters without @param (each)', name + '/' + n)
        rt = qual_type or ''
        if entry['kind'] in ('FunctionDecl', 'CXXMethodDecl') and not rt.startswith('void ') and not rt.startswith('void('):
            record(area, kind, 'documented, non-void', name)
            if not returns:
                record(area, kind, 'documented, non-void, no @returns', name)


def visit(node, path, area, public, f):
    k = node.get('kind')
    f = file_of(node, f)
    if node.get('isImplicit'):
        return
    if f and not f.startswith('include/stately/'):
        return
    if k == 'NamespaceDecl':
        name = node.get('name', '')
        sub_area = 'detail' if (area == 'detail' or name == 'detail') else area
        for c in node.get('inner', []):
            visit(c, path + [name], sub_area, True, f)
        return
    if k == 'LinkageSpecDecl':
        for c in node.get('inner', []):
            visit(c, path, area, public, f)
        return
    access = node.get('access')
    if access in ('private', 'protected') or not public:
        return
    if k in FUNCS:
        count_function(node, None, path, area, False, f)
    elif k == 'FunctionTemplateDecl':
        inner = [c for c in node.get('inner', []) if c.get('kind') in FUNCS]
        if inner:
            count_function(inner[0], node, path, area, False, f)
    elif k == 'FriendDecl':
        for c in node.get('inner', []):
            if c.get('kind') in FUNCS:
                count_function(c, None, path, area, True, f)
            elif c.get('kind') == 'FunctionTemplateDecl':
                inner = [d for d in c.get('inner', []) if d.get('kind') in FUNCS]
                if inner:
                    count_function(inner[0], c, path, area, True, f)
    elif k in (RECORD, 'ClassTemplateDecl', 'ClassTemplatePartialSpecializationDecl'):
        rec = node
        tmpl = None
        if k == 'ClassTemplateDecl':
            recs = [c for c in node.get('inner', []) if c.get('kind') == RECORD]
            if not recs:
                return
            rec, tmpl = recs[0], node
        if not rec.get('completeDefinition'):
            return
        tag = rec.get('tagUsed', 'class')
        name = '::'.join(path + [rec.get('name', '(anonymous)')])
        record_names[rec.get('id')] = name
        if (name, tag, f) in seen:
            return
        seen.add((name, tag, f))
        kind = {'struct': 'struct', 'class': 'class', 'union': 'union'}.get(tag, tag)
        record(area, kind, 'total', name)
        if not (comment(rec) or (comment(tmpl) if tmpl else None)):
            record(area, kind, 'no doc comment', name)
        default_public = tag != 'class'
        current = default_public
        for c in rec.get('inner', []):
            if c.get('kind') == 'AccessSpecDecl':
                current = c.get('access') == 'public'
                continue
            visit(c, path + [rec.get('name', '')], area, current if 'access' not in c else c['access'] == 'public', f)
    elif k == 'EnumDecl':
        name = '::'.join(path + [node.get('name', '(anonymous)')])
        record(area, 'enum', 'total', name)
        if not comment(node):
            record(area, 'enum', 'no doc comment', name)
        for c in node.get('inner', []):
            if c.get('kind') == 'EnumConstantDecl':
                cname = name + '::' + c.get('name', '')
                record(area, 'enumerator', 'total', cname)
                if not comment(c):
                    record(area, 'enumerator', 'no doc comment', cname)
    elif k in ALIASES:
        name = '::'.join(path + [node.get('name', '')])
        record(area, 'alias', 'total', name)
        if not comment(node):
            record(area, 'alias', 'no doc comment', name)
    elif k == 'ConceptDecl':
        name = '::'.join(path + [node.get('name', '')])
        record(area, 'concept', 'total', name)
        if not comment(node):
            record(area, 'concept', 'no doc comment', name)
    elif k in ('FieldDecl', 'VarDecl', 'VarTemplateDecl'):
        name = '::'.join(path + [node.get('name', '')])
        record(area, 'variable', 'total', name)
        if not comment(node):
            record(area, 'variable', 'no doc comment', name)
    elif k == 'UsingDecl':
        name = '::'.join(path + [node.get('name', '')])
        record(area, 'using-declaration', 'total', name)
        if not comment(node):
            record(area, 'using-declaration', 'no doc comment', name)


for top in load(sys.argv[1]):
    if top.get('kind') == 'NamespaceDecl' and top.get('name') == 'stately':
        visit(top, [], 'public', True, None)

tally_functions()
order = ['function', 'friend function', 'class', 'struct', 'union', 'enum', 'enumerator', 'alias', 'concept',
         'using-declaration', 'variable']
for area in ('public', 'detail'):
    print(f'== {area}')
    for kind in order:
        items = {w: v for (a, k, w), v in rows.items() if a == area and k == kind}
        if items:
            print(f'  {kind:18s} ' + ', '.join(f'{w}: {items[w]}' for w in sorted(items, key=lambda w: (w != "total", w))))
if len(sys.argv) > 2:
    for key in sorted(examples):
        if key[2] != 'total':
            print(key, examples[key][:400])
