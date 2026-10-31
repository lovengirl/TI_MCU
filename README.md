# TI MCU 매핑센서 FW 시험 프로젝트

MSPM0G3507SPMR 기반 20채널 Tx/Rx 순차 제어와 DAC 광량 조절을 검증하는 CCS용 FW입니다.

- [사용법·핀 배치·UART 시험 순서](mapping_sensor_fw/README.md)
- [내부 검증 결과](mapping_sensor_fw/VALIDATION.md)
- [CCS 프로젝트 가져오기](mapping_sensor_fw/mapping_sensor.projectspec)
- [다운로드용 OUT](releases/v0.1/mapping_sensor.out) / [Intel HEX](releases/v0.1/mapping_sensor.hex)
- [ARM 에뮬레이션 결과](releases/v0.1/test_results.json)
- [생성 설정·ELF 검사 결과](releases/v0.1/artifact_checks.json)

CCS 21.0.1 자체 빌드: 오류 0 / 경고 0. 측정 코어 ARM 에뮬레이션: 13 시나리오, 20,948 assertion 통과. 생성 설정·ELF 검사: 35개 통과.

MCU 라이팅과 실물 검증은 수행하지 않았습니다. 아날로그 파형·실제 주변장치 타이밍은 보드 시험이 필요합니다. 설정은 RAM 전용이며 CC-Link IE TSN 통신 스택과 자동 광량 보정은 포함하지 않습니다.

## CCS에서 열기

저장소를 내려받아 `mapping_sensor_fw/mapping_sensor.projectspec`을 CCS 프로젝트 가져오기에서 선택합니다. 필요한 TI SDK 헤더·DriverLib·시작 파일과 검증된 생성 설정이 포함되어 있습니다. `.syscfg`는 자동 빌드에서 제외되어 있습니다.

독립 빌드 및 SysConfig 재생성에는 MSPM0 SDK 2.11.00.07 전체가 필요합니다. 이 저장소에는 전체 SDK 대신 CCS 빌드에 필요한 파일만 포함합니다. `build.ps1 -SdkRoot <SDK 경로>`로 지정할 수 있습니다. 생성 파일 패키지를 갱신할 때는 `python tools/prepare_ccs.py --sdk-root <SDK 경로>`를 사용합니다. 테스트 실행 시 Python과 Unicorn 2.1.4, pyelftools 0.32가 필요합니다.

`releases/v0.1`은 검증된 빌드의 고정 산출물입니다. 재빌드하면 새 산출물은 `mapping_sensor_fw/build`에 생성됩니다. 전체 SDK, Python 실행 의존성, CCS 임시 작업공간은 Git에서 제외했습니다.

## FW 시험 블록도

![FW 시험 블록도](docs/MSPM0G3507_FW_block_diagram.png)

## CCS 프로젝트 화면

![CCS 프로젝트](docs/ccs_project_open.jpg)
