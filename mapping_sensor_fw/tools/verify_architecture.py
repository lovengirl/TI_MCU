"""Check source boundaries and CCS membership without executing target hardware."""
from pathlib import Path
import json
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
checks = []
def check(name, passed):
    checks.append({'check': name, 'pass': bool(passed)})
    if not passed:
        raise AssertionError(name)

sources = sorted((root / 'src').rglob('*.c'))
check('19 implementation C files', len(sources) == 19)
check('no object filename collisions', len({p.stem for p in sources}) == len(sources))
main = (root / 'src/main.c').read_text(encoding='utf-8')
check('main delegates initialization and loop', len(main.splitlines()) <= 12 and 'App_Init();' in main and 'App_Process();' in main)
check('legacy monoliths removed', not (root / 'src/sensor.c').exists() and not (root / 'src/platform_mspm0.c').exists())
members = {f.attrib['path'] for f in ET.parse(root / 'mapping_sensor.projectspec').findall('./project/file')}
for source in sources:
    check('CCS source ' + source.name, source.relative_to(root).as_posix() in members)
for group in ['app', 'comm', 'config', 'diagnostics', 'processing', 'sensor']:
    for source in (root / 'src' / group).glob('*.c'):
        text = source.read_text(encoding='utf-8')
        check('no direct TI hardware access ' + source.name,
              'DL_' not in text and 'ti_msp_dl_config.h' not in text and 'board_pins.h' not in text)
for group in ['comm', 'processing', 'config', 'sensor', 'diagnostics']:
    for source in (root / 'src' / group).glob('*.c'):
        text = source.read_text(encoding='utf-8')
        check('no application dependency ' + source.name, '#include "app.h"' not in text)
(root / 'build/architecture_checks.json').write_text(json.dumps(checks, indent=2), encoding='utf-8')
print(f'PASS: {len(checks)} architecture / CCS membership checks')
