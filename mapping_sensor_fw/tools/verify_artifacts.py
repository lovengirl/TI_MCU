"""Inspect generated configuration and final ELF; no target connection."""
from pathlib import Path
import sys,struct,json,re
sys.path.insert(0,str(Path(__file__).parent/'python_deps'))
from elftools.elf.elffile import ELFFile
root=Path(__file__).resolve().parents[1];b=root/'build'
c=(b/'ti_msp_dl_config.c').read_text(encoding='utf-8');h=(b/'ti_msp_dl_config.h').read_text(encoding='utf-8')
checks=[]
def check(label,condition):
    checks.append({'check':label,'pass':bool(condition)})
    if not condition:raise AssertionError(label)
pins={'TX_A0':47,'TX_A1':48,'TX_A2':50,'TX_A3':51,'TX_A4':52,'TX_EN':53,
      'RX_A0':58,'RX_A1':59,'RX_A2':60,'RX_A3':61,'RX_A4':62,'RX_EN':63,
      'HOST_IRQ':64,'HOST_RESET':1,'STATUS':2,'SCAN_MARK':3,'CS_N':54}
for name,pin in pins.items():check('pin '+name,bool(re.search(r'Defines for '+name+r':.*package pin '+str(pin)+r' \*/',h)))
check('80 MHz CPU',bool(re.search(r'CPUCLK_FREQ\s+80000000',h)))
check('ADC0 channel 0, PA27','DL_ADC12_INPUT_CHAN_0' in c and 'DL_GPIO_PIN_27' in h)
check('ADC requires each timer trigger','DL_ADC12_TRIGGER_MODE_TRIGGER_NEXT' in c)
check('DAC on PA15','GPIO_DAC12_OUT_PIN' in h and 'DL_GPIO_PIN_15' in h)
check('DMA half-word source/destination',c.count('DL_DMA_WIDTH_HALF_WORD')==2)
check('DMA source fixed destination increments','DL_DMA_ADDR_UNCHANGED' in c and 'DL_DMA_ADDR_INCREMENT' in c)
check('SPI software CS: no unexpected hardware CS pin','GPIO_HOST_SPI_CS0_PIN' not in h)
check('watchdog 1 second','DL_WWDT_TIMER_PERIOD_15_BITS' in c and 'DL_WWDT_CLOCK_DIVIDE_1' in c)
with open(b/'mapping_sensor.out','rb') as f:
    elf=ELFFile(f);symbols={s.name:s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols()}
    vec=elf.get_section_by_name('.intvecs').data()
    check('initial stack pointer in SRAM',0x20200000<struct.unpack_from('<I',vec,0)[0]<=0x20208000)
    check('2048 byte stack',elf.get_section_by_name('.stack')['sh_size']==2048)
    for name in ['SysTick_Handler','TIMG0_IRQHandler','DMA_IRQHandler','UART0_IRQHandler']:
        check('vector '+name,struct.pack('<I',symbols[name]|1) in vec)
    check('no BCR or BSL provisioning payload',all(s['sh_size']==0 for s in elf.iter_sections() if s.name in ['.BCRConfig','.BSLConfig']))
    for seg in elf.iter_segments():
        if seg['p_type']=='PT_LOAD' and seg['p_memsz']:
            addr=seg['p_vaddr'];end=addr+seg['p_memsz']
            check('load segment '+hex(addr),end<=0x20000 if addr<0x20000 else 0x20200000<=addr<end<=0x20208000)
(b/'artifact_checks.json').write_text(json.dumps(checks,indent=2),encoding='utf-8')
print(f'PASS: {len(checks)} generated-configuration / ELF checks')
