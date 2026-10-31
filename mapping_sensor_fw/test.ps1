param([string]$CcsRoot='C:\ti\ccs2101\ccs',[string]$Python='C:\Users\LG\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe')
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
$compiler=Join-Path $CcsRoot 'tools\compiler\ti-cgt-armllvm_5.1.1.LTS'
Push-Location $root
try {
  New-Item -ItemType Directory -Force build | Out-Null
  & "$compiler/bin/tiarmclang.exe" -mcpu=cortex-m0plus -march=thumbv6m -mthumb -mfloat-abi=soft -O2 -g -Wall -Wextra -Werror -Iinclude src/sensor.c tests/test_sensor.c -o build/tests.out '-Wl,--entry_point=tests_run' '-Wl,--ram_model' '-Wl,--stack_size=2048' '-Wl,-m,build/tests.map' "-L$compiler/lib" tests/test_linker.cmd '-llibc.a'
  if($LASTEXITCODE -ne 0){throw 'Test build failed'}
  & $Python tools/emulate_tests.py
  if($LASTEXITCODE -ne 0){throw 'ARM emulation tests failed'}
} finally {Pop-Location}
