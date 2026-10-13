#!/usr/bin/env python3
"""Writes a Boost-style test matrix from b2 --out-xml files.

Each input is the file `b2 --dump-tests --out-xml=FILE` wrote; one file per
toolset run, or one run with several toolsets. Rows are libraries and tests,
columns are toolsets as b2 names their directories (clang-darwin-21, gcc-15).
A cell is `pass`, the step that failed (`comp`, `link`, `run`), the step that
succeeded where the test expected a failure (`compiled`, `linked`, `ran`), or
`n/a` when b2 ran no action for it (not built, or up to date: run b2 with -a
or a fresh build directory). The exit status is 1 when any cell failed.

    python3 b2_matrix.py --title NAME --output matrix.html run1.xml run2.xml
"""
import argparse
import html
import xml.etree.ElementTree as ET

TYPE_CODES = {'RUN': 'r', 'RUN_FAIL': 'rf', 'COMPILE': 'c', 'COMPILE_FAIL': 'cf',
              'LINK': 'l', 'LINK_FAIL': 'lf', 'RUN_OUTPUT': 'r', 'UNIT_TEST': 'u'}
SUCCEEDED_INSTEAD = {'COMPILE_FAIL': 'compiled', 'LINK_FAIL': 'linked', 'RUN_FAIL': 'ran'}
SEVERITY = ['comp', 'link', 'compiled', 'linked', 'run', 'ran', 'fail']


def step(action_name):
    if '.compile' in action_name:
        return 'comp'
    if '.link' in action_name or '.archive' in action_name:
        return 'link'
    if 'capture-output' in action_name:
        return 'run'
    return 'fail'


def load(files):
    """Returns the tests in order, the toolsets in order, and the actions of
    every (test, toolset) pair."""
    tests, toolsets, actions = {}, [], {}
    for file in files:
        root = ET.parse(file).getroot()
        for t in root.iter('test'):
            location, _, target = t.findtext('target').partition('//')
            location = location.lstrip('./') if location not in ('.', '') else ''
            name = t.get('name')
            library, _, test = name.rpartition('/')
            key = '/%s/%s/' % (location, target) if location else '/%s/' % target
            tests.setdefault(key, {'library': library, 'test': test, 'type': t.get('type'),
                                   'source': t.findtext('source')})
        for a in root.iter('action'):
            path = a.findtext('path') or ''
            for key in tests:
                position = path.find(key)
                if position >= 0:
                    toolset = path[position + len(key):].split('/')[0]
                    if not toolset:
                        break
                    if toolset not in toolsets:
                        toolsets.append(toolset)
                    if a.findtext('name'):
                        actions.setdefault((key, toolset), []).append({
                            'name': a.findtext('name'), 'status': a.get('status'),
                            'command': (a.findtext('command') or '').strip(),
                            'output': (a.findtext('output') or '').strip()})
                    break
    return tests, toolsets, actions


def classify(test, steps):
    """Returns the cell's label and the action that explains a failure."""
    if not steps:
        return 'n/a', None
    if any(s['name'].startswith('testing%testing.expect-') and s['status'] == '0' for s in steps):
        return 'pass', None
    failed = [s for s in steps if s['status'] != '0']
    if failed:
        return step(failed[0]['name']), failed[0]
    return SUCCEEDED_INSTEAD.get(test['type'], 'fail'), steps[-1]


STYLE = """
:root { --pass: #1a7f37; --fail: #cf222e; --na: #6e7781; --bg: #ffffff; --fg: #1f2328; --line: #d0d7de; }
@media (prefers-color-scheme: dark) {
  :root { --pass: #3fb950; --fail: #f85149; --na: #8b949e; --bg: #0d1117; --fg: #e6edf3; --line: #30363d; }
}
body { background: var(--bg); color: var(--fg); font: 14px/1.4 system-ui, sans-serif; margin: 16px; }
table { border-collapse: collapse; margin: 8px 0 24px; }
th, td { border: 1px solid var(--line); padding: 3px 10px; text-align: center; }
th.row, td.row { text-align: left; }
.pass { color: var(--pass); } .fail { color: var(--fail); font-weight: 600; } .na { color: var(--na); }
pre { border: 1px solid var(--line); padding: 8px; overflow-x: auto; white-space: pre-wrap; }
"""


def render(title, tests, toolsets, actions):
    cells, details = {}, []
    for key, test in tests.items():
        for toolset in toolsets:
            label, why = classify(test, actions.get((key, toolset), []))
            anchor = None
            if why is not None:
                anchor = 'd%d' % len(details)
                details.append((anchor, '%s/%s on %s: %s' % (test['library'], test['test'], toolset, label), why))
            cells[(key, toolset)] = (label, anchor)

    def css(label):
        return 'pass' if label in ('pass', 'OK') else 'na' if label == 'n/a' else 'fail'

    out = ['<!doctype html><html><head><meta charset="utf-8">',
           '<meta name="viewport" content="width=device-width, initial-scale=1">',
           '<title>%s</title><style>%s</style></head><body>' % (html.escape(title), STYLE),
           '<h1>%s</h1>' % html.escape(title), '<h2>Summary</h2><table><tr><th class="row">library</th>']
    out += ['<th>%s</th>' % html.escape(t) for t in toolsets]
    libraries = sorted({t['library'] for t in tests.values()})
    for library in libraries:
        out.append('</tr><tr><td class="row"><a href="#%s">%s</a></td>' % (html.escape(library), html.escape(library)))
        for toolset in toolsets:
            labels = [cells[(k, toolset)][0] for k, t in tests.items() if t['library'] == library]
            failures = [l for l in labels if l not in ('pass', 'n/a')]
            label = 'n/a' if all(l == 'n/a' for l in labels) else \
                'OK' if not failures else min(failures, key=SEVERITY.index)
            out.append('<td class="%s">%s</td>' % (css(label), label))
    out.append('</tr></table>')
    for library in libraries:
        out.append('<h2 id="%s">%s</h2><table><tr><th class="row">test</th><th>type</th>' % (
            html.escape(library), html.escape(library)))
        out += ['<th>%s</th>' % html.escape(t) for t in toolsets]
        for key, test in sorted((kv for kv in tests.items() if kv[1]['library'] == library),
                                key=lambda kv: kv[1]['test']):
            out.append('</tr><tr><td class="row" title="%s">%s</td><td>%s</td>' % (
                html.escape(test['source'] or ''), html.escape(test['test']),
                TYPE_CODES.get(test['type'], test['type'].lower())))
            for toolset in toolsets:
                label, anchor = cells[(key, toolset)]
                text = '<a class="%s" href="#%s">%s</a>' % (css(label), anchor, label) if anchor else label
                out.append('<td class="%s">%s</td>' % (css(label), text))
        out.append('</tr></table>')
    out.append('<h2>Failures</h2>')
    for anchor, heading, why in details:
        out.append('<h3 id="%s">%s</h3><pre>%s</pre><details><summary>command</summary><pre>%s</pre></details>' % (
            anchor, html.escape(heading), html.escape(why['output'] or '(no output)'),
            html.escape(why['command'])))
    out.append('<p>pass: the test passed. comp, link, run: that step failed. compiled, linked, ran: '
               'that step succeeded although the test expects it to fail. n/a: b2 ran nothing for it.</p>')
    out.append('</body></html>')
    failed = sum(1 for label, _ in cells.values() if label not in ('pass', 'n/a'))
    return '\n'.join(out), cells, failed


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--title', default='Test matrix')
    parser.add_argument('--output', required=True)
    parser.add_argument('files', nargs='+')
    args = parser.parse_args()
    tests, toolsets, actions = load(args.files)
    page, cells, failed = render(args.title, tests, toolsets, actions)
    with open(args.output, 'w', encoding='utf-8') as f:
        f.write(page)
    width = max(len(t['library'] + t['test']) + 1 for t in tests.values())
    print(' ' * width, ' '.join('%-18s' % t for t in toolsets))
    for key, test in sorted(tests.items(), key=lambda kv: (kv[1]['library'], kv[1]['test'])):
        print(('%s/%s' % (test['library'], test['test'])).ljust(width),
              ' '.join('%-18s' % cells[(key, t)][0] for t in toolsets))
    return 1 if failed else 0


if __name__ == '__main__':
    raise SystemExit(main())
