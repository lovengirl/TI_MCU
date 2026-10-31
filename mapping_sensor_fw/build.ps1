param([string]$CcsRoot='C:\ti\ccs2101\ccs',[string]$SdkRoot='')
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
if (!$SdkRoot) {$SdkRoot=Join-Path (Split-Path $root -Parent) 'vendor\mspm0-sdk'}
$compiler=Join-Path $CcsRoot 'tools\compiler\ti-cgt-armllvm_5.1.1.LTS'
$sysconfig=Join-Path $CcsRoot 'utils\sysconfig_1.28.1\sysconfig_cli.bat'
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Force -Path $build | Out-Null
& $sysconfig --product "$SdkRoot/.metadata/product.json" --script "$root/board.syscfg" --compiler ticlang --output "$build"
if ($LASTEXITCODE -ne 0) {throw 'SysConfig failed'}
Push-Location $build
try {
  $root='..'
  $SdkRoot=Resolve-Path -LiteralPath $SdkRoot -Relative
  $build='.'
  $cc=Join-Path $compiler 'bin\tiarmclang.exe'
  $common=@('-mcpu=cortex-m0plus','-march=thumbv6m','-mthumb','-mfloat-abi=soft','-O2','-g','-Wall','-Wextra','-Werror','-D__MSPM0G3507__',"-I$root/include","-I$build","-I$SdkRoot/source","-I$SdkRoot/source/third_party/CMSIS/Core/Include")
  $sources=@("$root/src/sensor.c","$root/src/main.c","$root/src/platform_mspm0.c","$build/ti_msp_dl_config.c","$SdkRoot/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c")
  $objects=@()
  foreach($src in $sources){
    $obj=Join-Path $build (([IO.Path]::GetFileNameWithoutExtension($src))+'.o')
    & $cc @common -c $src -o $obj
    if($LASTEXITCODE -ne 0){throw "Compile failed: $src"};$objects+=$obj
  }
  & $cc @common @objects -o mapping_sensor.out '-Wl,--rom_model' '-Wl,--warn_sections' '-Wl,--heap_size=0' '-Wl,-m,mapping_sensor.map' "-L$SdkRoot/source" "-L$compiler/lib" '-ldevice.cmd.genlibs' 'device_linker.cmd' '-llibc.a' '-Wl,--stack_size=2048'
  if($LASTEXITCODE -ne 0){throw 'Link failed'}
  & "$compiler/bin/tiarmobjcopy.exe" -O ihex mapping_sensor.out mapping_sensor.hex
  if($LASTEXITCODE -ne 0){throw 'HEX generation failed'}
  Write-Output 'BUILD PASS: build/mapping_sensor.out and mapping_sensor.hex'
} finally {Pop-Location}
