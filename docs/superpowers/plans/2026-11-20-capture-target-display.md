# 녹화 화면·마이크 이름 표시 구현 계획

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 설정에서 고른 디스플레이 캡처 소스의 모니터 이름과 마이크 장치 이름을 캡슐에 항상 표시하고, 찾지 못하면 빨간 `찾을 수 없음`으로 표시

**Architecture:** OBS 소스 속성(`obs_source_properties`)의 목록 속성에서 현재 설정값과 같은 항목의 문구를 읽는 공용 함수(`source-label`)를 두고, 화면은 새 `CaptureMonitor`, 마이크는 기존 `MicMonitor`가 이 함수로 이름을 만든다. `OverlayController`가 2초 타이머와 이벤트 때마다 두 이름을 읽어 `OverlayWindow`에 넘기고, 창은 캡슐에 화면 구간과 마이크 이름을 그린다.

**Tech Stack:** C++17, Qt6, libobs / obs-frontend-api, CMake (obs-plugintemplate)

**Spec:** `docs/superpowers/specs/2026-11-20-capture-target-display-design.md`

## Global Constraints

- 캡슐 모양: `● 녹화 중 00:12:34 │ 🖥 DELL U2720Q │ 🎤 USB 마이크`
- 캡처 소스 미지정 시 화면 구간 숨김
- 모니터 문구는 `:` 앞부분만 사용, 이름은 24자 넘으면 `…`로 줄임
- `찾을 수 없음`은 `kRecRed`(239, 68, 68)로 표시
- 재확인 주기 2000ms
- 설정 파일 키 `capture_source`, 설정 창 항목 `녹화 화면 소스`는 마이크 소스 행 위
- 코드 형식: clang-format 19.1.1 (`.clang-format`, 탭 들여쓰기, 120열). CI `check-format`이 검사함
- 로컬에 CMake·Visual Studio가 없음. 빌드 확인은 `main` push 후 GitHub Actions `Build Project`로 함 (push는 사용자 승인 후)
- 자동 테스트 체계 없음. 각 작업 확인 = clang-format 통과 + CI 빌드 + 마지막 작업의 실기 확인
- 커밋 메시지: `.claude/git-workflow.md` 형식 (`feat: 요약`, 한국어 명사형, 마침표 없음)

## Review Focus

1. 소스 크기가 0이 아닌데 OBS가 목록에 저장값을 유지하는 경우(모니터 재연결 후 ID 변경): 목록에 같은 값이 없으므로 `찾을 수 없음` → Task 2 `list_property_label` 비교 로직
2. OBS가 저장값을 목록에 "사용 불가" 항목으로 남기는 경우: 비활성(disabled) 항목이면 `찾을 수 없음` → Task 2
3. 지정한 캡처 소스를 삭제·이름 변경: 삭제면 `찾을 수 없음`, 이름 변경이면 설정값이 새 이름으로 갱신 → Task 2
4. 마이크 소스가 장치 목록 속성이 없는 종류(예: 미디어 소스): OBS 소스 이름 표시 → Task 2
5. 2초마다 `obs_source_properties` 호출로 인한 끊김: 실기에서 OBS 통계 창 프레임 드롭 확인 → Task 4

---

### Task 1: 설정 항목 추가

**Files:**
- Modify: `src/overlay-settings.hpp` (`micSource` 위)
- Modify: `src/settings-store.cpp` (기본값·로드·저장)
- Modify: `src/settings-dialog.hpp`, `src/settings-dialog.cpp`
- Modify: `data/locale/ko-KR.ini`, `data/locale/en-US.ini`

**Interfaces:**
- Produces: `OverlaySettings::captureSource` (`QString`, 기본 빈 문자열 = 사용 안 함)

- [ ] **Step 1:** `OverlaySettings`에 `QString captureSource;` 추가
- [ ] **Step 2:** `settings-store.cpp`의 `set_defaults`, `load_overlay_settings`, `save_overlay_settings`에 `"capture_source"` 키 추가. `kSettingsVersion`은 그대로 2 (새 키는 기본값으로 충분)
- [ ] **Step 3:** 설정 창에 `QComboBox *capture_` 추가, `populateCaptureSources(const QString &selected)` 구현
  - 첫 항목 `Settings.CaptureNone`(빈 데이터), 이후 `obs_enum_sources`로 `obs_source_get_unversioned_id(source) == "monitor_capture"`인 소스 이름을 정렬해 추가
  - 저장값이 목록에 없으면 맨 앞에 추가 (마이크 콤보와 같은 방식)
  - 폼에서 `Settings.MicSource` 행 바로 위에 `Settings.CaptureSource` 행으로 배치
  - `load`에서 채우고 `collect`에서 `s.captureSource` 설정 (첫 항목이면 빈 문자열)
- [ ] **Step 4:** 문구 추가

  ko-KR.ini
  ```ini
  Settings.CaptureSource="녹화 화면 소스"
  Settings.CaptureNone="(사용 안 함)"
  Overlay.NotFound="찾을 수 없음"
  ```
  en-US.ini
  ```ini
  Settings.CaptureSource="Recorded display source"
  Settings.CaptureNone="(None)"
  Overlay.NotFound="Not found"
  ```
- [ ] **Step 5:** 형식 검사 (도구 준비는 한 번만)

  ```bash
  py -m venv "$SCRATCH/cf" && "$SCRATCH/cf/Scripts/pip" install clang-format==19.1.1
  "$SCRATCH/cf/Scripts/clang-format" --dry-run --Werror src/*.cpp src/*.hpp
  ```
  Expected: 출력 없음, 종료 코드 0. 실패하면 `-i`로 고친 뒤 다시 확인
- [ ] **Step 6:** 커밋 `feat: 녹화 화면 소스 설정 항목 추가`

### Task 2: 화면·마이크 이름 읽기

**Files:**
- Create: `src/source-label.hpp`, `src/source-label.cpp`
- Create: `src/capture-monitor.hpp`, `src/capture-monitor.cpp`
- Modify: `src/mic-monitor.hpp`, `src/mic-monitor.cpp`
- Modify: `src/overlay-controller.hpp`, `src/overlay-controller.cpp`
- Modify: `CMakeLists.txt` (`target_sources`에 새 파일 4개, `mic-monitor` 다음)

**Interfaces:**
- Consumes: `OverlaySettings::captureSource`
- Produces:
  ```cpp
  // source-label.hpp
  struct SourceLabel {
  	bool visible = false; // false면 캡슐에서 구간 숨김
  	bool found = false;   // false면 빨간색
  	QString text;         // 표시 문구 (found=false면 Overlay.NotFound 문구)
  	bool operator==(const SourceLabel &o) const;
  	bool operator!=(const SourceLabel &o) const;
  };
  enum class ListLabel { Found, Missing, NoProperty };
  // prop 목록에서 소스 설정의 현재 값과 같은 활성 항목을 찾아 *label에 문구 저장
  ListLabel list_property_label(obs_source_t *source, const char *prop, QString *label);
  QString trim_monitor_label(const QString &label); // ':' 앞부분, 앞뒤 공백 제거
  QString elide_label(const QString &label, int maxChars = 24); // 넘으면 maxChars-1자 + '…'
  QString not_found_text(); // obs_module_text("Overlay.NotFound")

  // capture-monitor.hpp
  class CaptureMonitor : public QObject {
  	Q_OBJECT
  public:
  	explicit CaptureMonitor(QObject *parent = nullptr);
  	~CaptureMonitor() override;
  	void setSourceName(const QString &name);
  	SourceLabel label() const;
  	void refresh(); // 다시 읽고, 바뀌었으면 labelChanged
  signals:
  	void labelChanged();
  	void sourceRenamed(const QString &newName);
  };

  // mic-monitor.hpp 추가
  SourceLabel MicMonitor::deviceLabel() const; // 소스 미지정이면 visible=false
  ```

- [ ] **Step 1:** `source-label.cpp` 구현
  - `list_property_label`: `obs_source_properties` → `obs_properties_get(props, prop)`이 없거나 목록 속성이 아니면 `NoProperty`. 소스 설정(`obs_source_get_settings`)의 값과 목록 항목을 형식(`obs_property_list_format`)에 맞춰 비교 (STRING: `obs_data_get_string` vs `obs_property_list_item_string`, INT: `obs_data_get_int` vs `obs_property_list_item_int`). 일치 항목이 있고 `obs_property_list_item_disabled`가 false면 `Found`, 아니면 `Missing`. 문자열 값이 비어 있으면 `Missing`. `obs_properties_destroy`, `obs_data_release` 잊지 말 것
- [ ] **Step 2:** `CaptureMonitor` 구현
  - `refresh()`: 이름이 비면 `visible=false`. `obs_get_source_by_name`으로 찾고 `obs_source_removed`면 없는 것으로 처리. 없으면 `{visible, !found, not_found_text()}`
  - 있으면 `list_property_label(source, "monitor_id", ...)`, `NoProperty`면 `"monitor"`(구버전 정수 설정)로 한 번 더. `NoProperty`가 계속되면 OBS 소스 이름을 found로 표시
  - `Found`여도 `obs_source_get_width == 0 || obs_source_get_height == 0`이면 `찾을 수 없음`
  - 표시 문구 = `elide_label(trim_monitor_label(label))`
  - 바뀌면 `labelChanged` emit + 로그 `capture source '%s': %s (found: yes/no, %ux%u)` (실기 확인용 크기 포함)
  - 전역 `source_rename` 시그널: `prev_name == name_`이면 `name_` 갱신 후 `sourceRenamed` (메인 스레드로 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`, `MicMonitor::onSourceRename`과 같은 방식). 소멸자에서 연결 해제
  - 소스 `update` 시그널은 연결하지 않음. 2초 재확인으로 대신함 (설계 문서와 다른 점, 최대 2초 지연)
- [ ] **Step 3:** `MicMonitor::deviceLabel()` 구현
  - 소스 미지정이면 `visible=false`. `weak_`가 없으면 `찾을 수 없음`
  - `list_property_label(source, "device_id", ...)`: `Found`면 `elide_label(label)`, `Missing`이면 `찾을 수 없음`, `NoProperty`면 `elide_label(name_)`
- [ ] **Step 4:** `OverlayController` 연결
  - 멤버 `CaptureMonitor *capture_`, `QTimer labelTimer_`(2000ms), `void updateLabels()`
  - `updateLabels()`: `capture_->refresh()` 후 `window_->setCaptureLabel(capture_->label())`, `window_->setMicLabel(mic_->deviceLabel())` (창 메서드는 Task 3에서 추가. 이 작업에서는 빌드가 되도록 Task 3의 두 setter를 빈 본문으로 먼저 선언·정의)
  - 호출 시점: `start()`, `applySettings()`(먼저 `capture_->setSourceName(settings_.captureSource)`), `refreshMic()`, `screenAdded`/`screenRemoved`, `mic_` `attachedChanged`, `labelTimer_` 만료
  - `capture_` `sourceRenamed` → `settings_.captureSource` 갱신 후 `save_overlay_settings`
  - `start()`에서 `labelTimer_.start()`, 소멸자에서 정지 후 `capture_` 삭제
- [ ] **Step 5:** Task 1 Step 5의 형식 검사 실행. Expected: 종료 코드 0
- [ ] **Step 6:** 커밋 `feat: 녹화 화면·마이크 장치 이름 읽기`

### Task 3: 캡슐에 화면·마이크 이름 그리기

**Files:**
- Modify: `src/overlay-window.hpp`, `src/overlay-window.cpp`

**Interfaces:**
- Consumes: `SourceLabel` (Task 2)
- Produces: `void OverlayWindow::setCaptureLabel(const SourceLabel &label)`, `void OverlayWindow::setMicLabel(const SourceLabel &label)` (값이 같으면 무시, 다르면 `update()`)

- [ ] **Step 1:** `PillLayout`에 `bool showCapture` 추가, `computeLayout` 너비 계산
  - `showCapture = captureLabel_.visible`
  - 구간 순서: 상태 → 시간 → 화면 → 마이크. 구간 사이 구분선 규칙은 기존과 같음(`gap*2 + 1`)
  - 화면 구간 너비 = `icon + gap + label.horizontalAdvance(captureLabel_.text)`
  - 마이크 구간: `micLabel_.visible`이면 `icon + gap + 이름 너비`, 아니면 기존처럼 아이콘만
  - 이름 글꼴은 `make_font(fontPx, QFont::Medium)`
  - `!showStatus && !showCapture && !showMic`이면 빈 레이아웃
- [ ] **Step 2:** `draw_monitor_icon(QPainter &, const QRectF &box, const QColor &color)` 추가: 화면 사각형(테두리만, 둥근 모서리, 높이 약 65%) + 아래 받침 선. 선 두께는 `draw_mic_icon`과 같은 계산식
- [ ] **Step 3:** `drawPill`에서 시간 다음에 화면 구간(구분선 → 아이콘 → 이름), 마이크 아이콘 뒤에 이름을 그림
  - 이름 색: `found`면 `kTextPrimary`, 아니면 `kRecRed`. 화면 아이콘 색도 같은 규칙
- [ ] **Step 4:** Task 1 Step 5의 형식 검사 실행. Expected: 종료 코드 0
- [ ] **Step 5:** 커밋 `feat: 캡슐에 녹화 화면·마이크 이름 표시`

### Task 4: 문서 반영과 실기 확인

**Files:**
- Modify: `README.md` (사용법의 상태 표, 처음 설정할 것)

- [ ] **Step 1:** README 상태 표에 `화면` 행 추가: `구분선 뒤 모니터 아이콘 + 캡처 중인 모니터 이름. 찾지 못하면 빨간 찾을 수 없음`. 마이크 행에 장치 이름 표시 추가. 처음 설정할 것에 `녹화 화면 소스` 지정 추가
- [ ] **Step 2:** 커밋 `docs: 녹화 화면·마이크 이름 표시 사용법 추가`
- [ ] **Step 3:** 사용자 승인 후 `git push`, GitHub Actions `Check Formatting`·`Build Project` 성공 확인 (`gh run watch`)
- [ ] **Step 4:** 사용자 실기 확인 (설계 문서의 실기 확인 항목)
  - 설정에서 녹화 화면 소스 지정 → 캡슐에 모니터 이름 표시
  - 서브 모니터 케이블 분리·재연결 → OBS 속성 창이 "찾을 수 없음"일 때 캡슐도 빨간 `찾을 수 없음`
  - 로그의 `capture source ...` 줄에서 그때 크기가 0인지 확인
  - 마이크 장치 분리 → 빨간 `찾을 수 없음`
  - 녹화 중 OBS 통계 창에서 프레임 드롭 없음
