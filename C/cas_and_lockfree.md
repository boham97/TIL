📘 Lock-Free, CAS, Interlocked, Spring Boot – 개념 정리 README
1. 락(lock)과 CAS 경쟁의 기본 개념

대부분의 언어(JVM, Go, C++, Rust)의 경량 락(mutex) 은 경쟁이 거의 없으면
OS 시스템콜까지 내려가지 않고 유저 모드(spin)에서 빠르게 끝남.

하지만 경쟁이 높아지면:

스핀 횟수 증가

커널 스케줄링(futex) 진입

우선순위 역전·컨텍스트 스위치 비용 발생
→ 급격하게 느려짐.

✔ 경쟁이 심해지면 CAS도 느리다

CAS(Compare-And-Swap) 역시 경쟁이 심하면 스핀 루프가 길어져서 느림.
그러나 **락프리(lock-free)의 목적은 성능이 아니라 “전체 시스템의 진행 보장(progress)”**임.

2. 락프리(lock-free)가 존재하는 진짜 이유

락프리는 성능만을 위한 기술이 아니다.

✔ “어떤 스레드가 멈추거나 실패해도 전체 시스템이 멈추지 않게 하는 것”

데드락 없음

우선순위 역전 없음

특정 스레드가 stuck돼도 다른 스레드는 계속 동작

실시간 시스템(HFT·게임 엔진·드라이버)에 매우 유리

따라서 CAS가 실패해도 계속 시도하는 구조가 가치가 있음.

3. 로그, 커넥션풀처럼 여러 락이 얽힐 때 문제

예:

로그 처리에서 락

커넥션 풀에서 락

기타 subsystem에서 락
→ 락 간 의존 관계가 꼬이면 데드락·지연이 발생함.

✔ 이런 구조에서 락프리가 강력함

로그: MPSC lock-free queue

커넥션 풀: CAS 기반 free-list

job queue: lock-free ring buffer

락프리는 “중간에서 멈추는 애” 때문에 전체가 멈추는 문제를 근본적으로 제거함.

4. Spring Boot에서는 어떻게 되는가?

Spring Boot는 개발자가 이런 락프리 자료구조를 직접 만들 필요가 없다.

✔ 스프링 생태계는 “이미 락프리/락 최소화 설계”

Logback / Log4j2

AsyncAppender 사용 시 Disruptor 기반 lock-free MPSC 큐

HikariCP (Spring 기본 커넥션 풀)

CAS 기반 lock-minimal 구조

대부분 fast-path는 atomic + volatile 만으로 처리

Spring WebFlux / Reactor

기본적으로 lock-free scheduler

Caffeine Cache

lock-free path 포함

Tomcat/Netty request 처리

lock-free 또는 최소화된 락 구조

즉, 스프링은 내부적으로 알아서 락프리 기법을 활용해 고성능·고안정성을 보장함.

→ 스프링에서 직접 락프리 구현할 필요 없음.

5. Visual Studio 2008에서의 원자적 연산

VS2008은 C11 stdatomic.h도 없고
GCC의 __sync_*, __atomic_*도 없음.

✔ 사용 가능한 원자 연산은 오직 Win32 Interlocked API

InterlockedCompareExchange (CAS)

InterlockedExchange (swap)

InterlockedIncrement / Decrement

InterlockedExchangeAdd

pointer 버전도 존재

이 함수들은 CPU의 LOCK XADD, CMPXCHG 등을 직접 호출하며
메모리 배리어까지 보장됨 → 완전한 atomic.

6. VS2008에서도 락프리 로그 구현은 충분히 가능

Interlocked 함수만으로도 아래가 모두 구현된다:

Lock-free MPSC log queue

Lock-free linked queue(Michael-Scott)

Lock-free free-list

Lock-free stack

ABA 방지 tagged pointer도 구현 가능

MPSC 로그 구조:

여러 스레드 → log push (CAS 기반)

하나의 소비자 스레드 → 파일로 flush

완벽히 락프리 구조 구축 가능.

📌 요약

CAS와 락은 경쟁이 적을 때는 빠르지만 경쟁이 심해지면 둘 다 느려진다.

락프리는 “빠르기 위해서”가 아니라 “시스템 진행 보장(Progress)”을 위해 존재한다.

로그/커넥션풀처럼 여러 락이 꼬이는 상황에서 락프리가 특히 유리하다.

Spring Boot는 내부적으로 이미 lock-free 또는 lock-minimal 구조를 채택하고 있어 개발자가 직접 락프리를 만들 필요가 거의 없다.

VS2008에서도 Win32 Interlocked API만으로 완전한 lock-free log system 구현이 가능하다.