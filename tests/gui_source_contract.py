from pathlib import Path
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parents[1]

def function(text, name):
    match = re.search(r'^\s*(?:static\s+)?(?:void|bool|HSV|Vec2)\s+' + re.escape(name) + r'\s*\(', text, re.M)
    if not match:
        raise ValueError(f'Missing function {name}')
    start = text.index('{', match.end())

    tokens = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', re.S)
    depth = 0
    for token in tokens.finditer(text, start):
        if token.group() == '{': depth += 1
        elif token.group() == '}':
            depth -= 1
            if depth == 0: return text[match.start():token.end()]
    raise ValueError(f'Unclosed function {name}')

def digest(body):
    body = re.sub(r'//[^\n]*|/\*.*?\*/', '', body, flags=re.S)
    body = body.replace('bool hovered = false, held = false;', 'bool hovered, held;')
    body = body.replace('Font->LegacySize', 'Font->FontSize')
    body = body.replace('obj.Text.c_str()', 'obj.Text')
    body = re.sub(r'\*current_key = std::clamp\(\*current_key, 0, 165\);', '', body)
    return hashlib.sha256(re.sub(r'\s+', '', body).encode()).hexdigest()

if __name__ == '__main__':
    manifest = json.loads((ROOT / 'tests/gui_source_contract.json').read_text())
    for filename, entries in manifest.items():
        source = (ROOT / 'src/ui/original' / filename).read_text(encoding='utf-8-sig')
        for name, expected in entries.items():
            actual = digest(function(source, name))
            assert actual == expected, f'Source GUI routine changed: {filename}: {name}'
    print(f'PASS: {sum(map(len, manifest.values()))} original GUI drawing/interaction routines match the supplied source.')
