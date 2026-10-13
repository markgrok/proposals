#!/usr/bin/env python3
"""Writes ucd_tables.hpp: the Unicode 17 data an ECMA-262 `u`-mode regex needs.

General_Category (every value and alias of PropertyValueAliases.txt, groups included), Script and
Script_Extensions (every sc value and alias), the 53 binary properties ECMA-262 lists in its table
"Binary Unicode property aliases", and simple case folding (CaseFolding.txt C + S). Inputs: ucd/,
the Unicode 17.0.0 files (UnicodeData.txt, Scripts.txt, DerivedNormalizationProps.txt checked
against the SHA-256 that webcpp's idna pins).
"""
import collections
import re
import sys
from pathlib import Path

U = Path(__file__).parent / 'ucd'
MAXCP = 0x10FFFF


def lines(name):
    for raw in (U / name).read_text(encoding='utf-8').splitlines():
        line = raw.split('#', 1)[0].strip()
        if line:
            yield [f.strip() for f in line.split(';')]


def cps(field):
    if '..' in field:
        a, b = field.split('..')
        return int(a, 16), int(b, 16)
    return int(field, 16), int(field, 16)


# ECMA-262 (2025) table "Binary Unicode property aliases": canonical names.
BINARY = """ASCII ASCII_Hex_Digit Alphabetic Any Assigned Bidi_Control Bidi_Mirrored Case_Ignorable Cased
Changes_When_Casefolded Changes_When_Casemapped Changes_When_Lowercased Changes_When_NFKC_Casefolded
Changes_When_Titlecased Changes_When_Uppercased Dash Default_Ignorable_Code_Point Deprecated Diacritic
Emoji Emoji_Component Emoji_Modifier Emoji_Modifier_Base Emoji_Presentation Extended_Pictographic Extender
Grapheme_Base Grapheme_Extend Hex_Digit IDS_Binary_Operator IDS_Trinary_Operator ID_Continue ID_Start
Ideographic Join_Control Logical_Order_Exception Lowercase Math Noncharacter_Code_Point Pattern_Syntax
Pattern_White_Space Quotation_Mark Radical Regional_Indicator Sentence_Terminal Soft_Dotted
Terminal_Punctuation Unified_Ideograph Uppercase Variation_Selector White_Space XID_Continue XID_Start""".split()
assert len(BINARY) == 53, len(BINARY)

# Aliases of the binary properties, from PropertyAliases.txt.
prop_alias = {}
for f in lines('PropertyAliases.txt'):
    for a in f:
        prop_alias[a] = f
binary_names = {}
for i, name in enumerate(BINARY):
    names = {name}
    if name in prop_alias:
        names |= set(prop_alias[name])
    for n in names:
        binary_names[n] = i

# General_Category.
gc_leaf = []
gc_values = []  # (aliases, leaf list)
for f in lines('PropertyValueAliases.txt'):
    if f[0] != 'gc':
        continue
    gc_values.append(f[1:])
raw_gc = (U / 'PropertyValueAliases.txt').read_text(encoding='utf-8')
groups = {}
for m in re.finditer(r'^gc ; (\w+)\s*;.*# ([\w |]+)$', raw_gc, re.M):
    groups[m.group(1)] = [x.strip() for x in m.group(2).split('|')]
for v in gc_values:
    if v[0] not in groups:
        gc_leaf.append(v[0])
assert len(gc_leaf) == 30, gc_leaf
leaf_index = {v: i for i, v in enumerate(gc_leaf)}
gc_masks = []
gc_names = {}
for i, v in enumerate(gc_values):
    members = groups.get(v[0], [v[0]])
    gc_masks.append(sum(1 << leaf_index[x] for x in members))
    for a in v:
        gc_names[a] = i

gc = [leaf_index['Cn']] * (MAXCP + 1)
for f in lines('DerivedGeneralCategory.txt'):
    a, b = cps(f[0])
    for c in range(a, b + 1):
        gc[c] = leaf_index[f[1]]

# Script and Script_Extensions.
sc_values = [f[1:] for f in lines('PropertyValueAliases.txt') if f[0] == 'sc']
sc_index = {}
for i, v in enumerate(sc_values):
    for a in v:
        sc_index[a] = i
sc = [sc_index['Zzzz']] * (MAXCP + 1)
for f in lines('Scripts.txt'):
    a, b = cps(f[0])
    for c in range(a, b + 1):
        sc[c] = sc_index[f[1]]
scx = [None] * (MAXCP + 1)
for f in lines('ScriptExtensions.txt'):
    a, b = cps(f[0])
    s = tuple(sorted(sc_index[x] for x in f[1].split()))
    for c in range(a, b + 1):
        scx[c] = s

# Binary properties.
sources = ['PropList.txt', 'DerivedCoreProperties.txt', 'emoji-data.txt', 'DerivedBinaryProperties.txt',
           'DerivedNormalizationProps.txt']
bin_sets = [set() for _ in BINARY]
for src in sources:
    for f in lines(src):
        if len(f) != 2 or f[1] not in BINARY:
            continue
        a, b = cps(f[0])
        bin_sets[BINARY.index(f[1])].update(range(a, b + 1))
bin_sets[BINARY.index('ASCII')] = set(range(0x80))
bin_sets[BINARY.index('Any')] = set(range(MAXCP + 1))
bin_sets[BINARY.index('Assigned')] = {c for c in range(MAXCP + 1) if gc[c] != leaf_index['Cn']}
for i, s in enumerate(bin_sets):
    assert s, BINARY[i]


def ranges(s):
    out = []
    for c in sorted(s):
        if out and out[-1][1] == c - 1:
            out[-1][1] = c
        else:
            out.append([c, c])
    return out


def runs(values):
    out = []
    prev = object()
    for c, v in enumerate(values):
        if v != prev:
            out.append((c, v))
            prev = v
    return out


# Simple case folding: C and S.
fold = []
for f in lines('CaseFolding.txt'):
    if f[1] in ('C', 'S'):
        fold.append((int(f[0], 16), int(f[2], 16)))
fold.sort()

# Bits of the char_class_type: internal classes first, then gc, binary, sc, scx.
INTERNAL = ['w', 's', 'd', 'lower', 'upper', 'alpha', 'any']
BIT_GC = 8
BIT_BIN = BIT_GC + len(gc_values)
BIT_SC = BIT_BIN + len(BINARY)
BIT_SCX = BIT_SC + len(sc_values)
BITS = BIT_SCX + len(sc_values)
assert BITS <= 512, BITS

out = []
w = out.append
w('// Generated by gen_tables.py from the Unicode 17.0.0 UCD. Do not edit.')
w('#pragma once')
w('#include <cstdint>')
w('namespace ecma_ucd {')
w(f'inline constexpr unsigned bit_gc = {BIT_GC}, bit_bin = {BIT_BIN}, bit_sc = {BIT_SC}, bit_scx = {BIT_SCX}, bit_count = {BITS};')
w(f'inline constexpr unsigned gc_count = {len(gc_values)}, bin_count = {len(BINARY)}, sc_count = {len(sc_values)};')
w('inline constexpr std::uint32_t gc_masks[] = {' + ', '.join(f'0x{m:08X}u' for m in gc_masks) + '};')
w('struct run { std::uint32_t start : 24; std::uint32_t value : 8; };')
r = runs(gc)
w(f'inline constexpr run gc_runs[{len(r)}] = {{' + ', '.join(f'{{0x{a:X},{v}}}' for a, v in r) + '};')
r = runs(sc)
w(f'inline constexpr run sc_runs[{len(r)}] = {{' + ', '.join(f'{{0x{a:X},{v}}}' for a, v in r) + '};')
sets = sorted({s for s in scx if s is not None})
set_index = {s: i + 1 for i, s in enumerate(sets)}
flat = [0]
offs = [0, 0]
for s in sets:
    flat.extend(s)
    offs.append(len(flat))
r = runs([set_index[s] if s is not None else 0 for s in scx])
assert len(sets) + 1 < 2048
w('struct xrun { std::uint32_t start : 21; std::uint32_t set : 11; };')
w(f'inline constexpr xrun scx_runs[{len(r)}] = {{' + ', '.join(f'{{0x{a:X},{v}}}' for a, v in r) + '};')
w(f'inline constexpr std::uint16_t scx_set_offsets[{len(offs)}] = {{' + ', '.join(map(str, offs)) + '};')
w(f'inline constexpr std::uint8_t scx_set_scripts[{len(flat)}] = {{' + ', '.join(map(str, flat)) + '};')
flat = []
offs = [0]
for s in bin_sets:
    for a, b in ranges(s):
        flat.append((a, b))
    offs.append(len(flat))
for i, n in enumerate(BINARY):
    w(f'inline constexpr unsigned bin_{n} = {i};')
w('struct range { char32_t lo, hi; };')
w(f'inline constexpr range bin_ranges[{len(flat)}] = {{' + ', '.join(f'{{0x{a:X},0x{b:X}}}' for a, b in flat) + '};')
w(f'inline constexpr std::uint32_t bin_offsets[{len(offs)}] = {{' + ', '.join(map(str, offs)) + '};')
w('struct fold_pair { char32_t from, to; };')
w(f'inline constexpr fold_pair simple_fold[{len(fold)}] = {{' + ', '.join(f'{{0x{a:X},0x{b:X}}}' for a, b in fold) + '};')
orbit = collections.defaultdict(set)
for a, b in fold:
    orbit[b].add(a)
inv = sorted((b, a) for b, s in orbit.items() for a in s)
w(f'inline constexpr fold_pair fold_sources[{len(inv)}] = {{' + ', '.join(f'{{0x{a:X},0x{b:X}}}' for a, b in inv) + '};')
names = []
for n, i in gc_names.items():
    names.append((n, 'lone_or_gc', BIT_GC + i))
for n, i in binary_names.items():
    names.append((n, 'binary', BIT_BIN + i))
for n, i in sc_index.items():
    names.append((n, 'script', i))
names.sort()
w('enum class name_kind : std::uint8_t { lone_or_gc, binary, script };')
w('struct property_name { char const* name; name_kind kind; std::uint16_t value; };')
w(f'inline constexpr property_name property_names[{len(names)}] = {{')
for n, k, v in names:
    w(f'    {{"{n}", name_kind::{k}, {v}}},')
w('};')
w('}  // namespace ecma_ucd')
Path(sys.argv[1] if len(sys.argv) > 1 else 'ucd_tables.hpp').write_text('\n'.join(out) + '\n')
print(f'bits {BITS}; gc runs {len(runs(gc))}, sc runs {len(runs(sc))}, scx sets {len(sets)}, binary ranges {len(flat)}, '
      f'fold {len(fold)}, names {len(names)}')
