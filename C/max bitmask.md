// 기존 방식
int max = (a > b) ? a : b;

// 비트 연산으로 대체
int diff = a - b;
int mask = -(diff >> 31);  // a > b이면 mask = 0, a < b이면 mask = -1
int max = a - (diff & mask);

//분기 예측 실패가 줄어 든다

```
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
```

분기 예측 힌트를 줄수도있음!