# Hello, Java 21



https://spring.io/blog/2023/09/20/hello-java-21#virtual-threads-and-project-loom



안녕하세요, Spring 팬 여러분!

### 설정하기

먼저 간단히 작업을 시작해 봅시다. 아직 설치하지 않았다면, **SKDMAN**을 설치하세요.

그런 다음 아래 명령어를 실행하세요:

bash

코드 복사

`sdk install java 21-graalce && sdk default java 21-graalce`

이제 여러분의 시스템에 Java 21과 GraalVM이 설치되었습니다. 제 생각에, Java 21은 Java 역사상 가장 중요한 릴리스 중 하나입니다. 이는 Java를 사용하는 개발자들에게 완전히 새로운 가능성을 열어주기 때문입니다. Java 21은 패턴 매칭과 같은 멋진 API 및 기능을 제공하며, 수년에 걸쳐 플랫폼에 천천히 추가된 기능들이 결실을 맺는 릴리스입니다. 그러나 단연코 가장 주목할 만한 기능은 **가상 쓰레드**(Virtual Threads, Project Loom) 지원입니다.

가상 쓰레드와 GraalVM 네이티브 이미지를 통해, 이제 C, Rust, Go와 같은 언어에 필적하는 성능과 확장성을 갖춘 코드를 작성할 수 있으며, 여전히 JVM의 견고하고 익숙한 생태계를 유지할 수 있습니다.

**JVM 개발자가 되기에 이보다 좋은 시기는 없을 것입니다.**

저는 Java 21과 GraalVM의 새로운 기능과 가능성을 탐구한 영상을 방금 게시했습니다. 이 블로그에서는 텍스트로 추가적인 데이터를 포함하여 같은 내용을 다루고자 합니다.

---

### 왜 GraalVM인가? 그냥 기존 Java는 왜 안 되는가?

먼저, 위의 설치 과정을 통해 알 수 있듯이, 저는 GraalVM을 먼저 설치하는 것을 권장합니다. GraalVM은 OpenJDK 기반으로 OpenJDK의 모든 기능을 제공하며, GraalVM 네이티브 이미지를 생성할 수도 있습니다.

그렇다면 왜 GraalVM 네이티브 이미지를 사용해야 할까요? 이유는 간단합니다. **빠르고 리소스를 매우 효율적으로 사용하기 때문입니다.** 전통적으로, 이에 대해 "그렇지만 JIT(Java Just-In-Time Compiler)는 여전히 기존 Java에서 더 빠르지 않나?"라는 반론이 있었습니다. 이에 대해 저는 다음과 같이 대답하곤 했습니다.

*"네, 하지만 잃은 처리량을 보완하기 위해 새로운 인스턴스를 훨씬 적은 리소스 사용량으로 더 쉽게 확장할 수 있고, 여전히 자원 소비 측면에서는 유리합니다!"*

그리고 그 주장은 사실이었습니다.

하지만 이제 이런 세부적인 논의는 필요하지 않게 되었습니다. GraalVM 릴리스 블로그에 따르면, Oracle의 GraalVM 네이티브 이미지는 **프로파일 기반 최적화**(Profile-Guided Optimization, PGO)를 통해, 이전에는 일부 벤치마크에서만 JIT을 앞섰던 성능이 이제는 대부분의 경우 JIT을 앞서고 있습니다. Oracle GraalVM은 오픈 소스 GraalVM 배포판과 반드시 동일하지는 않지만, 중요한 점은 **최고 수준의 성능이 이제 JRE JIT을 능가한다는 것**입니다.

10MinuteMail의 이 훌륭한 글에서는 GraalVM과 Spring Boot 3을 활용하여 애플리케이션의 시작 시간을 약 30초에서 **3밀리초**로 단축하고, 메모리 사용량을 **6.6GB에서 1GB**로 줄이면서도 동일한 처리량과 CPU 활용도를 유지한 방법을 보여줍니다. 정말 놀라운 성과입니다.

# Java 17

**Java 21**의 많은 기능은 **Java 17**에서 처음 도입된 기능들을 기반으로 하고 있습니다. (때로는 그 이전 버전의 기능까지 포함합니다!) 이들 기능이 Java 21에서 어떻게 발전했는지 살펴보기 전에, Java 17에서 도입된 주요 기능들을 복습해 봅시다.

---

### **멀티라인 문자열 (Multiline Strings)**

Java가 **멀티라인 문자열**을 지원한다는 것을 알고 계셨나요? 이 기능은 JSON, JDBC, JPA QL 등을 훨씬 더 간편하게 사용할 수 있도록 해 주는, 개인적으로 매우 좋아하는 기능 중 하나입니다.

```java
package bootiful.java21;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

class MultilineStringTest {

    @Test
    void multiline() throws Exception {

        var shakespeare = """

                To be, or not to be, that is the question:
                Whether 'tis nobler in the mind to suffer
                The slings and arrows of outrageous fortune,
                Or to take arms against a sea of troubles
                And by opposing end them. To die—to sleep,
                No more; and by a sleep to say we end
                The heart-ache and the thousand natural shocks
                That flesh is heir to: 'tis a consummation
                Devoutly to be wish'd. To die, to sleep;
                To sleep, perchance to dream—ay, there's the rub:
                For in that sleep of death what dreams may come,
                """;
        Assertions.assertNotEquals(shakespeare.charAt(0), 'T');

        shakespeare = shakespeare.stripLeading();
        Assertions.assertEquals(shakespeare.charAt(0), 'T');
    }
}
```

위 예제에서처럼 **트리플 쿼트(`"""`)**로 멀티라인 문자열을 시작하고 종료합니다. 그리고 **`stripLeading()`** 메서드로 문자열의 선행 공백을 제거할 수도 있습니다.

---

### **레코드 (Records)**

Java에서 가장 멋진 기능 중 하나인 **레코드**는 정말 환상적입니다! 클래스의 정체성이 클래스의 필드에 의해 정의되는 경우가 있죠? 기본적인 엔티티, 이벤트, DTO 등에서 흔히 나타납니다.

과거에 **Lombok의 @Data**를 사용했다면, 이제는 레코드를 사용하는 것이 더 간편합니다. Kotlin의 **data class**나 Scala의 **case class**와 유사하며, 많은 개발자들이 익숙할 것입니다. 이제 Java에서도 사용할 수 있게 되어 정말 반갑습니다.

```java
package bootiful.java21;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

class RecordTest {

    record JdkReleasedEvent(String name) { }

    @Test
    void records() throws Exception {
        var event = new JdkReleasedEvent("Java21");
        Assertions.assertEquals(event.name(), "Java21");
        System.out.println(event);
    }
}
```

레코드는 간결한 문법으로 다음과 같은 기능을 제공합니다:

- 생성자
- 내부 필드 저장
- 게터 (예: `event.name()`)
- 올바른 **`equals`** 메서드
- 유용한 **`toString`** 구현

---

### **향상된 Switch (Enhanced Switch)**

기존의 **switch** 문은 사용하기 불편하여 잘 쓰지 않았습니다. 보통은 방문자 패턴(visitor pattern) 같은 다른 방식들이 더 유용했기 때문입니다. 이제 **새로운 switch**는 **표현식(expression)**으로 동작하므로, 결과를 변수에 할당하거나 반환할 수 있습니다.

```java
package bootiful.java21;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import java.time.DayOfWeek;

class EnhancedSwitchTest {

    // ① 기존 switch 문
    int calculateTimeOffClassic(DayOfWeek dayOfWeek) {
        var timeoff = 0;
        switch (dayOfWeek) {
            case MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY:
                timeoff = 16;
                break;
            case SATURDAY, SUNDAY:
                timeoff = 24;
                break;
        }
        return timeoff;
    }

    // ② 새로운 switch 표현식
    int calculateTimeOff(DayOfWeek dayOfWeek) {
        return switch (dayOfWeek) {
            case MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY -> 16;
            case SATURDAY, SUNDAY -> 24;
        };
    }

    @Test
    void timeoff() {
        Assertions.assertEquals(calculateTimeOffClassic(DayOfWeek.SATURDAY), calculateTimeOff(DayOfWeek.SATURDAY));
        Assertions.assertEquals(calculateTimeOff(DayOfWeek.FRIDAY), 16);
    }
}
```

이 예제에서:

1. **기존 switch 문**은 장황하고 복잡합니다.
2. **새로운 switch 표현식**은 간결하고 가독성이 훨씬 좋습니다

**향상된 instanceof 검사**  
새로운 `instanceof` 테스트를 사용하면, 다음과 같이 번거로웠던 기존의 검사 및 캐스팅 과정을 피할 수 있습니다:

```
var animal = (Object) new Dog();
if (animal instanceof Dog) {
    var fido = (Dog) animal;
    fido.bark();
}
```

위 코드는 다음과 같이 간단히 바꿀 수 있습니다:

```
var animal = (Object) new Dog();
if (animal instanceof Dog fido) {
    fido.bark();
}
```

var animal = (Object) new Dog(); if (animal instanceof Dog fido) {     fido.bark(); }

스마트 `instanceof`는 테스트 범위 내에서 사용할 **다운캐스트된 변수**를 자동으로 할당합니다. 같은 블록 내에서 `Dog` 클래스를 두 번 지정할 필요가 없습니다.

스마트 `instanceof` 연산자의 사용은 **Java 플랫폼에서 패턴 매칭을 처음으로 경험**하게 해줍니다. 패턴 매칭의 아이디어는 간단합니다: **타입을 매칭하고, 해당 타입에서 데이터를 추출**하는 것입니다.

**Sealed Types (봉인된 타입)**

엄밀히 말하면, **봉인된 타입**은 Java 17에도 도입된 기능이지만, 당시에는 그다지 활용도가 높지 않았습니다. 기본 개념은 간단합니다. 과거에는 특정 타입의 확장을 제한하려면 `public`, `private` 등 **가시성 제어자**를 사용해야 했습니다.

하지만 **`sealed` 키워드**를 사용하면 **특정 클래스가 어떤 클래스만 서브클래싱(subclassing)할 수 있는지 명시적으로 허용**할 수 있습니다. 이는 큰 도약으로, 컴파일러가 특정 타입을 확장할 수 있는 타입을 알 수 있게 되어 최적화를 수행하거나 컴파일 시점에 다음과 같은 점을 확인할 수 있습니다:

- 향상된 `switch` 표현식에서 **모든 가능한 경우가 처리되었는지 확인**.

```
package bootiful.java21;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

class SealedTypesTest {

    // ① 봉인된 인터페이스 정의
    sealed interface Animal permits Bird, Cat, Dog {
    }

    // ② 허용된 서브클래스 정의
    final class Cat implements Animal {
        String meow() {
            return "meow";
        }
    }

    final class Dog implements Animal {
        String bark() {
            return "woof";
        }
    }

    final class Bird implements Animal {
        String chirp() {
            return "chirp";
        }
    }

    @Test
    void doLittleTest() {
        Assertions.assertEquals(communicate(new Dog()), "woof");
        Assertions.assertEquals(communicate(new Cat()), "meow");
    }

    // ③ 클래식 방식
    String classicCommunicate(Animal animal) {
        var message = (String) null;
        if (animal instanceof Dog dog) {
            message = dog.bark();
        }
        if (animal instanceof Cat cat) {
            message = cat.meow();
        }
        if (animal instanceof Bird bird) {
            message = bird.chirp();
        }
        return message;
    }

    // ④ 향상된 switch 표현식과 패턴 매칭
    String communicate(Animal animal) {
        return switch (animal) {
            case Cat cat -> cat.meow();
            case Dog dog -> dog.bark();
            case Bird bird -> bird.chirp();
        };
    }
}
```

### 주요 특징:

1. **봉인된 인터페이스**: `Animal` 인터페이스는 `Cat`, `Dog`, `Bird` 클래스만 서브클래싱할 수 있도록 제한되었습니다.
2. **허용된 서브클래스**: 이 인터페이스를 구현하는 클래스는 `sealed`로 선언하여 추가 서브클래스를 제한하거나, `final`로 선언해 더 이상 확장할 수 없도록 할 수 있습니다.
3. **패턴 매칭의 도움**: `instanceof`를 활용하여 각 타입에 따라 처리할 수 있지만, 이 방식은 컴파일러의 도움을 받을 수 없습니다.
4. **향상된 `switch` 표현식**: 새로운 `switch` 표현식을 활용하면 모든 가능한 경우를 처리해야 하며, 누락된 경우 컴파일러가 경고를 제공합니다.

**장점**: 클래식 방식은 장황하고 실수하기 쉽지만, 향상된 `switch`와 봉인된 타입은 코드의 안전성과 가독성을 높여줍니다. **감사합니다, 컴파일러!**

**Java 17 이후: Java 21로의 도약**

모든 개선 사항을 종합해 보면 이제 Java 21 시대에 본격적으로 접어들었음을 느낄 수 있습니다. 여기부터는 Java 17 이후 추가된 기능들에 대해 살펴보겠습니다.

---

### `record`와 `switch`, 그리고 `if`를 활용한 차세대 패턴 매칭

향상된 `switch` 표현식과 패턴 매칭 기능은 놀라울 정도로 혁신적입니다. 몇 년 전 Akka를 사용하던 시절에 이러한 새로운 문법을 갖춘 Java를 사용했다면 얼마나 훨씬 나았을까 하는 생각이 들 정도입니다.

패턴 매칭은 특히 **`record`**와 함께 사용될 때 훨씬 매끄럽게 동작합니다. `record`는 컴포넌트(멤버 필드)의 요약 형태로 설계되었기 때문에, 컴파일러가 해당 컴포넌트를 알고 있으며 이를 새로운 변수로 쉽게 추출할 수 있습니다.

또한 이 패턴 매칭 문법은 단순히 `switch` 표현식에서뿐 아니라 **`if` 조건문**에서도 사용할 수 있습니다.

```java
package bootiful.java21;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import java.time.Instant;

class RecordsTest {

    record User(String name, long accountNumber) {
    }

    record UserDeletedEvent(User user) {
    }

    record UserCreatedEvent(String name) {
    }

    record ShutdownEvent(Instant instant) {
    }

    @Test
    void respondToEvents() throws Exception {
        Assertions.assertEquals(
                respond(new UserCreatedEvent("jlong")), "the new user with name jlong has been created"
        );
        Assertions.assertEquals(
                respond(new UserDeletedEvent(new User("jlong", 1))),
                "the user jlong has been deleted"
        );
    }

    String respond(Object o) {
        // ①
        if (o instanceof ShutdownEvent(Instant instant)) {
            System.out.println(
                "going to to shutdown the system at " + instant.toEpochMilli());
        }
        return switch (o) {
            // ②
            case UserDeletedEvent(var user) -> "the user " + user.name() + " has been deleted";
            // ③
            case UserCreatedEvent(var name) -> "the new user with name " + name + " has been created";
            default -> null;
        };
    }

}
```

특정 이벤트를 처리하는 경우에 대한 특별한 사례로, 문자열을 생성하지 않고 시스템을 종료해야 한다면, **패턴 매칭**의 새로운 기능을 활용하여 `if` 문에서 이를 처리할 수 있습니다.

- 이 경우, 단순히 **타입**만 매칭하는 것이 아니라 `UserDeletedEvent`의 `User user`를 추출하고 있습니다.
- 또한, `UserCreatedEvent`의 **`String name`**도 매칭과 동시에 추출하고 있습니다.

이러한 기능들은 이전 버전의 Java에서도 점진적으로 도입되기 시작했으나, **Java 21**에 이르러 **데이터 중심 프로그래밍(data-oriented programming)**으로 정점에 도달했다고 볼 수 있습니다. 이는 기존 객체 지향 프로그래밍을 대체하려는 것이 아니라, 이를 보완하는 접근 방식입니다.

`패턴 매칭`, `향상된 switch`, 그리고 `instanceof` 연산자와 같은 기능들은 코드에 새로운 다형성을 부여하면서, 퍼블릭 API에서 디스패치 지점을 노출하지 않아도 되게 합니다.

---

### Java 21의 기타 새로운 기능

Java 21에는 이 외에도 여러 유용한 기능들이 포함되어 있습니다.

- **Project Loom** 또는 **가상 스레드(Virtual Threads)**와 같은 대규모 개선 사항도 포함되어 있습니다.
- 특히, 가상 스레드는 단독으로도 사용할 가치가 있을 정도로 뛰어난 기능입니다.

다음 단계에서는 이러한 놀라운 기능들 중 일부를 깊이 탐구해 보겠습니다.

### 개선된 수학 연산 기능

인공지능(AI) 및 알고리즘에서 효율적인 수학 연산은 그 어느 때보다 중요해지고 있습니다. 새로운 Java JDK에는 이러한 요구를 충족시키기 위해 몇 가지 유용한 개선 사항이 도입되었습니다.

- **BigInteger의 병렬 곱셈(parallel multiplication)** 지원:  
  큰 비트 크기를 가진 `BigInteger`에 대해 병렬 처리를 통해 곱셈 연산의 성능을 향상시킬 수 있습니다.

- **오버플로우 예외 처리 기능이 포함된 나눗셈(divisions with overflow detection):**  
  새로운 나눗셈 오버로드는 나눗셈 중 오버플로우가 발생할 경우 예외를 던지며, 이는 단순히 **0으로 나누기** 예외만 처리하는 것에서 더 발전된 기능입니다.

이러한 개선 사항은 대규모 데이터 연산이나 고성능이 요구되는 환경에서 특히 유용합니다.

```java
package bootiful.java21;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import java.math.BigInteger;

class MathematicsTest {

    @Test
    void divisions() throws Exception {
        //<1>
        var five = Math.divideExact( 10, 2) ;
        Assertions.assertEquals( five , 5);
    }

    @Test
    void multiplication() throws Exception {
        var start = BigInteger.valueOf(10);
        // ②
        var result = start.parallelMultiply(BigInteger.TWO);
        Assertions.assertEquals(BigInteger.valueOf(10 * 2), result);
    }
}
```

첫 번째 나눗셈 연산은 여러 오버로드 중 하나로, 나눗셈을 더 안전하고 예측 가능하게 만듭니다.

또한, **BigInteger 인스턴스의 병렬 곱셈**에 대한 새로운 지원이 추가되었습니다.  
다만, 이 기능은 BigInteger가 **수천 비트 이상**의 크기를 갖는 경우에만 유용합니다.

---

### `Future#state`

비동기 프로그래밍을 사용 중이라면(네, Project Loom이 있어도 여전히 유용합니다), 이제 익숙한 `Future<T>` 클래스가 **상태 인스턴스**를 제공합니다. 이를 통해 진행 중인 비동기 작업의 상태를 스위치 구문에서 확인할 수 있습니다.

이 새로운 상태 확인 기능은 비동기 작업의 상태를 더욱 직관적으로 관리할 수 있도록 도와줍니다.

```java
package bootiful.java21;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import java.util.concurrent.Executors;

class FutureTest {

    @Test
    void futureTest() throws Exception {
        try (var executor = Executors
                .newFixedThreadPool(Runtime.getRuntime().availableProcessors())) {
            var future = executor.submit(() -> "hello, world!");
            Thread.sleep(100);
            // ①
            var result = switch (future.state()) {
                case CANCELLED, FAILED -> throw new IllegalStateException("couldn't finish the work!");
                case SUCCESS -> future.resultNow();
                default -> null;
            };
            Assertions.assertEquals(result, "hello, world!");
        }
    }
}
```

이 코드는 **스레드 상태를 열거**할 수 있는 상태 객체를 반환합니다.  
이는 향상된 switch 기능과 잘 어울립니다.

Autocloseable HTTP client 부터 ... Sequenced Collections  생략

**가상 스레드와 프로젝트 Loom**  
드디어 Loom에 대해 살펴봅니다. Loom에 대해 많이 들어보셨을 겁니다. 기본 아이디어는 **대학 시절에 작성했던 코드를 확장 가능하게 만드는 것**입니다!  
그게 무슨 뜻일까요? 간단한 네트워크 서비스를 작성해 봅시다. 이 서비스는 제공받은 모든 내용을 출력합니다. 우리는 한 `InputStream`에서 데이터를 읽고, 새 버퍼(`ByteArrayOutputStream`)에 모든 내용을 누적합니다. 요청이 완료되면 이 `ByteArrayOutputStream`의 내용을 출력합니다.

문제는 동시에 많은 데이터를 받을 수 있다는 점입니다. 따라서 여러 요청을 동시에 처리하기 위해 **스레드**를 사용해야 합니다.

다음은 코드입니다:

```java
package bootiful.java21;

import java.io.ByteArrayOutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.concurrent.Executors;

class NetworkServiceApplication {

    public static void main(String[] args) throws Exception {
        try (var executor = Executors.newVirtualThreadPerTaskExecutor()) {
            try (var serverSocket = new ServerSocket(9090)) {
                while (true) {
                    var clientSocket = serverSocket.accept();
                    executor.submit(() -> {
                        try {
                            handleRequest(clientSocket);
                        } catch (Exception e) {
                            throw new RuntimeException(e);
                        }
                    });
                }
            }
        }
    }

    static void handleRequest(Socket socket) throws Exception {
        var next = -1;
        try (var baos = new ByteArrayOutputStream()) {
            try (var in = socket.getInputStream()) {
                while ((next = in.read()) != -1) {
                    baos.write(next);
                }
            }
            var inputMessage = baos.toString();
            System.out.println("request: %s".formatted(inputMessage));
        }
    }
}
```

**네트워킹-101과 스레드의 한계**  
이것은 꽤 기본적인 네트워킹 작업입니다. `ServerSocket`을 생성하고 새로운 클라이언트(여기서는 `Socket`의 인스턴스로 표현됨)를 기다립니다. 각 클라이언트가 도착하면, 그것을 스레드 풀에서 스레드 하나에 할당합니다. 각 스레드는 클라이언트 `Socket` 인스턴스의 `InputStream`에서 데이터를 읽습니다. 클라이언트는 연결을 끊을 수 있거나, 지연이 발생할 수 있으며, 큰 데이터 덩어리를 보낼 수 있습니다. 이 모든 것은 문제입니다. 왜냐하면 사용할 수 있는 스레드 수에는 한계가 있기 때문이며, 우리는 그 스레드들에 소중한 시간을 낭비할 수 없습니다.

우리는 스레드를 사용하여 처리할 수 없는 요청이 쌓이지 않도록 합니다. 하지만 여전히 문제가 발생합니다. Java 21 이전에는 스레드가 비쌌습니다! 각 스레드는 약 2MB의 RAM을 차지했습니다. 그래서 우리는 스레드 풀을 사용하여 이를 재사용합니다. 그러나 너무 많은 요청이 있을 경우, 풀에서 사용 가능한 스레드가 없게 될 수 있습니다. 모든 스레드는 어떤 요청이든 완료되기를 기다리며 막혀 있는 상태입니다. 사실, 많은 스레드는 단지 `InputStream`에서 오는 다음 바이트를 기다리고 있지만, 이들은 사용할 수 없습니다.

스레드가 차단된 상태입니다. 아마도 클라이언트로부터 데이터를 기다리고 있을 것입니다. 불행히도, 서버는 그 데이터를 기다리면서 스레드에 주차된 상태로 있을 수밖에 없습니다. 그렇게 되면 아무도 이 스레드를 사용할 수 없게 됩니다.

**Java 21과 가상 스레드**  
하지만 이제는 다릅니다. Java 21에서는 새로운 종류의 스레드인 가상 스레드를 도입했습니다. 이제 우리는 힙을 위해 수백만 개의 스레드를 생성할 수 있습니다. 그것은 쉬운 일입니다. 하지만 근본적으로 중요한 사실은 실제 스레드에서 가상 스레드가 실행되기 때문에, 실제 스레드는 여전히 비쌉니다. 그렇다면 JRE는 실제 작업을 위한 수백만 개의 스레드를 어떻게 제공할 수 있을까요? 이제 Java는 차단이 발생할 때 이를 인식하고, 해당 스레드의 실행을 중지하여 우리가 기다리고 있는 일이 완료될 때까지 스레드를 일시 중지합니다. 그러면 시스템은 조용히 이 스레드를 다른 스레드로 전환하여 작업을 계속할 수 있도록 합니다.

Java 21은 이제 `InputStream`, `OutputStream`과 같은 전통적으로 스레드를 차단하는 작업과 `Thread.sleep` 같은 작업에서 개선을 이루었습니다. 이제 이러한 작업들이 런타임에 올바르게 신호를 보내어 스레드를 회수하고 다른 가상 스레드에 재사용될 수 있도록 하고, 가상 스레드가 '차단' 상태일 때도 작업을 계속 진행할 수 있게 됩니다. 이 예제에서는 Oracle의 Java Developer Advocate인 José Paumard의 작업을 참고했으며, 그는 이 분야에서 훌륭한 일을 하고 있습니다.

```java
package bootiful.java21;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import java.io.ByteArrayOutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.concurrent.ConcurrentSkipListSet;
import java.util.concurrent.Executors;
import java.util.stream.IntStream;

class LoomTest {

    @Test
    void loom() throws Exception {

        var observed = new ConcurrentSkipListSet<String>();

        var threads = IntStream
                .range(0, 100)
                .mapToObj(index -> Thread.ofVirtual() // ①
                        .unstarted(() -> {
                            var first = index == 0;
                            if (first) {
                                observed.add(Thread.currentThread().toString());
                            }
                            try {
                                Thread.sleep(100);
                            } catch (InterruptedException e) {
                                throw new RuntimeException(e);
                            }
                            if (first) {
                                observed.add(Thread.currentThread().toString());
                            }
                            try {
                                Thread.sleep(20);
                            } catch (InterruptedException e) {
                                throw new RuntimeException(e);
                            }
                            if (first) {
                                observed.add(Thread.currentThread().toString());
                            }
                            try {
                                Thread.sleep(20);
                            } catch (InterruptedException e) {
                                throw new RuntimeException(e);
                            }
                            if (first) {
                                observed.add(Thread.currentThread().toString());
                            }
                        }))
                .toList();

        for (var t : threads)
            t.start();

        for (var t : threads)
            t.join();

        System.out.println(observed);

        Assertions.assertTrue(observed.size() > 1);

    }

}
```

Java 21에서는 가상 스레드를 생성하기 위해 새로운 팩토리 메서드를 사용하고 있습니다. 또한, 팩토리 메서드를 생성하는 다른 대체 팩토리 메서드도 존재합니다.

이 예제에서는 많은 스레드를 실행하여 운영 체제의 실제 스레드와 자원을 공유해야 할 정도로 경합을 일으킵니다. 그 후 스레드를 잠들게 합니다. 일반적으로 `sleep`은 스레드를 차단하지만, 가상 스레드에서는 차단되지 않습니다.

우리는 각 스레드(첫 번째로 실행된 스레드)를 샘플링하여 각 `sleep` 전후에 가상 스레드가 실행되는 실제 스레드의 이름을 확인합니다. 그 결과, 스레드 이름이 바뀐 것을 알 수 있습니다! 런타임은 가상 스레드를 다른 실제 스레드로 이동시켰으며, 우리는 코드에 아무런 변경을 하지 않았습니다! 이것이 바로 Project Loom의 마법입니다. 사실상(단어의 재치에 따라) 코드 변경 없이, 반응형 프로그래밍에서나 얻을 수 있는 뛰어난 확장성과 스레드 재사용을 얻을 수 있습니다.

**네트워크 서비스**에서는 한 가지 변경이 필요합니다. 하지만 기본적인 변경입니다. 스레드 풀을 다음과 같이 교체하는 것입니다:

`try (var executor = Executors.newVirtualThreadPerTaskExecutor()) {...}`

그 외의 모든 부분은 동일하고, 이제 우리는 비할 데 없는 확장성을 얻게 됩니다! Spring Boot 애플리케이션은 일반적으로 다양한 작업을 처리하기 위해 많은 `Executor` 인스턴스를 사용합니다. 예를 들어, 통합, 메시징, 웹 서비스 등에서 사용됩니다. 만약 Spring Boot 3.2(2023년 11월 출시 예정)와 Java 21을 사용하고 있다면, 이 새로운 속성을 사용하여 Spring Boot가 자동으로 가상 스레드 풀을 설정해줍니다! 정말 멋진 기능입니다.

**결론**

Java 21은 매우 중요한 릴리스입니다. 이 버전은 많은 최신 언어들과 동등한 구문을 제공하며, 코드 복잡성을 증가시키지 않으면서도 많은 현대 언어들보다 더 뛰어난 확장성을 자랑합니다. 예를 들어, `async/await`, 반응형 프로그래밍과 같은 복잡한 개념을 사용하지 않아도 됩니다.

만약 네이티브 이미지를 원한다면, GraalVM 프로젝트가 있습니다. GraalVM은 Java 21을 위한 ahead-of-time(AOT) 컴파일러를 제공하며, 이를 사용하여 높은 확장성을 가진 Spring Boot 애플리케이션을 GraalVM 네이티브 이미지로 컴파일할 수 있습니다. 이 네이티브 이미지들은 JVM에서 실행되는 것보다 훨씬 빠르게 시작하고, 훨씬 적은 메모리를 사용합니다. 또한, GraalVM은 Project Loom의 이점을 누리며, 비할 데 없는 확장성을 제공합니다.

`./gradlew nativeCompile`

멋지죠! 이제 우리는 작은 바이너리를 얻었고, 이는 짧은 시간 안에 시작되며, 적은 메모리만 사용하면서도 가장 확장성이 뛰어난 런타임과 동일한 확장성을 자랑합니다. 축하합니다! 이제 당신은 Java 개발자이고, Java 개발자로서 이보다 더 좋은 시기는 없었습니다!
