### inline
함수 호출을 인라인으로 처리하도록 컴파일러에 힌트를 줍니다.

인라인 함수는 호출하는 쪽에 코드가 그대로 삽입되어서, 함수 호출 오버헤드가 줄어듭니다.

`inline int add(int a, int b) {return a + b;}`

`int x = add(2,3); // → int x = 2 + 3;`


### static
함수가 정의된 파일 내에서만 접근 가능하도록 제한합니다.

즉, 다른 파일에서 extern처럼 접근할 수 없습니다.

헤더 파일에서 정의할 때 주로 사용되며, 중복 정의 오류를 방지합니다.


### static inline -> 헤더 파일에서 오버헤드를 줄이며 안전하게 함수 정의(중복 정의 방지)

실행시간 비교 코드

```
#include <stdio.h>
#include <time.h>

static inline int add_inline(int a, int b) {
    return a + b;
}

int add_normal(int a, int b) {
    return a + b;
}

int main() {
    const int N = 100000000; // 반복 횟수
    int sum = 0;

    clock_t start = clock();
    for (int i = 0; i < N; i++) sum += add_inline(i, i);
    clock_t end = clock();
    printf("inline: %lf sec\n", (double)(end - start) / CLOCKS_PER_SEC);

    sum = 0;
    start = clock();
    for (int i = 0; i < N; i++) sum += add_normal(i, i);
    end = clock();
    printf("normal: %lf sec\n", (double)(end - start) / CLOCKS_PER_SEC);

    return 0;
}
```


-o 최적화 옵션 여부와 관계없이 빨랐다
root@79a1c58b2b44:/mnt/shared# cc -o2 -Wall inline.c  -o inline
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.146539 sec
normal: 0.160634 sec
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.149128 sec
normal: 0.160022 sec
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.145217 sec
normal: 0.159571 sec
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.145636 sec
normal: 0.160233 sec
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.145009 sec
normal: 0.158974 sec
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.145356 sec
normal: 0.160393 sec
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.145124 sec
normal: 0.159659 sec
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.146111 sec
normal: 0.160425 sec
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.146238 sec
normal: 0.159372 sec
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.148551 sec
normal: 0.159179 sec
root@79a1c58b2b44:/mnt/shared# cc  -Wall inline.c  -o inline
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.146213 sec
normal: 0.160025 sec
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.145586 sec
normal: 0.159855 sec
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.145839 sec
normal: 0.160439 sec
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.145642 sec
normal: 0.160326 sec
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.146715 sec
normal: 0.158866 sec
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.145778 sec
normal: 0.159011 sec
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.145374 sec
normal: 0.159141 sec
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.144677 sec
normal: 0.159547 sec
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.149449 sec
normal: 0.159512 sec
root@79a1c58b2b44:/mnt/shared# ./inline 
inline: 0.145572 sec
normal: 0.160329 sec
```