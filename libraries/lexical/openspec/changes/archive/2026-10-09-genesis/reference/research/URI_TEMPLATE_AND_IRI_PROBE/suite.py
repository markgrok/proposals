#!/usr/bin/env python3
"""Runs a harness against the suite's iri, iri-reference and uri-template tests, plus extra probes."""
import json, os, subprocess, sys, collections
SUITE = '~/.claude/jobs/fcf3a902/tmp/jsonschema-research/JSON-Schema-Test-Suite/tests'
DRAFTS = ['draft6', 'draft7', 'draft2019-09', 'draft2020-12', 'v1']
FORMATS = ['iri', 'iri-reference', 'uri-template']
B = lambda s: s.encode('utf-8')
# Extra probes: (format, bytes, expected, note). Expected values follow the RFC ABNF unless the note says otherwise.
EXTRA = [
 ('iri', B('http://1.2.3.4.example.com/'), True, 'reg-name that starts like an IPv4'),
 ('iri', B('http://example.com:8o/'), False, 'non-digit port'),
 ('iri', B('http://ex ample.com/'), False, 'space in host'),
 ('iri', B('http://a/\u200e'), False, 'LRM: RFC 3987 4.1 MUST NOT (ABNF admits it)'),
 ('iri', B('http://a/\ufffd'), False, 'U+FFFD is outside ucschar'),
 ('iri', B('http://a/\u0080'), False, 'C1 control is outside ucschar'),
 ('iri', B('http://a/\u00a0'), True, 'U+00A0 is the first ucschar'),
 ('iri', B('http://a/\ue000'), False, 'iprivate in a path'),
 ('iri', B('http://a/?\ue000'), True, 'iprivate in a query'),
 ('iri', B('http://a/#\ue000'), False, 'iprivate in a fragment'),
 ('iri', B('http://[v1.x]/'), True, 'IPvFuture'),
 ('iri', B('http://[::1%25eth0]/'), False, 'zone id is not RFC 3986/3987'),
 ('iri', b'http://a/\xc3', False, 'truncated UTF-8'),
 ('iri', b'http://a/\xc0\xaf', False, 'overlong UTF-8'),
 ('iri', b'http://a/\xed\xa0\x80', False, 'UTF-8-encoded surrogate'),
 ('iri', B('http://\u0192\u00f8\u00f8.\u00df\u00e5r:/'), True, 'empty port'),
 ('iri', B('HTTP://a'), True, 'upper-case scheme'),
 ('iri', B('1http://a'), False, 'scheme starts with a digit'),
 ('iri', B('a:'), True, 'scheme and empty path'),
 ('iri', B('http://a:b@c:d@e/'), False, 'two @ in authority'),
 ('iri-reference', B('//a@b@c/'), False, 'two @ in authority'),
 ('iri-reference', B('//a:abc/p'), False, 'non-digit port'),
 ('iri-reference', B(''), True, 'empty reference'),
 ('iri-reference', B('./\u00e9:x'), True, 'colon after a dot segment'),
 ('iri-reference', B('\u00e9:x'), False, 'colon in the first segment of a relative path'),
 ('iri-reference', B('#\ue000'), False, 'iprivate in a fragment'),
 ('iri-reference', B('a b'), False, 'space'),
 ('uri-template', B('{'), False, 'lone opening brace'),
 ('uri-template', B('}'), False, 'lone closing brace'),
 ('uri-template', B('{{var}}'), False, 'nested braces'),
 ('uri-template', B('{v:9999}'), True, 'largest prefix'),
 ('uri-template', B('{v*:3}'), False, 'explode then prefix'),
 ('uri-template', B('{a b}'), False, 'space in a varname'),
 ('uri-template', B('{_a}'), True, 'underscore varname'),
 ('uri-template', B('{a.}'), False, 'trailing dot in a varname'),
 ('uri-template', B('{..a}'), False, 'operator then dot'),
 ('uri-template', B('{-var}'), False, 'hyphen is neither operator nor varchar'),
 ('uri-template', B('{\u00e4}'), False, 'non-ASCII varname'),
 ('uri-template', B('a<b'), False, '< in a literal'),
 ('uri-template', B('a"b'), False, '" in a literal'),
 ('uri-template', B('a^b'), False, '^ in a literal'),
 ('uri-template', B('a|b'), False, '| in a literal'),
 ('uri-template', B('a\\b'), False, 'backslash in a literal'),
 ('uri-template', B('a`b'), False, 'backtick in a literal'),
 ('uri-template', B('a\u0080b'), False, 'C1 control in a literal'),
 ('uri-template', B('a\ufffdb'), False, 'U+FFFD in a literal'),
 ('uri-template', b'a\xc3', False, 'truncated UTF-8 in a literal'),
 ('uri-template', B('{v:5}{w*}/\u00e9{?x,y}'), True, 'mixed'),
 ('uri-template', B('{v:}'), False, 'empty prefix'),
]
binary = sys.argv[1].split()
tests = []
for d in DRAFTS:
    for f in FORMATS:
        path = os.path.join(SUITE, d, 'format' if d == 'v1' else 'optional/format', f + '.json')
        if not os.path.exists(path):
            continue
        for g in json.load(open(path, encoding='utf-8')):
            for t in g['tests']:
                if isinstance(t['data'], str):
                    tests.append(dict(draft=d, format=f, test=t['description'], data=t['data'].encode('utf-8', 'surrogatepass'), expected=t['valid']))
for f, data, exp, note in EXTRA:
    tests.append(dict(draft='extra', format=f, test=note, data=data, expected=exp))
inp = ''.join(f"{t['format']}\t{t['data'].hex()}\n" for t in tests)
out = subprocess.run(binary, input=inp, capture_output=True, text=True, check=True).stdout.split()
assert len(out) == len(tests), (len(out), len(tests))
for t, r in zip(tests, out):
    t['verdict'] = 'unsupported' if r == 'N' else 'passed' if (r == '1') == t['expected'] else 'failed'
print('| set | format | total | passed | failed |')
print('|---|---|---|---|---|')
for d in ['v1 (= draft7, 2019-09, 2020-12)', 'extra']:
    key = 'v1' if d.startswith('v1') else d
    for f in FORMATS:
        ts = [t for t in tests if t['draft'] == key and t['format'] == f and t['verdict'] != 'unsupported']
        if not ts: continue
        c = collections.Counter(t['verdict'] for t in ts)
        print(f"| {d} | {f} | {len(ts)} | {c['passed']} | {c['failed']} |")
print('\nFailing:\n')
seen = collections.OrderedDict()
for t in tests:
    if t['verdict'] == 'failed':
        seen.setdefault((t['format'], t['test'], repr(t['data'])[2:-1], t['expected']), []).append(t['draft'])
for (f, tst, data, exp), ds in seen.items():
    print(f"- {f}: {tst}: `{data}` expected {'valid' if exp else 'invalid'} [{', '.join(ds)}]")
