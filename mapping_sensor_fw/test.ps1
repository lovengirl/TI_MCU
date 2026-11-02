param([string]$CcsRoot='C:\ti\ccs2101\ccs',[string]$Python='C:\Users\LG\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe')
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
$compiler=Join-Path $CcsRoot 'tools\compiler\ti-cgt-armllvm_5.1.1.LTS'
Push-Location $root
try {
  New-Item -ItemType Directory -Force build | Out-Null
  $sources=@(Get-ChildItem src -Recurse -Filter '*.c' | Where-Object { $_.Directory.Name -in @('app','comm','config','diagnostics','processing','sensor') } | ForEach-Object { Resolve-Path $_.FullName -Relative })
  & "$compiler/bin/tiarmclang.exe" -mcpu=cortex-m0plus -march=thumbv6m -mthumb -mfloat-abi=soft -O2 -g -Wall -Wextra -Werror -Iinclude -Itests @sources tests/test_v2.c tests/mock_hardware.c -o build/tests.out '-Wl,--entry_point=tests_run' '-Wl,--ram_model' '-Wl,--stack_size=2048' '-Wl,-m,build/tests.map' "-L$compiler/lib" tests/test_linker.cmd '-llibc.a'
  if($LASTEXITCODE -ne 0){throw 'Test build failed'}
  & $Python tools/emulate_tests.py
  if($LASTEXITCODE -ne 0){throw 'ARM emulation tests failed'}
  $drivers=@(Get-ChildItem src/drivers -Filter '*.c' | ForEach-Object {Resolve-Path $_.FullName -Relative})
  & "$compiler/bin/tiarmclang.exe" -mcpu=cortex-m0plus -march=thumbv6m -mthumb -mfloat-abi=soft -O2 -g -Wall -Wextra -Werror -D__MSPM0G3507__ -Iinclude -Ibuild -Iccs_support/sdk/source -Iccs_support/sdk/source/third_party/CMSIS/Core/Include @drivers tests/test_drivers.c -o build/driver_tests.out '-Wl,--entry_point=tests_run' '-Wl,--ram_model' '-Wl,--stack_size=2048' '-Wl,-m,build/driver_tests.map' "-L$compiler/lib" tests/test_linker.cmd '-llibc.a'
  if($LASTEXITCODE -ne 0){throw 'Driver test build failed'}
  & $Python tools/emulate_tests.py --drivers
  if($LASTEXITCODE -ne 0){throw 'Driver register-contract tests failed'}
} finally {Pop-Location}
