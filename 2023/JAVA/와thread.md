# Thread

각 코어들은 아주 짧은 시간동안 여러 프로세스를 번갈아가며 처리하는 방식을 통해 동시에 동작하는 것처럼 보이게 할 뿐

멀티스레딩이란 하나의 프로세스 안에 여러개의 스레드가 동시에 작업을 수행하는 것을 말한다. 스레드는 하나의 작업단위라고 생각하면 편하다.

### 자바에서 구현

1. Runnable 인터페이스 구현
   
   - run() 이미 구현되어 오버라이딩X
   -  Thread 클래스를 상속하면 Thread 클래스를 상속한 클래스가 다른 클래스를 상속할 수 없기 때문이다

2. Thread 클래스 상속
   
   - `currentThread()`를 호출해야 스레드에대한 참조를 얻어 호출 가능
   
   - `System.out.print(i + ":" + Thread.currentThread().getName() + " ");`

둘다 run() 메소드를 오버라이딩 하는 방식

```java
public class MyThread implements Runnable {
    @Override
    public void run() {
        // 수행 코드
   }
}
public class MyThread extends Thread {
    @Override
    public void run() {
        // 수행 코드
    }
}
```

#### 실행제어<img src="https://t1.daumcdn.net/cfile/tistory/998212335C18D52F29" title="" alt="thread" width="598">

- NEW : 스레드가 생성되고 아직 start()가 호출되지 않은 상태
- RUNNABLE : 실행 중 또는 실행 가능 상태
- BLOCKED : 동기화 블럭에 의해 일시정지된 상태(lock이 풀릴 때까지 기다림)
- WAITING, TIME_WAITING : 실행가능하지 않은 일시정지 상태
- TERMINATED : 스레드 작업이 종료된 상태

스레드로 구현하는 것이 어려운 이유는 바로 동기화와 스케줄링 때문이다.

스케줄링과 관련된 메소드는 sleep(), join(), yield(), interrupt()와 같은 것들이 있다.

start() 이후에 join()을 해주면 main 스레드가 모두 종료될 때까지 기다려주는 일도 해준다.

#### 스레드 동기화 방법

- 임계 영역(critical section) : 공유 자원에 단 하나의 스레드만 접근하도록(하나의 프로세스에 속한 스레드만 가능)
- 뮤텍스(mutex) : 공유 자원에 단 하나의 스레드만 접근하도록(서로 다른 프로세스에 속한 스레드도 가능)
- 이벤트(event) : 특정한 사건 발생을 다른 스레드에게 알림
- 세마포어(semaphore) : 한정된 개수의 자원을 여러 스레드가 사용하려고 할 때 접근 제한
- 대기 가능 타이머(waitable timer) : 특정 시간이 되면 대기 중이던 스레드 깨움

#### synchronized 활용

> synchronized를 활용해 임계영역을 설정할 수 있다.

서로 다른 두 객체가 동기화를 하지 않은 메소드를 같이 오버라이딩해서 이용하면, 두 스레드가 동시에 진행되므로 원하는 출력 값을 얻지 못한다.

이때 오버라이딩되는 부모 클래스의 메소드에 synchronized 키워드로 임계영역을 설정해주면 해결할 수 있다.

`synchronized(this) {...}`

`public synchronized void saveMoney(int save){...`

`public synchronized void minusMoney(int minus){...`

설정한 메서드가 호출되었을 때, 메서드를 실행할 스레드는 메서드가 포함된 객체의 락(Lock)을 얻으며, 다시 락(Lock)을 반납하기 전까지는 다른 스레드는 해당 메서드를 실행하지 못한다.

#### wait()과 notify() 활용

> 스레드가 서로 협력관계일 경우에는 무작정 대기시키는 것으로 올바르게 실행되지 않기 때문에 사용한다.

- wait() : 스레드가 lock을 가지고 있으면, lock 권한을 반납하고 대기하게 만듬
  
  - sleep() 메서드는 현재 스레드를 잠시 멈추게 할 뿐 lock을 release 하진 않는다.

- notify() : 대기 상태인 스레드에게 다시 lock 권한을 부여하고 수행하게 만듬

이 두 메소드는 동기화 된 영역(임계 영역)내에서 사용되어야 한다.

동기화 처리한 메소드들이 반복문에서 활용된다면, 의도한대로 결과가 나오지 않는다. 이때 wait()과 notify()를 try-catch 문에서 적절히 활용해 해결할 수 있다.

```java

```

조건 만족 안할 시 wait(), 만족 시 notify()를 받아 수행한다.
