데이터 경쟁(Data Race) 없음

여러 스레드가 동시에 같은 메모리 위치에 하나는 쓰기, 하나는 읽기를 하는 경우 반드시 동기화되어야 합니다.

C 기준: volatile, pthread_mutex, atomic, __sync_* 등의 방법으로 제어.

원자성(Atomicity) 보장

연산 단위가 중간에 끊기지 않고 완전하게 수행되어야 함.

예: a++는 원자 연산이 아님 → __sync_fetch_and_add() 같은 원자 연산으로 대체 필요.

가시성(Visibility) 보장

한 스레드에서 수정한 메모리 값을 다른 스레드가 즉시 볼 수 있어야 함.

C에선 __sync_synchronize(), pthread_barrier, 메모리 배리어 등이 사용됨.

멀티코어 환경에서는 CPU 캐시 때문에 가시성이 무너질 수 있음 → 적절한 memory barrier 필요.

순서 보장(Ordering)

컴파일러나 CPU가 명령을 재배치할 수 있으므로, 중요한 연산 순서는 메모리 배리어 등으로 보장해야 함.

예: CAS 전후에 __sync_synchronize() 또는 __atomic_thread_fence() 필요.

동기화 전략 일관성

락 기반이면 락을 걸고 푸는 위치가 명확하고, 데드락 방지 구조여야 함.

락 없이 구현한다면 모든 접근을 CAS 기반 등으로 통일해야 함.



memory barrier란?	CPU나 컴파일러의 명령 재정렬을 막아주는 장치
왜 필요한가?	스레드 간 메모리 접근 순서 보장을 위해
C에서 어떻게 쓰나?	__sync_synchronize(), __atomic_thread_fence() 등 사용
언제 써야 하나?	CAS 전후, 플래그 기반 동기화 시, 락 없는 통신 시 등


CAS 자체가 atomic + memory barrier 기능을 포함하므로, (`__sync_bool_compare_and_swap`)

별도의 __sync_synchronize()는 필요 없습니다.