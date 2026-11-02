# V2 FW 시험판 검증 결과

기준일: 2026-11-02 (한국 시간), FW 0.2. V0.1 릴리스 파일은 보존합니다.

## 결과

| 검증 | 결과 |
|---|---|
| TI SysConfig CLI | 오류/경고 없음, 정보 메시지만 출력 |
| TI Arm Clang 직접 빌드 | PASS, OUT/HEX 생성, -Wall -Wextra -Werror |
| CCS 21.0.1 프로젝트 가져오기 | PASS |
| CCS 자체 Debug 빌드 | 오류 0 / 경고 0 |
| 응용·프로토콜 통합 ARM 시험 | 15 시나리오 / 15,933 assertion 통과 |
| 실제 드라이버 레지스터 계약 ARM 시험 | 6 시나리오 / 42 assertion 통과 |
| 생성 설정·ELF 검사 | 35 검사 통과 |
| 계층 분리·CCS 소스 목록 검사 | 41 검사 통과 |
| MCU 다운로드·실물 시험 | 수행하지 않음 |

직접 빌드 메모리: Flash 16,464 bytes / 128KB, SRAM 6,526 bytes / 32KB (2KB 스택 예약 포함). 스택 최대 사용량의 실측은 수행하지 않았습니다. 본체 C 파일은 19개로 분리했고 main은 App_Init/App_Process만 호출합니다. board.syscfg는 변경하지 않았습니다.

## 시험 구성과 한계

통합 시험은 TI 컴파일러로 생성한 app/scan/config/frame/processing/detection/parser/dispatcher/formatter를 실제로 실행하며, 보드 드라이버만 모의 구현입니다. 별도 드라이버 시험은 실제 drivers/*.c를 실행하되 MMIO를 일반 메모리에 매핑하고 상태 레지스터에 값을 주입합니다.

실제 레지스터의 하드웨어 부작용, UART 파형, DMA 이벤트 타이밍, 광학 신호, DAC settling, Watchdog 리셋, 전원/클록 동작은 보드 시험이 필요합니다. TI 주변장치 시뮬레이터나 실물 측정 결과가 아닙니다.

15 통합 시나리오: UART 단발 전체 스캔·20채널 결과, 히스테리시스, 설정 적용 경계, 잘못된 설정 거부, 각 상태 즉시 정지·채널 종료 정지, 타임아웃·오류 해제, 포화·극성/보정, 잘못된 샘플/이벤트, 프레임 소유권, 신호 경계값, 명령 파서, 긴 줄/오버런 후 복구, 송신 정체·결과 스냅샷 보존, 결과 송신 중 ESTOP, 100프레임 연속 운전.

6 드라이버 시나리오: Tx/Rx 활성 상태의 주소·DAC 갱신 금지, 대기 완료 단일 소비·취소 IRQ 무시, 25ms 타임아웃 출력 차단, ADC 취득 세대·소유권, 취소된 DMA·진행 중 변환 재무장 거부, UART 송신 backpressure.

35 정적 검사: 주요 GPIO 물리 핀, 80MHz 설정, ADC0/PA27, 매 샘플 이벤트 트리거, DAC/PA15, DMA 폭·주소 증가, 불필요 SPI CS 핀 미할당, WWDT, 2KB 스택, 주요 IRQ 벡터, Flash/RAM 적재 범위, BCR/BSL 설정 데이터 없음.

41 구조 검사: 본체 소스 19개, 오브젝트 이름 충돌 없음, 최소 main, 기존 단일 파일 제거, 모든 CCS 소스 등록, 상위 모듈 직접 TI 레지스터 접근 금지, 순수 모듈 app 역참조 금지.

## 재현과 산출물

- build.ps1: TI Arm Clang 5.1.1.LTS와 SysConfig로 재생성·빌드.
- CCS: ccs_v2_workspace에 projectspec을 가져와 Debug 빌드.
- test.ps1: Cortex-M0+ 시험 실행 파일 2개를 빌드해 Unicorn으로 실행.
- tools/verify_artifacts.py 및 tools/verify_architecture.py: 설정·ELF·구조 검사.
- JSON 결과, OUT/HEX/MAP 및 SHA-256 manifest: 저장소 releases/v0.2.

## 호환성과 범위

기존 핀 배치와 명령 이름/응답 필드를 유지하고 부팅 버전을 0.2로 변경했습니다. 응답 중 추가 요청은 ERR로 거부하지만 STOP/ESTOP은 실행 후 응답을 순서대로 전송합니다. 전체 응답을 받은 뒤 다음 명령을 보내십시오.

설정은 RAM 전용입니다. 자동 광량 보정·Flash 설정 저장·CC-Link IE TSN 스택은 포함하지 않습니다. SPI는 예약 상태입니다. 실물 시험 순서는 저장소 루트 README의 체크리스트를 따르십시오.
