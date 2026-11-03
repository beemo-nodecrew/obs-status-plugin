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
| M2 | 고정 모니터에 상태별 오버레이 | 예정 |
| M3 | 캡처 제외 + 최상위 유지 | 예정 |
| M4 | 설정 UI | 예정 |
| M5 | 마이크 음소거 아이콘 | 예정 |
| M6 | 패키징 + 실사용 테스트 | 예정 |

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

1. OBS 종료
2. 빌드 결과의 `obs-status-overlay` 폴더를 `C:\ProgramData\obs-studio\plugins\`에 복사
   - 결과 구조: `C:\ProgramData\obs-studio\plugins\obs-status-overlay\bin\64bit\obs-status-overlay.dll`
3. OBS 실행 후 `도움말 > 로그 파일 > 현재 로그 보기`에서 아래 줄 확인

```
[obs-status-overlay] plugin loaded successfully (version 0.1.0)
```

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
