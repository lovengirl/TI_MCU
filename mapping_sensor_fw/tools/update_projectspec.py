"""Keep CCS source membership synchronized with the modular source tree."""
from pathlib import Path
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
path = root / 'mapping_sensor.projectspec'
tree = ET.parse(path)
project = tree.getroot().find('project')
for item in list(project.findall('file')):
    if item.attrib['path'].startswith(('src/', 'include/')):
        project.remove(item)
for folder, suffix in [('src', '*.c'), ('include', '*.h')]:
    for file in sorted((root / folder).rglob(suffix)):
        relative = file.relative_to(root)
        ET.SubElement(project, 'file', {
            'path': relative.as_posix(), 'action': 'copy',
            'targetDirectory': relative.parent.as_posix()
        })
ET.indent(tree, space='  ')
tree.write(path, encoding='UTF-8', xml_declaration=True)
print('CCS project membership synchronized')
