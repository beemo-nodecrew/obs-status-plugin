# OBS Status Overlay

OBS 녹화 상태(녹화 중 / 일시정지 / 정지)와 마이크 음소거 상태를 실제 모니터 위에 표시하는 OBS 플러그인입니다. 오버레이는 모니터에는 보이지만 OBS 녹화본에는 찍히지 않습니다.

- 개발 지침: [docs/PLAN.md](docs/PLAN.md)
- 기반: [obsproject/obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate)
- 대상 OS: Windows 10 2004 이상

## 진행 상황

| 마일스톤 | 내용 | 상태 |
| --- | --- | --- |
| M0 | 빈 플러그인 빌드·로드 확인 | 코드 완료, 실기 확인 대기 |
| M1 | 녹화 상태 이벤트 수신 → 로그 | 코드 완료, 실기 확인 대기 |
| M2 | 고정 모니터에 상태별 오버레이 | 코드 완료, 실기 확인 대기 |
| M3 | 캡처 제외 + 최상위 유지 | 코드 완료, 실기 확인 대기 |
| M4 | 설정 UI | 코드 완료, 실기 확인 대기 |
| M5 | 마이크 음소거 아이콘 | 코드 완료, 실기 확인 대기 |
| M6 | 패키징 + 실사용 테스트 | 패키징 완료, 실사용 테스트 대기 |

## 빌드 (Windows)

준비물: Visual Studio 2022 (C++ 데스크톱 개발), CMake 3.28 이상, Git

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64
cmake --install build_x64 --prefix release --config RelWithDebInfo
```

처음 configure할 때 OBS 소스, obs-deps, Qt6를 자동으로 받습니다 (`buildspec.json`).

`main`에 푸시하면 GitHub Actions가 Windows·macOS·Ubuntu 빌드를 실행합니다. Windows 결과물은 Actions 실행 페이지의 Artifacts에서 받을 수 있습니다.

## 설치 (Windows)

1. GitHub Actions 최신 `main` 실행 페이지 → Artifacts에서 `obs-status-overlay-<버전>-windows-x64-<커밋>` 다운로드 후 압축 해제
2. OBS 종료
3. `install.cmd` 실행 (관리자 권한 요청 → 허용)
   - 설치 위치: `C:\ProgramData\obs-studio\plugins\obs-status-overlay\`
   - 제거: `uninstall.cmd`
4. OBS 실행 후 `도움말 > 로그 파일 > 현재 로그 보기`에서 아래 줄 확인

```
[obs-status-overlay] plugin loaded successfully (version 0.1.0)
[obs-status-overlay] capture exclusion applied: WDA_EXCLUDEFROMCAPTURE
```

수동 설치: 압축 안의 `obs-status-overlay` 폴더를 `C:\ProgramData\obs-studio\plugins\`에 복사

## 사용법

- 메뉴: `도구 > 녹화 상태 오버레이`
- 기본값: 주 모니터, 아래 가운데(작업 표시줄 위치) 캡슐, 글자 14px, 일시정지 테두리 6px
- 처음 설정할 것: 표시 모니터를 강사가 보는 모니터로, 마이크 소스를 실제 마이크로 지정 후 `확인`
- 설정 파일: `%APPDATA%\obs-studio\plugin_config\obs-status-overlay\settings.json`

작업 표시줄 높이 안 가로 중앙에 어두운 캡슐 하나로 표시. 작업 표시줄이 아래에 없거나 자동 숨김이면 화면 맨 아래에서 12px 위에 표시

| 상태 | 캡슐 표시 | 화면 테두리 |
| --- | --- | --- |
| 녹화 중 | 빨간 점 + `녹화 중` + 경과 시간 (설정에서 끌 수 있음) | 없음 |
| 일시정지 | 주황 일시정지 아이콘 + `녹화 일시정지` + 경과 시간, 주황 외곽선 | 주황 테두리 |
| 정지 | 회색 사각형 + `녹화 안 함` (설정에서 끌 수 있음) | 없음 |
| 마이크 | 구분선 뒤 마이크 아이콘. 켜짐: 흰색 + 초록 점, 음소거: 빨간 사선 | - |

설정 창의 `녹화본 제외` 항목이 실패로 표시되면 오버레이가 녹화본에 찍힐 수 있으니 녹화 전 확인 필요

## M1 확인 방법

OBS에서 녹화 시작 → 일시정지 → 재개 → 정지 후 로그에 아래 줄이 순서대로 남는지 확인

```
[obs-status-overlay] initial recording state: stopped
[obs-status-overlay] recording state: stopped -> recording (started)
[obs-status-overlay] recording state: recording -> paused (paused)
[obs-status-overlay] recording state: paused -> recording (unpaused)
[obs-status-overlay] recording state: recording -> stopped (stopped)
```

## 라이선스

GPL-2.0 (OBS 플러그인 템플릿 라이선스 승계)
