"""Run TI-compiled Cortex-M0+ core tests. No MSPM0 peripheral model is claimed."""
import sys, json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent/'python_deps'))
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS
from unicorn.arm_const import UC_ARM_REG_SP,UC_ARM_REG_LR,UC_ARM_REG_R0
from elftools.elf.elffile import ELFFile
root=Path(__file__).resolve().parents[1]
uc=Uc(UC_ARCH_ARM,UC_MODE_THUMB|UC_MODE_MCLASS)
uc.mem_map(0,0x20000);uc.mem_map(0x20200000,0x8000)
with open(root/'build/tests.out','rb') as f:
    elf=ELFFile(f)
    for section in elf.iter_sections():
        if section['sh_flags']&2 and section['sh_size'] and section['sh_type']!='SHT_NOBITS':
            uc.mem_write(section['sh_addr'],section.data())
    sym={s.name:s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols()}
uc.reg_write(UC_ARM_REG_SP,0x20207ff0)
uc.reg_write(UC_ARM_REG_LR,0x1fff1)
uc.emu_start(sym['tests_run']|1,0x1fff0,count=20000000)
value=lambda n:int.from_bytes(uc.mem_read(sym[n],4),'little')
result={'test_cases_passed':value('test_cases'),'assertions':value('test_checks'),
        'failed_case':uc.reg_read(UC_ARM_REG_R0),'failed_line':value('test_fail_line'),
        'scope':'TI Arm Clang compiled sensor.c on ARM Thumb emulator; mock HAL, no peripheral timing simulation'}
(root/'build/test_results.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print(json.dumps(result,indent=2))
if result['failed_case'] or result['test_cases_passed']!=13:sys.exit(1)
