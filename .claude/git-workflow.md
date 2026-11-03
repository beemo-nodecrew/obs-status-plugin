---
repo: beemo-nodecrew/obs-status-plugin
branch: main
check:
tag:
---

## 커밋 메시지

```text
<type>: <요약>

- <바꾼 이유나 내용>
```

### 제목

- `type: 요약` 한 줄로 쓴다. 범위는 쓰지 않는다(기존 이력과 동일).
- 요약은 한국어로 50자 이내로 쓰고, 명사형으로 끝내며 마침표를 찍지 않는다.
- 마일스톤 작업이면 요약 앞에 `M<번호>`를 붙인다. 예: `feat: M5 마이크 음소거 아이콘 표시`

### 본문

- 이유가 diff만 보고 드러나지 않을 때 제목 아래 한 줄을 비우고 `- ` 목록 1~5개로 쓴다.
- 제목만으로 충분한 변경이면 본문을 생략한다.

### type

| type | 쓰는 경우 |
| --- | --- |
| `feat` | 새 기능 (마일스톤 M0~M6 포함) |
| `fix` | 버그 수정 |
| `refactor` | 동작은 같고 구조만 바꾼 변경 |
| `docs` | README, `docs/PLAN.md` 같은 문서 |
| `chore` | 빌드·CI·설정, `.gitignore`, `.claude/git-workflow.md` |

판단이 애매하면 주된 변경을 기준으로 고른다.

### 예시

```text
feat: M6 Windows 설치 스크립트 패키징

- install.cmd / uninstall.cmd / install.ps1 추가 (관리자 권한 자동 요청, OBS 실행 중 차단)
- Windows 패키지 zip에 설치 스크립트 포함
```

```text
fix: 일시정지 배지에서 마이크 음소거 사선이 보이지 않던 문제 수정
```

## 저장소 규칙

- `src/`의 C/C++ 파일을 바꿨으면 push 전에 clang-format 19.1.1로 정리한다(`uvx --from clang-format==19.1.1 clang-format -i <파일>`). CI의 check-format이 같은 버전으로 검사한다.
- 로컬에 빌드 도구가 없으므로 빌드 확인은 `main` push 후 GitHub Actions 결과로 한다.
