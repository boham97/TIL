PostgreSQL의 `date_trunc` 함수 사용법을 설명해드릴게요.

## `date_trunc` 기본 문법

```sql
date_trunc('field', source)
```

-----

## 주요 사용 예시

### 연/월/일/시간 단위로 자르기

```sql
-- 연도
SELECT date_trunc('year', NOW());
-- 결과: 2026-01-01 00:00:00

-- 월
SELECT date_trunc('month', NOW());
-- 결과: 2026-05-01 00:00:00

-- 일
SELECT date_trunc('day', NOW());
-- 결과: 2026-05-07 00:00:00

-- 시간
SELECT date_trunc('hour', NOW());
-- 결과: 2026-05-07 14:00:00

-- 분
SELECT date_trunc('minute', NOW());
-- 결과: 2026-05-07 14:32:00
```

-----

### 주(week) 단위

```sql
SELECT date_trunc('week', NOW());
-- 결과: 2026-05-04 00:00:00 (해당 주 월요일)
```

-----

### 분기(quarter) 단위

```sql
SELECT date_trunc('quarter', NOW());
-- 결과: 2026-04-01 00:00:00
```

-----

## 실무 활용 예시


### 일별 사용자 집계

```sql
SELECT
  date_trunc('day', created_at) AS day,
  COUNT(*) AS user_count
FROM users
GROUP BY date_trunc('day', created_at)
ORDER BY day;
```

### 특정 기간 필터링 (이번 달 데이터)

```sql
SELECT *
FROM orders
WHERE order_date >= date_trunc('month', NOW())
  AND order_date < date_trunc('month', NOW()) + INTERVAL '1 month';
```

-----

## 지원하는 field 값

|field         |설명        |
|--------------|----------|
|`microseconds`|마이크로초     |
|`milliseconds`|밀리초       |
|`second`      |초         |
|`minute`      |분         |
|`hour`        |시         |
|`day`         |일         |
|`week`        |주 (월요일 기준)|
|`month`       |월         |
|`quarter`     |분기        |
|`year`        |연도        |
|`decade`      |10년       |
|`century`     |세기        |
