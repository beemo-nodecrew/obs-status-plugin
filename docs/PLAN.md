# OBS 녹화 상태 오버레이 플러그인 개발 지침

Nov 3, 2026 · @정우영

## 목적

- 쉬는 시간 후 강의 재개 시 녹화 상태(녹화 중 / 일시정지 / 정지)와 마이크 음소거 상태를 실제 모니터 위에 표시
- "녹화 재개 누락", "마이크 미연결" 실수 방지
- 오버레이는 모니터에는 보이되 OBS 녹화본에는 찍히지 않아야 함

## 사용자 환경

| 모니터 | 용도 | 비고 |
| --- | --- | --- |
| 1번 | 강사 작업 화면 | 오버레이 대상 아님 |
| 2번 | 녹화 대상 화면 | OBS 디스플레이 캡처 대상으로 추정 |
| 3번 | 강의장 교육생용 화면 | 2번과 복제(Duplicate) 구성 |

- 2·3번 복제 구성 → Windows 입장에서는 하나의 화면(QScreen 1개)
- 2번에 띄운 오버레이는 3번에도 그대로 보임 → 2번에만 따로 표시 불가 (수용된 제약)
- OS: Windows 10 2004 이상 (확인됨)
- 2번 화면 내용: 주로 웹 브라우저 (실제 화면, OBS 프로젝터 아님, 확인됨)

## 범위

포함

- 녹화 상태 오버레이 (핵심)
- 마이크 음소거 상태 아이콘 (상태 표시만, 경고·소리 로직 없음)
- 설정 UI: 표시 모니터, 마이크 소스, 배지 위치·크기·문구, REC 표시 여부

제외 (사용자가 불필요하다고 판단)

- 녹화 중 마이크 무음 감지·경고
- 일시정지 중 발화 감지(VAD)

## 상태별 표시 사양 (초안, 조정 가능)

| 상태 | 표시 |
| --- | --- |
| 녹화 중 | 모서리 작은 "● REC" (설정으로 숨김 가능) |
| 일시정지 | 화면 테두리 빨간 프레임 + "녹화 일시정지" 배지 |
| 정지(미녹화) | 노란 테두리 + "녹화 안 함" 배지 (일시정지 대신 중지를 누른 경우 대비) |
| 마이크 음소거 | 배지 옆 음소거 마이크 아이콘 (모든 상태에서 표시) |

- 교육생에게도 보이므로 문구는 짧고 중립적으로, 크기 조절 옵션 제공

## 기술 스택

- C++17, CMake, Qt6 (OBS 내장 Qt 사용)
- 시작점: obsproject/obs-plugintemplate
- 대상: 최신 OBS Studio 안정 버전 33.0.1 (2026-11-03 기준). 빌드 의존성은 템플릿 고정값 31.1.1 사용, 사용자 OBS 버전 확인 후 조정
- 사용자 OBS 버전: 미확인 (추후 확인)
- 빌드: Windows + Visual Studio 2022
- 개발·테스트는 OBS가 설치된 Windows PC에서 진행 (오버레이·캡처 제외 동작은 실기 테스트 필수)

## 구현 설계

### 1. 녹화 상태 감지

- `obs_frontend_add_event_callback` 등록
  - `OBS_FRONTEND_EVENT_RECORDING_STARTED / STOPPED / PAUSED / UNPAUSED`
  - `OBS_FRONTEND_EVENT_FINISHED_LOADING`: 초기 상태 반영 및 오버레이 생성
  - `OBS_FRONTEND_EVENT_EXIT`: 정리
- 초기 상태: `obs_frontend_recording_active()`, `obs_frontend_recording_paused()`

### 2. 마이크 음소거 감지

- 설정에서 선택한 소스 이름으로 `obs_get_source_by_name` → weak source로 보관
- `obs_source_get_signal_handler` + `"mute"` 시그널 연결, 초기값은 `obs_source_muted()`
- 소스 이름 변경·삭제 대응: 전역 시그널 `source_rename`, `source_remove`
- 시그널은 다른 스레드에서 올 수 있으므로 UI 갱신은 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`으로 메인 스레드에서 처리

### 3. 오버레이 창

- `QWidget` 플래그: `Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus`
- 속성: `Qt::WA_TranslucentBackground`, `Qt::WA_ShowWithoutActivating`
- 선택한 `QScreen`의 geometry에 맞춰 전체 덮기, `paintEvent`에서 테두리·배지 그리기
- 모니터 식별: `QScreen::name()` 저장 + 실패 시 인덱스 fallback
- 화면 추가·제거 시 재배치: `QGuiApplication::screenAdded / screenRemoved`
- Alt+Tab·작업 표시줄 비노출 (`Qt::Tool`)

### 4. 녹화본 제외 (핵심 리스크)

- `SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE)` (Windows 10 2004 이상)
- 반환값 실패 시 로그 + 설정 UI에 경고 표시
- 검증 필요: Qt 투명(레이어드) 창과 함께 정상 동작하는지

### 5. 최상위 유지

- 웹 브라우저 전체 화면(F11), PPT 슬라이드쇼 등 다른 최상위 창에 가려질 가능성
- `QTimer` 1\~2초 주기로 `SetWindowPos(hwnd, HWND_TOPMOST, ..., SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE)` 재지정
- 포커스를 빼앗지 않는지 확인

### 6. 설정

- 메뉴: `obs_frontend_add_tools_menu_qaction`으로 "도구 > 녹화 상태 오버레이" 추가
- 저장: `obs_module_config_path` 경로의 JSON (`obs_data_t`)
- 항목: 표시 모니터, 마이크 소스, 배지 위치(4모서리/상단 중앙), 배지 크기, 문구, REC 표시 여부, 테두리 두께

## 마일스톤

각 단계 완료 시 빌드·실행 확인 후 다음 단계로 진행.

1. M0: obs-plugintemplate로 빈 플러그인 빌드·로드 확인 (OBS 로그에 로드 메시지)
2. M1: 녹화 상태 이벤트 수신 → 로그 출력
3. M2: 고정 모니터에 상태별 오버레이 표시
4. M3: 캡처 제외 + 최상위 유지 적용
5. M4: 설정 UI (모니터·마이크 소스·스타일)
6. M5: 마이크 음소거 아이콘
7. M6: 패키징 (설치용 zip 또는 인스톨러) + 사용자 실사용 테스트

## 테스트 체크리스트

- [ ] 녹화 시작 → 일시정지 → 재개 → 정지 순서별 표시 전환
- [ ] OBS 디스플레이 캡처 방식 2종(DXGI 데스크톱 복제 / Windows 10 캡처)에서 오버레이가 녹화본에 안 찍히는지
- [ ] 녹화 중 "● REC" 표시가 녹화본에 안 남는지
- [ ] 복제된 3번 모니터에도 동일하게 보이는지
- [ ] 웹 브라우저 전체 화면(F11) 위에서 가려지지 않는지
- [ ] PPT 슬라이드쇼 위에서 가려지지 않는지
- [ ] 오버레이가 마우스 클릭·키보드 포커스를 가로채지 않는지
- [ ] 마이크 음소거 토글 즉시 반영, 소스 이름 변경·삭제 시 오류 없음
- [ ] 모니터 연결 해제·재연결, DPI 배율 변경 시 위치 정상
- [ ] OBS 종료 시 크래시·잔여 창 없음

## 사용자에게 먼저 확인할 것

1. Windows 사용 여부 및 버전: Windows 10 2004 이상 (확인됨)
2. 2번 모니터에 띄우는 화면 종류: 주로 웹 브라우저 (확인됨)
3. 사용 중인 OBS 버전, 마이크 소스 이름: 미확인, 추후 확인 후 기입

## 작업 방식

- 사용자 응답 언어: 한국어, 간결한 개조식(명사형 어미)
- 마일스톤 단위로 진행하고 각 단계 완료 시 빌드·실행 확인 후 다음 단계로
- 브랜치 분리 없이 `main`에서 개발, 작업 단위마다 커밋·푸시
