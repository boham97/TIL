# PL/pgSQL 커서 - DECLARE / OPEN / FETCH 실행 시점

## 핵심 정리

| 단계 | 하는 일 |
|---|---|
| `DECLARE cur CURSOR FOR SELECT ...;` | 아무 실행도 안 함. 커서 이름과 쿼리 텍스트(+변수/파라미터 자리)만 바인딩하는 **선언**일 뿐, 파싱/플래닝 없음 |
| `OPEN cur;` | 실제로 파서·플래너가 동작해 **실행계획 생성**, 그 계획으로 **portal** 오픈. `EXPLAIN`으로 보는 그 플랜이 이 시점 산출물. 아직 로우는 안 가져옴 |
| `FETCH cur INTO ...;` | portal에서 executor가 실제로 동작 → **로우 산출**. 필요한 만큼만 lazy하게 실행 |

## 주의할 점

- "DECLARE 단계에서 실행계획만 확인한다"는 표현은 부정확함. 실행계획은 **DECLARE가 아니라 OPEN 시점**에 만들어짐.
- 예외: `Sort`, `Materialize`, CTE 등 일부 플랜 노드는 특성상 하위 입력을 먼저 구체화해야 해서, OPEN 이후 첫 FETCH 전에 일부 계산이 선행될 수 있음.
- 파라미터 바인딩 커서는 OPEN 시점에 실제 파라미터 값을 반영한 계획이 세워짐 (custom plan 대상).

## DECLARE에서 쓴 변수값은 OPEN 시점 값을 따라감

DECLARE는 선언일 뿐이라 그 시점의 변수값을 캡처하지 않음. 실제 값은 **OPEN이 실행되는 시점**의 변수값을 따라감.

```sql
DECLARE
  v_id int := 10;
  cur CURSOR FOR SELECT * FROM tbl WHERE id = v_id;
BEGIN
  v_id := 20;      -- DECLARE 이후 값 변경
  OPEN cur;         -- 이 시점의 v_id = 20이 쿼리에 반영됨 (10 아님)


  명시적 파라미터 방식도 동일한 원리:

DECLARE
  cur CURSOR (p_id int) FOR SELECT * FROM tbl WHERE id = p_id;
BEGIN
  OPEN cur(20);      -- 여기서 넘긴 20이 실행계획/실행에 쓰임
즉 DECLARE는 쿼리 텍스트 + 변수 자리만 기억해두는 거고, 실제 바인딩은 OPEN 시점에 일어남.