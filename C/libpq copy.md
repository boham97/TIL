# TIL: PostgreSQL COPY 사용기

libpq로 Oracle v$sql 데이터를 PostgreSQL에 COPY로 적재하면서 겪은 시행착오 정리.

## TEXT 포맷의 함정

기본 `COPY ... FROM STDIN` 은 TEXT 포맷이고, 다음 문자들이 특수문자로 처리된다.

| 문자 | 의미 |
|------|------|
| `\t` | 컬럼 구분자 (DELIMITER) |
| `\n` | 행 구분자 |
| `\\` | 역슬래시 이스케이프 |
| `\N` | NULL |
| `\.` | end-of-copy 마커 (단독 라인) |

데이터에 위 문자들이 들어오면 직접 이스케이프해야 한다. 안 하면 다음과 같은 에러:

```
COPY failed: ERROR:  end-of-copy marker corrupt
```

특히 윈도우 경로 (`C:\path\to\file`) 가 들어오면 역슬래시 때문에 자주 깨진다.

## TEXT 포맷에서 이스케이프 하기

C에서 직접 이스케이프 처리:

```c
void escape_for_copy(const char *src, char *dst, size_t dst_size)
{
    size_t i, j;

    for (i = 0, j = 0; src[i]; i++)
    {
        if (src[i] == '\\' || src[i] == '\t' || src[i] == '\n')
        {
            if (j + 2 >= dst_size) break;
            if (src[i] == '\\')      { dst[j++] = '\\'; dst[j++] = '\\'; }
            else if (src[i] == '\t') { dst[j++] = '\\'; dst[j++] = 't';  }
            else                     { dst[j++] = '\\'; dst[j++] = 'n';  }
        }
        else
        {
            if (j + 1 >= dst_size) break;
            dst[j++] = src[i];
        }
    }
    dst[j] = '\0';
}
```

성능을 신경쓴다면 `strchr` 로 역슬래시 위치 찾고 `memcpy` 로 한번에 복사하는 방식도 있다.

## Oracle 쿼리에서 처리하면 안 되는 이유

처음엔 Oracle 쪽에서 REPLACE로 처리하려 했다:

```sql
REPLACE(NVL(max(a.module), '\N'), '\', '\\') as module
```

근데 이러면 `\.v` → `\\.v` 로 바뀌어도 PostgreSQL COPY 파서는 `\\` 를 이스케이프된 `\` 한 개로 해석해서 결국 `\.` 로 복원된다. **클라이언트 측(C)에서 이스케이프하거나 포맷 자체를 바꿔야 한다.**

## 더 나은 해법: CSV 포맷

CSV 포맷으로 바꾸면 역슬래시가 특수문자가 아니다. 이스케이프 함수 자체가 필요없어짐:

```sql
COPY table_name FROM STDIN
WITH (
    FORMAT CSV,
    DELIMITER E'\t',
    QUOTE '"',
    ESCAPE '"',
    NULL '\N'
);
```

각 옵션 의미:

- `FORMAT CSV` — CSV 파싱 규칙 사용, `\` 특수문자 아님
- `DELIMITER E'\t'` — 컬럼 구분자 탭
- `QUOTE '"'` — 큰따옴표로 문자열 감쌀 수 있음
- `ESCAPE '"'` — CSV 내부 `"` 이스케이프를 `"` 로 (기본 `\` 대신). 역슬래시 특수기능 완전 비활성화
- `NULL '\N'` — `\N` 을 NULL로 인식 (안 지정하면 빈 문자열이 NULL이 됨)

C 코드에서 문자열 인자 부분도 깔끔해진다:

```c
// 기존 (TEXT 포맷)
data[i].module[0] ? (char *)data[i].module : "\\N",

// CSV 포맷 + NULL '\N'
(char *)data[i].module,
```

## CSV 포맷의 한계

- 데이터에 **탭이 들어오면** 여전히 깨진다 (구분자라서)
  - `module`, `action` 같은 컬럼에 탭 들어올 가능성 있으면 큰따옴표로 감싸서 보내야 함
  - 가능성 낮으면 그냥 두고 가도 OK
- 데이터에 **큰따옴표가 들어오면** 문자열 감쌀 때 `""` 로 이스케이프 필요

## libpq COPY 흐름

```c
// 1. COPY 시작
PQexec(conn, "COPY ... FROM STDIN WITH (...)");
// PGRES_COPY_IN 확인

// 2. 행 단위로 데이터 전송
for (i = 0; i < n; i++) {
    int len = snprintf(buf, sizeof(buf), "%s\t%s\n", ...);
    if (len < 0 || len >= (int)sizeof(buf)) {
        PQputCopyEnd(conn, "error");  // 에러 메시지로 중단
        return -1;
    }
    PQputCopyData(conn, buf, len);
}

// 3. COPY 종료
PQputCopyEnd(conn, NULL);  // NULL = 정상 종료

// 4. 결과 확인
res = PQgetResult(conn);
if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "COPY failed: %s\n", PQerrorMessage(conn));
}
```

## 디버깅 팁

데이터가 깨질 때 바이트 단위로 까서 보면 빠르다:

```bash
# 의심되는 행을 ASCII 번호로 출력
printf '1001\t...' | od -A x -t x1z

# 탭 개수 확인 = 컬럼수 - 1
printf '...' | tr -cd '\t' | wc -c
```

자주 보게 되는 ASCII:

- `\t` = 9 (0x09) — 탭 (구분자)
- `\n` = 10 (0x0a) — 개행
- `\` = 92 (0x5c) — 역슬래시
- `0` = 48, `-` = 45 — 숫자값에 공백 패딩 들어왔는지 확인용

## 언제 COPY가 아니라 INSERT를 쓸까

- **100개 미만의 작은 batch**: 멀티로우 INSERT가 비슷하거나 빠를 수 있음
- **이스케이프가 복잡한 데이터**: `PQexecParams` 의 파라미터 바인딩이 특수문자 자동 처리
- **batch 반복이 많고 행 수가 큼**: COPY가 압도적으로 빠름

본인 케이스(100개씩 반복)는 COPY + CSV 포맷이 합리적이었다.

## 결론

- TEXT 포맷은 함정이 많다. 가능하면 **CSV 포맷 + 옵션 조합** 으로 가자.
- Oracle 쿼리에서 이스케이프 시도하지 말 것. 클라이언트 또는 포맷 옵션으로 처리.
- 깨지면 바이트 단위로 까보면 원인이 보통 보인다.