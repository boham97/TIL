# 리눅스 / Vim CLI 유틸리티

> 2026-08-13 · 단발성 조회 질문 2건

---

## 1. Vim에서 파일 라인 수 확인

**질문:** Vim으로 연 파일의 총 라인 수를 어떻게 보는가?

**정리**

| 방법 | 설명 |
|---|---|
| `Ctrl + g` | 상태줄에 현재 라인/전체 라인 표시. **가장 빠름** |
| `g Ctrl + g` | 라인·단어·문자·바이트 수까지 상세 통계 |
| `:set number` → `G` | 라인 번호 켜고 마지막 줄로 이동해 직접 확인 |
| `:%s/^//n` | 치환 카운트만 수행(`n` 플래그)해 매칭 라인 수 출력 |
| `wc -l 파일명` | Vim 밖 셸에서 확인 |

**주의:** `wc -l`은 개행 문자 개수를 세므로, 마지막 줄에 개행이 없으면 Vim 표시보다 1 적게 나옵니다.

**근거**
- Vim 공식 문서 (파일 정보 표시): https://vimhelp.org/editing.txt.html#CTRL-G
- GNU Coreutils `wc`: https://www.gnu.org/software/coreutils/manual/html_node/wc-invocation.html

---

## 2. 리눅스 `date`로 유닉스 타임 조회

**질문:** `date` 명령으로 유닉스 타임스탬프를 다루는 방법.

**정리**

```bash
date +%s                          # 현재 시각 → 유닉스 타임(초)
date +%s%3N                       # 밀리초 정밀도
date -d "2026-08-13 09:00:00" +%s # 특정 시각 → 유닉스 타임
date -d @1755100000               # 유닉스 타임 → 사람이 읽는 형식
date -u +%s                       # UTC 기준
date -d "yesterday" +%s           # 상대 표현
```

**핵심 주의점 — 이식성**

`-d` 옵션은 **GNU date(리눅스 기본)** 전용입니다. BSD date(macOS)에서는 동작하지 않고 `-r`, `-v` 등 다른 문법을 씁니다. 스크립트를 두 플랫폼에서 같이 돌린다면 이 부분이 깨집니다.

**근거**
- GNU Coreutils `date` 문서: https://www.gnu.org/software/coreutils/manual/html_node/date-invocation.html
- 날짜 입력 포맷(`-d`가 받는 문자열): https://www.gnu.org/software/coreutils/manual/html_node/Date-input-formats.html
