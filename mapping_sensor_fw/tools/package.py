from pathlib import Path
import zipfile,hashlib,json
root=Path(__file__).resolve().parents[1]
manifest={}
for name in ['mapping_sensor.out','mapping_sensor.hex','mapping_sensor.map','test_results.json','artifact_checks.json']:
    p=root/'build'/name;manifest[name]=hashlib.sha256(p.read_bytes()).hexdigest()
(root/'build/manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
out=root.parent/'mapping_sensor_fw_v0_1.zip'
excluded={'ccs_workspace','ccs_verified_workspace','python_deps','__pycache__'}
with zipfile.ZipFile(out,'w',compression=zipfile.ZIP_DEFLATED) as z:
    for p in root.rglob('*'):
        if p.is_file() and not excluded.intersection(p.relative_to(root).parts):
            z.write(p,Path('mapping_sensor_fw')/p.relative_to(root))
print(f'{out} ({out.stat().st_size:,} bytes)')
