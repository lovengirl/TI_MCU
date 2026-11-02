# 매핑센서 FW V2 상세 설계

상태: V2(0.2) 구현 완료, 2026-11-02. MCU: MSPM0G3507SPMR. 기존 핀 배치 및 UART 명령 유지. V0.1 릴리스 파일은 보존하고 현재 소스를 V2로 교체함.

구현 시 상세 조정: 헤더는 `include/`에 모으고 C 파일은 `src/` 하위 기능 폴더에 배치했다. Acquisition_Prepare는 Start 내부 준비 절차로 합쳤으며 ADC 완료 뷰를 소비·반납한다. 프레임에는 채널별 OFF/ON 평균과 설정 스냅샷을 보관한다. generation 검사는 완료 뷰의 세대 확인이며, 실제 IRQ에 과거 세대 번호가 실려 오는 것은 아니므로 하드웨어 취소 시 ADC/DMA 정지와 잔여 플래그 제거도 수행한다. 시료·광학 경로·실제 IRQ 타이밍은 보드에서 추가 검증해야 한다.

## 1. 설계 목표

- main은 App_Init / App_Process 호출만 담당한다.
- 하드웨어 접근, 측정 순서, 신호 계산, 사용자 명령을 별도 모듈로 구성한다.
- 파일 분리뿐 아니라 상태와 데이터의 소유자를 구분한다.
- 정상 정지, 즉시 정지, 오류 차단을 별도 API로 제공한다.
- GPIO 핀과 UART 명령, 알고리즘을 서로 독립적으로 변경할 수 있게 한다.
- 함수 선언에는 인수와 반환값 의미를 문서화하고, 함수 본문은 일반적인 들여쓰기와 한 문장 한 줄로 작성한다.

## 2. 디렉터리와 책임

| 위치 | C/H 모듈 | 책임 / 소유 상태 |
|---|---|---|
| src/main.c | main | 초기화 및 실행 루프 |
| app/ | app | 모듈 조립과 작업 순서 |
| app/ | command_dispatcher | 검증된 명령을 모듈 API로 전달 |
| board/ | board | 전원 투입 후 안전 출력, 주변장치 초기화 순서 |
| board/ | board_pins.h | 실제 핀 이름과 활성 극성 정의 |
| drivers/ | channel_selector | Tx/Rx 주소 및 Rx 선택 허가 |
| drivers/ | light_control | DAC 광량 기준과 Tx 출력 허가 |
| drivers/ | adc_acquisition | ADC/DMA 상태, 샘플 버퍼 |
| drivers/ | measurement_timer | 대기 타이머, 샘플 타이머, 이벤트 자원 |
| drivers/ | uart_transport | 송수신 FIFO 및 오류 카운터 |
| drivers/ | watchdog_service | main 진행 확인, 워치독 갱신 |
| sensor/ | scan_controller | 채널 번호와 측정 상태 머신 |
| sensor/ | frame_buffer | 원시 프레임 2개와 버퍼 소유권 |
| processing/ | signal_processing | 평균·차분·보정 계산 |
| processing/ | detection | 채널별 히스테리시스·연속 판정 상태 |
| config/ | sensor_config | 활성/예약 설정과 설정 버전 |
| comm/ | command_parser | 입력 줄 파싱, 문법·숫자 범위 검사 |
| comm/ | response_formatter | 상태·결과를 UART 응답으로 변환 |
| diagnostics/ | diagnostics | 오류 원인과 통계, 오류 해제 조건 |
| include/ | sensor_types.h | 공통 값 타입, enum, 읽기 전용 결과 형식 |
| interrupts/ | interrupt_handlers | IRQ 진입점과 각 드라이버 ISR 함수 연결 |
| generated/ | ti_msp_dl_config | SysConfig 생성 코드, 수동 수정 금지 |

각 C 파일에는 짝이 되는 헤더를 둔다. 파일 내부 보조 함수·변수는 static으로 제한한다. 다른 모듈의 내부 구조체를 수정하는 API는 제공하지 않는다.

## 3. 호출 방향

main → app → scan_controller / command_dispatcher / diagnostics

scan_controller → channel_selector / light_control / adc_acquisition / measurement_timer / frame_buffer / sensor_config

app → 완료 프레임 처리 → signal_processing → detection → 결과 스냅샷

UART 수신 → uart_transport → command_parser → command_dispatcher

응답 요청 → response_formatter → uart_transport

interrupt_handlers → 담당 드라이버의 IRQ 처리 함수만 호출한다. command_parser는 TI SDK를 include하지 않고, signal_processing도 UART나 GPIO에 접근하지 않는다.

## 4. 데이터 소유권

| 데이터 | 소유자 | 외부 접근 |
|---|---|---|
| 현재 채널, 상태, 정지 요청 | scan_controller | 상태 스냅샷 조회 |
| 활성 설정 / 예약 설정 / 버전 | sensor_config | 값 복사 조회, 변경 요청 |
| DMA 샘플 배열 | adc_acquisition | 완료 후 const 포인터 대여 |
| 원시 프레임 2개 | frame_buffer | 쓰기 핸들 / 읽기 핸들 |
| 완료된 보정·판정 결과 | app | 전체 처리 완료 후 스냅샷 공개 |
| 채널별 연속 확인 횟수 | detection | Reset / Update API |
| 오류 코드와 카운터 | diagnostics | 조회 / 기록 API |
| UART 링 버퍼 | uart_transport | Read / TryWrite API |

프레임은 FREE → WRITING → READY → READING → FREE로 이동한다. WRITING 버퍼만 측정 엔진이 수정할 수 있다. READY/READING 버퍼는 덮어쓰지 않는다. 초기 V2에서는 완료 프레임을 처리·해제한 뒤 다음 프레임을 시작하여 현재 동작을 유지한다.

DMA 버퍼는 IDLE → ARMED → COMPLETE → IDLE로 관리한다. COMPLETE 상태에서만 결과를 읽고, 다음 취득 시작 전에는 이전 결과의 소유권을 반납한다.

## 5. 공개 API 설계

### 응용과 보드

| 함수 | 역할 |
|---|---|
| App_Init() | 보드·센서·통신·진단 초기화 |
| App_Process() | 이벤트 소비, 프레임 처리, 명령 실행, 응답 송신, 상태 점검 |
| App_GetStatus(out_status) | 외부 공개 상태를 값 복사 |
| Board_InitSafeOutputs() | Tx/Rx 비활성화, 통신 CS 해제, 리셋 유지 |
| Board_InitPeripherals() | 생성 초기화 호출과 런타임 자원 준비 |

### 채널과 광량

| 함수 | 반환/조건 |
|---|---|
| ChannelSelector_Select(tx_channel, rx_channel) | 범위 0~19 검사. Tx와 Rx가 비활성일 때만 허용 |
| ChannelSelector_EnableRx() / DisableRx() | Rx 선택 허가 제어 |
| LightControl_SetLevel(code) | 0~4095 검사. Tx OFF일 때만 갱신 |
| LightControl_EnableTx() / DisableTx() | 선택된 Tx 출력 허가 |
| LightControl_ForceOffFromISR() | 계산·대기 없이 즉시 Tx 차단 |
| LightControl_SetIdleLevel() | 정지 시 DAC 0 적용 |

### 시간과 취득

| 함수 | 반환/조건 |
|---|---|
| MeasurementTimer_StartWait(duration_us) | one-shot 대기 시작, 중복 시작은 BUSY |
| MeasurementTimer_TakeWaitComplete() | 완료 플래그를 한 번만 소비 |
| MeasurementTimer_StartSampleClock(period_us) | 타이머 이벤트로 ADC 트리거 |
| MeasurementTimer_StopAll() | 타이머 중지와 잔여 상태 정리 |
| Acquisition_Prepare(sample_count) | DMA 목적지·개수·요청 상태 설정 |
| Acquisition_Start(sample_period_us) | 취득을 무장한 후 샘플 타이머 시작 |
| Acquisition_GetStatus() | IDLE / BUSY / COMPLETE / ERROR |
| Acquisition_GetSamples(out_view) | 완료 샘플 const 뷰 제공 |
| Acquisition_Release() | 읽기 완료 후 버퍼 반납 |
| Acquisition_Cancel() | 트리거 중지 → ADC/DMA 취소 → 플래그 정리 |

반환값은 공통 FwStatus(enum: OK, BUSY, INVALID_ARGUMENT, INVALID_STATE, TIMEOUT, HARDWARE_ERROR)를 사용한다. true/false 하나로 인수 오류와 사용 중 상태를 섞지 않는다.

### 측정·설정·프레임

| 함수 | 역할 |
|---|---|
| Scan_Init(interfaces) | 모의/실물 드라이버 인터페이스 주입 |
| Scan_StartSingle() / StartContinuous() | 대기 상태에서만 시작 |
| Scan_RequestStop() | 현재 채널 종료 후 프레임 폐기·정지 |
| Scan_EmergencyStop() | 출력 즉시 차단, 미완성 프레임 폐기 |
| Scan_Process() | 대기·취득 완료 이벤트에 따른 상태 전이 |
| Scan_EnterFault(code) | 안전 출력 적용 후 오류 상태 진입 |
| Scan_GetStatus(out_status) | 현재 동작 정보 조회 |
| Config_LoadDefaults() / Validate(candidate) | 기본값 생성 / 설정 범위 검사 |
| Config_Stage(candidate) | 예약 설정 수락 |
| Config_ApplyAtFrameBoundary() | 다음 프레임 시작에 일괄 적용 |
| Config_GetSnapshot(out_config, out_version) | 한 프레임에 사용할 설정 복사 |
| FrameBuffer_AcquireWrite(out_handle) | FREE 버퍼 획득 |
| FrameBuffer_CommitChannel(handle, channel, samples) | 채널 결과 기록 |
| FrameBuffer_Publish(handle, metadata) | 20채널 완료 버퍼 공개 |
| FrameBuffer_GetReady(out_handle) / Release(handle) | 완료 버퍼 대여 / 반납 |
| FrameBuffer_Discard(handle) | 미완성 버퍼 폐기 |

### 보정과 판정

| 함수 | 역할 |
|---|---|
| Signal_CalculateMean(samples, count) | 샘플 범위·개수 확인 후 평균 |
| Signal_CheckSaturation(samples, count) | ADC 상한 포화 확인 |
| Signal_CalculateDifference(off, on, polarity) | 부호를 보존한 차분 |
| Signal_ApplyCalibration(value, offset, gain_q12) | 충분한 비트 폭으로 계산 |
| Signal_ProcessFrame(raw, config, out_signals) | 채널별 처리 orchestration |
| Detection_Reset() | 누적 상태 초기화 |
| Detection_UpdateChannel(channel, signal, valid, limits) | 채널별 판정 상태 갱신 |
| Detection_UpdateFrame(signals, config, out_bitmap) | 완료 프레임의 판정 |

평균값 처리와 OFF/ON 샘플의 수집은 구별한다. 설정 스냅샷은 프레임이 처리될 때까지 유지하여 예약 설정이 이전 프레임의 보정에 섞이지 않게 한다.

### 통신·진단

| 함수 | 역할 |
|---|---|
| UartTransport_ReadByte(out_byte) | 수신 링 버퍼 소비 |
| UartTransport_TryWrite(data, size) | 공간 부족 시 BUSY, 부분 패킷 전송 방지 |
| UartTransport_ProcessTx() | 하드웨어 FIFO로 비차단 전송 |
| CommandParser_FeedByte(byte) | 줄 수집, 길이 초과·제어문자 처리 |
| CommandParser_Parse(line, out_command) | 문자열 → 타입이 있는 Command 구조체 |
| CommandDispatcher_Execute(command, out_reply) | 측정·설정 API 호출 |
| ResponseFormatter_BeginStatus(snapshot) | 상태 응답 준비 |
| ResponseFormatter_BeginFrame(snapshot) | 변경되지 않는 결과 스냅샷 확보 |
| ResponseFormatter_Process() | 공간에 맞춰 프레임 응답을 나누어 송신 |
| Diagnostics_ReportFault(code, context) | 첫 원인과 이후 통계 기록 |
| Diagnostics_GetSnapshot(out_status) | 진단 정보 조회 |
| Diagnostics_CanClearFault() | 출력 차단·진행 중 취득 없음 확인 |
| WatchdogService_ReportProgress(task) / Process() | 응용 루프와 시간 진행 확인 후 갱신 |

명령 목록은 STATUS / RESULT / SINGLE / START / STOP / ESTOP / CLEAR / DAC / THRESH / POLARITY / HELP를 유지한다. Flash 저장과 TSN 스택은 별도 요구사항으로 남긴다.

## 6. 상태 전이와 함수 분해

IDLE → SELECT_WAIT → OFF_ACQUIRE → LIGHT_WAIT → ON_ACQUIRE → CHANNEL_COMPLETE

CHANNEL_COMPLETE → 다음 채널 SELECT_WAIT 또는 FRAME_COMPLETE → IDLE

모든 동작 상태 → FAULT / 즉시 정지 IDLE.

scan_controller 내부 보조 함수:

- Scan_PrepareFrame: 설정 스냅샷, 프레임 버퍼 획득.
- Scan_PrepareChannel: Tx OFF, Rx OFF, 주소·DAC 설정, Rx ON.
- Scan_BeginDarkAcquisition: OFF 샘플 취득 준비.
- Scan_BeginLightSettling: Tx ON과 안정화 대기.
- Scan_BeginLightAcquisition: ON 샘플 취득 준비.
- Scan_StoreChannelResult: 취득 결과를 프레임에 저장.
- Scan_AdvanceChannel: 정지 요청 확인 및 다음 채널 선택.
- Scan_CompleteFrame: 출력 차단과 완료 버퍼 공개.
- Scan_AbortFrame: 출력 차단과 미완성 버퍼 폐기.

동일한 로직을 여러 파일에 복제하지 않고 이 경계로 함수를 분해한다. 각 함수의 실패는 Scan_EnterFault로 모은다.

## 7. 인터럽트와 동시성

- TIMG0 IRQ: 해당 인터럽트 확인/해제 후 대기 완료 이벤트만 기록.
- DMA IRQ: 샘플 트리거 정지, Tx 차단, 완료 상태 기록. 판정이나 문자열 처리 금지.
- UART IRQ: 수신 바이트와 오버런 상태 기록. 명령 실행 금지.
- SysTick IRQ: 시간과 진행 중 단계의 deadline 감시. 타임아웃 시 Tx 즉시 차단 후 오류 이벤트 기록.
- main은 이벤트 소비 및 모든 측정 상태 전이를 수행한다.
- volatile은 최적화 제어용이며 상호배제 대용으로 사용하지 않는다.
- 이벤트 교환·취득 재무장 등 짧은 구간에만 인터럽트를 마스킹한다. 파싱·포맷팅·프레임 계산 중 전체 인터럽트 마스킹은 금지한다.
- 취소 후 늦게 도착한 완료를 새 취득으로 오인하지 않도록 acquisition generation ID를 사용한다. 취소 과정에서는 peripheral/NVIC 잔여 요청도 정리한다.

## 8. 응용 루프의 실행 순서

1. 오류 이벤트 소비, 필요한 안전 차단 적용.
2. 측정 완료 이벤트 소비 및 Scan_Process 실행.
3. READY 프레임 보정/판정, 최신 결과의 원자적 공개.
4. 수신 바이트를 제한된 개수만 파싱하고 명령 처리.
5. 응답의 일부를 비차단 송신. 결과 응답 동안 동일 스냅샷 유지.
6. 진단·워치독 진행 확인.

응답 대기 중에도 측정 상태 처리를 우선한다. 단일 UART 인터페이스에서 여러 RESULT를 동시에 요청하면 BUSY로 응답하며 응답끼리 섞지 않는다.

## 9. 단계별 마이그레이션과 완료 기준

| 단계 | 작업 | 완료 기준 |
|---|---|---|
| 1 | 공통 타입·API·책임 확정 | 헤더 include 순환 없음, TI SDK가 순수 처리 모듈에 유입되지 않음 |
| 2 | 하드웨어 드라이버 분리 | 기존 핀/타이머/ADC/DMA 생성 설정과 동일 |
| 3 | 설정·버퍼 분리 | 프레임 경계 적용, 완료 버퍼 덮어쓰기 방지 |
| 4 | 보정·판정 분리 | 기존 코어 테스트와 경계값 시험 통과 |
| 5 | 스캔 상태 머신 재구현 | 20채널 순서·단발/연속·정지·오류 전이 통과 |
| 6 | 파서·명령 실행·응답 분리 | 인수 오류/긴 줄/송신 부족/응답 스냅샷 시험 통과 |
| 7 | app 조립과 빌드 설정 갱신 | main은 초기화/루프 호출만 포함, 직접 빌드·CCS 빌드 통과 |
| 8 | 통합 검증·문서 갱신 | V2 검증 보고서 작성. 실물 미검증 항목 구분 |

측정 타이밍과 핀은 구조 변경과 동시에 바꾸지 않는다. 변경 전 V0.1 OUT/HEX는 고정 기준으로 유지하고 V2 결과는 별도 경로에 보관한다.

## 10. V2 추가 시험

- 순수 처리: 평균·포화·부호·이득 경계, 히스테리시스 경계와 연속 확인 초기화.
- 설정: 프레임 처리 중 예약값 변경, 잘못된 값 거부, 버전 일관성.
- 버퍼: 이중 공개/해제, READY 덮어쓰기, 정지 중 미완성 폐기.
- 상태: 각 상태의 일반/즉시 정지, 타임아웃 후 CLEAR 조건, 늦은 IRQ 무시.
- 파서: 음수/숫자 오버플로/추가 인수/빈 줄/CRLF/오버런 후 복구.
- 응답: 송신 버퍼 부족, 연속 측정 중 RESULT의 한 프레임 일관성.
- 통합: 모의 HAL 이벤트 로그로 주소 변경 전 Tx OFF, DAC 변경 시 Tx OFF, 채널 0~19 순서 확인.

최종 보고서는 소프트웨어 검사와 실제 주변장치/아날로그 실측을 구분한다. 함수 수나 assertion 개수를 품질 목표로 삼지 않는다.
