"""Package verified generated config and SDK headers/library for a portable CCS project."""
from pathlib import Path
import shutil,argparse
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument('--sdk-root',type=Path,default=root.parent/'vendor/mspm0-sdk')
sdk=parser.parse_args().sdk_root.resolve()
support=root/'ccs_support';support.mkdir(exist_ok=True)
for name in ['ti_msp_dl_config.c','ti_msp_dl_config.h']:
    shutil.copy2(root/'build'/name,support/name)
shutil.copy2(sdk/'source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c',support/'startup_mspm0g350x_ticlang.c')
text=(root/'build/device_linker.cmd').read_text(encoding='utf-8')
(support/'firmware_linker.cmd').write_text(text.replace('--stack_size=512','--stack_size=2048'),encoding='utf-8')
files=list((sdk/'source/ti').rglob('*.h'))+list((sdk/'source/third_party/CMSIS/Core/Include').rglob('*.h'))
files += [sdk/'source/ti/driverlib/lib/ticlang/m0p/mspm0g1x0x_g3x0x/driverlib.a']
files += list((sdk/'source').rglob('LICENSE*'))
for src in files:
    dst=support/'sdk'/src.relative_to(sdk);dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst)
print(f'CCS support package: {len(files)} SDK headers/library/license files')
