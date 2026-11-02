# MSPM0G3507SPMR 매핑센서 시험 FW

20채널 Tx/Rx 순차 제어, 공통 DAC의 채널별 광량 갱신, ADC OFF/ON 차분 측정을 위한 시험용 FW입니다. 실물 보드에는 연결하거나 다운로드하지 않았습니다.

## V2 블록별 소스 구조

현재 FW 버전은 0.2입니다. [상세 구조·API 설계](ARCHITECTURE_V2.md)를 실제 C 파일로 구현했습니다. main은 App_Init/App_Process만 호출합니다.

| 폴더 | 모듈 |
|---|---|
| `src/app` | 응용 작업 순서, 명령 실행 |
| `src/board` | 보드 초기화, 짧은 임계구역, 상태 출력 |
| `src/drivers` | 채널 선택, DAC/Tx, ADC/DMA, 타이머, UART, 워치독 |
| `src/sensor` | 스캔 상태 머신, 프레임 버퍼 |
| `src/processing` | 신호 보정, 감지 판정 |
| `src/config` | 설정·예약값·버전 |
| `src/comm` | 명령 파서, 응답 스냅샷과 비차단 송신 |
| `src/diagnostics` | 오류 및 통계 |
| `src/interrupts` | 드라이버 IRQ 함수로 연결하는 진입점 |

모든 공개 헤더는 `include/`에 있습니다. 다른 모듈의 상태를 직접 수정하지 않고 API를 호출합니다. 설정과 원시 프레임·처리 결과의 소유권을 분리하고, 프레임마다 설정 스냅샷을 보존합니다.

V2는 RESULT 응답을 줄 단위로 재시도하면서 전송하므로 송신 버퍼가 일시적으로 부족해도 프레임 중간 줄을 버리지 않습니다. 응답 생성 중 추가 명령은 ERR로 거부하되 STOP/ESTOP은 즉시 실행한 뒤 응답을 기존 프레임 뒤에 정렬합니다. 짧은 ACK 큐가 넘치면 TX_DROP에 반영합니다. 명령은 기존처럼 한 번에 하나씩 보내고 전체 응답을 기다립니다.

## 빌드 결과 및 범위

- `build/mapping_sensor.out`: CCS 디버그/다운로드용 ELF
- `build/mapping_sensor.hex`: Intel HEX
- `build/mapping_sensor.map`: 메모리 배치
- `board.syscfg`: 실제 CLI 검증을 통과한 핀/주변장치 설정
- `mapping_sensor.projectspec`: CCS 가져오기용. 검증된 생성 파일을 사용합니다.
- `build/test_results.json`: 응용·프로토콜 ARM 에뮬레이션 결과
- `build/driver_test_results.json`: 실제 드라이버 C의 레지스터 계약 검사
- `build/architecture_checks.json`: 계층 분리·CCS 소스 목록 검사
- `build/artifact_checks.json`: 생성 설정/ELF 검사 결과

구현: 채널 0~19 스캔, DAC 코드 갱신, 타이머 이벤트 ADC 트리거와 DMA 취득, OFF/ON 각각 4샘플, 차분/극성/오프셋/이득 보정, 히스테리시스와 연속 프레임 확인, 포화 채널 무효 처리, 단발/연속 측정, 일반/긴급 정지, 25ms 상태 타임아웃, 약 1초 WWDT, UART 명령, 이중 프레임 버퍼.

초기 부팅은 IDLE이며 Tx와 Rx 허가는 Low, DAC는 0입니다. 측정은 UART 명령으로 시작합니다. High 활성 TX_EN/RX_EN과 외부 5비트 선택 회로를 전제로 합니다.

실물에서 확인할 부분: 광량 전류 상한, ADC 전압/극성/출력 임피던스, MUX 안정화 시간, DMA 반복 취득, 실제 샘플 간격, 스캔 지터, 리셋/워치독 복구. PC 에뮬레이션은 이 아날로그 특성을 검증하지 않습니다.

이번 시험판에서는 설정을 RAM에만 유지합니다. `SAVE`는 지원하지 않는다고 응답하며 Flash를 지우거나 쓰지 않습니다. 자동 광량 보정, CC-Link IE TSN 스택, 통신 IC별 SPI 프로토콜은 포함되지 않습니다. SPI는 핀과 주변장치를 예약·초기화하지만 데이터를 전송하지 않으며 통신부 RESET_N을 Low로 유지합니다.

## 개발 도구

- CCS 21.0.1
- TI Arm Clang 5.1.1.LTS
- SysConfig 1.28.1
- MSPM0 SDK 2.11.00.07, TI 공식 Git commit `20807db79aa17b49f87ab8ec87f6b6d63ee2cb32`
- 테스트: Unicorn 2.1.4, pyelftools 0.32, Python 3.12

다른 PC에서 테스트를 재실행할 때는 Python 환경에 `unicorn==2.1.4`와 `pyelftools==0.32`가 필요합니다. `python -m pip install --target tools/python_deps unicorn==2.1.4 pyelftools==0.32`로 준비하고 `test.ps1 -Python <python.exe 경로>`로 실행합니다. Git 저장소에는 이 실행 의존성을 포함하지 않습니다.

독립 빌드의 기본 SDK 경로는 저장소 루트의 `vendor/mspm0-sdk`입니다. 다른 위치는 `build.ps1 -SdkRoot <SDK 경로>`로 지정합니다. Windows 경로 길이 및 대소문자 충돌을 피하도록 소스/예제 중심의 sparse checkout으로 확보했습니다. 시스템 TI 설치 폴더에 SDK를 설치하거나 수정하지 않았습니다.

```powershell
& .\build.ps1
& .\test.ps1
& 'C:\Users\LG\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' .\tools\verify_artifacts.py
```

TI 컴파일러의 한글 절대 경로 처리 문제를 피하기 위해 빌드 스크립트 내부 컴파일/링크 인자는 상대 경로를 사용합니다. 소스는 `-Wall -Wextra -Werror` 조건으로 컴파일합니다.

## CCS 사용

`mapping_sensor.projectspec`을 프로젝트 가져오기 기능에서 선택합니다. 필요한 SDK 헤더/라이브러리와 검증된 `ccs_support` 생성 파일이 함께 복사됩니다. 이 프로젝트에서 `.syscfg` 자동 재생성은 제외되어 있습니다. 핀 설정을 변경하면 먼저 `build.ps1`로 재생성/검증하고 `tools/prepare_ccs.py`로 `ccs_support`를 갱신한 뒤 CCS 프로젝트에 반영해야 합니다.

원래 작업공간의 `ccs_v2_workspace/mapping_sensor_mspm0g3507`에서 CCS 자체 가져오기/빌드를 검증했습니다. 최종 빌드는 오류 0개·경고 0개입니다. Git에는 임시 작업공간 대신 projectspec과 필요한 파일을 포함했습니다. 기존 V1 프로젝트의 복사된 소스는 자동으로 V2로 갱신되지 않으므로 새 작업공간으로 가져와 사용합니다. C/H 파일 추가 후에는 `python tools/update_projectspec.py`로 목록을 갱신합니다.

XDS110을 연결하고 `build/mapping_sensor.out`을 사용해도 됩니다. 다운로드와 실물 시험은 사용자가 수행합니다. BCR/BSL 잠금이나 부트 설정을 변경하는 데이터는 포함하지 않습니다.

## 핀 배치 (LQFP64 물리 번호)

| 기능 | 포트 | 핀 |
|---|---|---|
| TX 주소 0~4 | PB0~PB4 | 47,48,50,51,52 |
| TX_EN | PB5 | 53 |
| RX 주소 0~4 | PB6~PB10 | 58,59,60,61,62 |
| RX_EN | PB11 | 63 |
| DAC | PA15 | 8 |
| ADC0 CH0 | PA27 | 31 |
| SPI SCK/MISO/MOSI | PA12/PA13/PA14 | 5/6/7 |
| CS_N | PA8 | 54 |
| IRQ_N/RESET_N | PB12/PB13 | 64/1 |
| UART TX/RX | PA10/PA11 | 56/57 |
| SWDIO/SWCLK | PA19/PA20 | 12/13 |
| STATUS/SCAN_MARK | PB14/PB15 | 2/3 |
| VDD/VSS/VCORE/NRST | — | 40/41/32/38 |

## 측정 타이밍과 결과

TIMG0는 1MHz one-shot으로 채널/DAC 안정화 50us, 발광 안정화 50us를 관리합니다. TIMG6는 1MHz 타이머 카운트 기준, 10us마다 ADC0에 이벤트를 보냅니다. ADC 샘플 시간은 2us이며 DMA CH0이 MEM0에서 16비트 데이터를 4개씩 RAM에 저장합니다. ADC는 매 변환마다 새 트리거를 요구합니다.

DMA 완료 ISR은 샘플 타이머를 정지하고 Tx를 차단합니다. 상태 전이는 메인 루프가 처리하므로 안정화 대기는 최소 대기시간이며, UART 처리량에 따라 채널/프레임 간격이 늘 수 있습니다. SCAN_MARK는 ADC 취득 블록 동안 High입니다. 이 핀과 TX_EN을 함께 측정하면 OFF/ON 블록을 구분할 수 있습니다.

초기 설정: DAC 코드 512, 이득 Q12=4096, 오프셋=0, 진입 임계값 500, 해제 임계값 400, 연속 확인 2프레임. 감지 비트 1은 보정 신호가 임계값 이상이라는 의미이며, 기판 존재/차광 여부와의 관계는 센서 회로에 맞춰 정해야 합니다. 4095로 포화된 샘플이 있으면 해당 채널은 무효 처리합니다. 0은 빛이 없는 정상 상황일 수 있어 자동으로 오류 판정하지 않습니다.

이중 버퍼는 완료 프레임을 소비할 때까지 다음 프레임을 보류합니다. 메인에서 보정/판정 후 최신 결과를 복사하고 버퍼를 해제합니다. 설정 변경은 처리 중인 프레임에 섞이지 않으며 다음 프레임 시작에 적용됩니다.

## UART 명령과 최초 보드 시험

3.3V USB-UART 사용, TX(PA10)→어댑터 RX, RX(PA11)←어댑터 TX, 공통 GND. 설정 115200 / 8N1 / 흐름제어 없음. 명령은 ASCII와 CR 또는 LF로 종료합니다. 채널 번호는 0~19입니다.

| 명령 | 의미 |
|---|---|
| `HELP` | 명령 목록 |
| `STATUS` | 상태/오류/프레임/설정 버전/버퍼 손실 횟수 |
| `SINGLE` | 20채널 1프레임 |
| `START` | 연속 측정 |
| `STOP` | 현재 채널 종료 후 정지, 미완성 프레임 폐기 |
| `ESTOP` | 명령 파싱 즉시 출력 차단·정지 |
| `RESULT` | 최근 완료 프레임의 비트맵·20채널 원시값/보정값 |
| `DAC 0 512` | 0번 채널 광량 코드 |
| `THRESH 0 500 400` | 진입/해제 임계값 |
| `POLARITY 1` | ON-OFF (`0`은 OFF-ON) |
| `CLEAR` | 오류 해제 후 IDLE |

명령 성공은 `OK`, 부적절한 인수/상태는 `ERR`입니다. 설정 변경의 `OK`는 유효한 설정을 수락했다는 의미이며 실제 적용 버전은 STATUS/RESULT에서 확인합니다. 프레임이 없으면 RESULT는 NO_DATA를 반환합니다. 자동 결과 스트리밍은 하지 않습니다.

1. 전원/핀 연결과 Tx 차단 상태를 확인합니다. UART 부팅 메시지는 READY/IDLE입니다.
2. `STATUS`, `SINGLE`, `RESULT` 순으로 시험합니다.
3. PA27에 알려진 전압을 넣어 20채널 취득/포화 표시를 확인합니다. 광학 경로 없이 고정 전압만 넣으면 ON/OFF 차분은 거의 0이 정상입니다.
4. 오실로스코프로 TX_EN, SCAN_MARK, 주소선을 확인합니다.
5. 광학 경로 연결 후 DAC를 낮은 값에서 조정하고 원시값·극성을 확인합니다.
6. `START`, `STOP`, `ESTOP`, 전원 재투입을 시험합니다.

V1의 g_sensor/g_latest_frame/g_frame_count는 제거했습니다. 조회 API는 App_GetStatus/App_GetLatestFrame/Scan_GetStatus입니다. CCS에서 소스 문맥을 선택해 app.c의 latest_frame 또는 scan_controller.c의 state/channel을 관찰하거나 UART STATUS/RESULT로 조회합니다. 최적화 빌드에서는 일부 변수가 표시되지 않을 수 있습니다. 오류 코드는 1=시간 초과, 2/3=잘못된 이벤트·드라이버 상태, 4=ADC 범위 이상입니다. STATUS LED는 오류 상태에서 High입니다.

## 시뮬레이션의 범위

설치된 CCS에서 MSPM0G3507의 주변장치 시뮬레이터는 확인되지 않았습니다. V2에서는 TI 컴파일러로 컴파일한 실제 응용·프로토콜 모듈을 모의 하드웨어와 ARM Thumb 에뮬레이터에서 실행합니다. 별도 시험에서는 실제 드라이버 C를 메모리로 치환한 레지스터와 실행해 호출 계약을 확인합니다. 실제 레지스터의 부작용·ADC/DAC 파형·광학 신호·실물 IRQ 타이밍은 검증하지 않습니다.

내부 테스트는 20채널 순서, 히스테리시스, 프레임 경계 설정 변경, 입력 범위, 일반 정지, 오류 복구, ADC 포화, 극성/보정, 잘못된 이벤트, 버퍼 소유권, 비정상 ADC값, 각 상태의 긴급 정지, 100프레임 연속 운전을 검사합니다.
