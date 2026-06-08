# 컴파일 타임 assertion: static_assert vs assert_int_is_4

## C11 이전: assert_int_is_4 트릭

C11 이전엔 컴파일 타임 검사를 위한 표준 방법이 없었습니다. 그래서 이런 트릭을 썼습니다:

```c
typedef char assert_int_is_4[(sizeof(int) == 4) ? 1 : -1];
```

동작 원리:
- 조건이 참이면 → `typedef char assert_int_is_4[1]` → 정상 컴파일
- 조건이 거짓이면 → `typedef char assert_int_is_4[-1]` → 크기가 음수인 배열 → 컴파일 오류

단점:
- 문법이 직관적이지 않음
- 오류 메시지가 불친절함 (`array size is negative` 같은 메시지)
- 이름을 매번 다르게 지어야 함 (중복 정의 방지)

```c
// 여러 개 쓸 때 이름 충돌 피하려고 이렇게 씀
typedef char assert_sizeof_int[(sizeof(int) == 4) ? 1 : -1];
typedef char assert_sizeof_long[(sizeof(long) == 8) ? 1 : -1];
typedef char assert_sizeof_ptr[(sizeof(void*) == 8) ? 1 : -1];
```


## C11: static_assert

C11에서 `_Static_assert` 키워드가 추가됐고, `<assert.h>` 를 포함하면 `static_assert` 로 쓸 수 있습니다.

```c
#include <assert.h>

static_assert(sizeof(int) == 4, "int must be 4 bytes");
```

조건이 거짓이면 컴파일 오류와 함께 메시지를 출력합니다:

```
error: static assertion failed: "int must be 4 bytes"
```


## runtime assert와 비교

```c
#include <assert.h>

// 런타임 assert: 프로그램 실행 중에 검사
assert(sizeof(int) == 4);         // 실행해야 알 수 있음, 실패 시 abort()

// 컴파일 타임 assert: 빌드 중에 검사
static_assert(sizeof(int) == 4, "int must be 4 bytes");  // 빌드할 때 바로 오류
```

`sizeof` 같이 컴파일 타임에 알 수 있는 값은 `static_assert`로 검사하는 게 좋습니다.
런타임까지 기다릴 필요가 없으니까요.


## 실전 사용 예시

```c
#include <assert.h>
#include <stdatomic.h>

#define QUEUE_SIZE 1024

typedef struct {
    char a;
    char pad[3];  // 명시적 패딩
    int b;
} MyData;

// 구조체 크기가 예상과 다르면 빌드 실패
static_assert(sizeof(MyData) == 8, "unexpected padding in MyData");

// atomic_int가 lock-free가 아니면 빌드 실패
static_assert(ATOMIC_INT_LOCK_FREE == 2, "atomic_int must be lock-free");

// 큐 크기가 2의 배수가 아니면 빌드 실패
static_assert(QUEUE_SIZE % 2 == 0, "queue size must be even");
```


## 정리

| | assert_int_is_4 트릭 | static_assert |
|---|---|---|
| 표준 | 비표준 (트릭) | C11 표준 |
| 오류 메시지 | 불친절 | 명시적 메시지 가능 |
| 가독성 | 나쁨 | 좋음 |
| 사용 가능 위치 | 파일 스코프 | 파일/함수 스코프 |