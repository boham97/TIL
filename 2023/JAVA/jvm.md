# JVM

**java virtual Machine**

`os에 종속받지 않고   cpu가 자바를 인식 실행시키는 가상 컴퓨터`

**클래스 로더**  
JVM 내로 클래스 파일(`*.class`)을 로드하고, 링크를 통해 배치하는 작업을 수행하는 모듈이다.  
런 타임시 동적으로 클래스를 로드하고 `jar` 파일 내 저장된 클래스들을 JVM 위에 탑재한다.  
<img src="https://blog.kakaocdn.net/dn/AbVQB/btrsH92x5AR/jFLGOD4KiwPEiBUW81RDx1/img.png" title="" alt="[Java] JVM의 클래스 로더란? - 클래스 로더의 종류" width="182">

###### 부트스트랩 클래스 로더 (Bootstrap Class Loader)

JVM 시작 시 가장 최초로 실행되는 클래스 로더

 부트스트랩 클래스 로더는 자바 클래스를 로드할 수 있는 자바 자체의 클래스 로더와 최소한의 자바 클래스(java.lang.Object, Class, ClassLoader)만을 로드한다.

**Java 8**

`jre/lib/rt.jar` 및 기타 핵심 라이브러리와 같은 JDK의 내부 클래스를 로드한다.

**Java 9 이후**

더 이상 `/re.jar`이 존재하지 않으며, `/lib` 내에 모듈화되어 포함됐다. 이제는 정확하게 ClassLoader 내 최상위 클래스들만 로드한다.

###### 확장 클래스 로더

확장 클래스 로더는 부트스트랩 클래스 로더를 부모로 갖는 클래스 로더로서, 확장 자바 클래스들을 로드한다

##### 시스템 클래스 로더

자바 프로그램 실행 시 지정한 Classpath에 있는 클래스 파일 혹은 jar에 속한 클래스들을 로드한다. 쉽게 말하자면, 우리가 만든 .class 확장자 파일을 로드한다.

자바 컴파일러가  `.java`파일을 `.class`라는 자바 bytecode로 변환

`javac test.java` 명령어로 컴파일 가능

`java 'class파일이름'`으로 실행

바이트 코드는 다시  인터프리터 또는 jit컴파일러에 의해 바이너리 코드로 변환

인터프리터 방식으로 실행하다가 적절한 시점에 바이트 코드 전체를 컴파일하여 기계어로 변경하고, 이후에는 해당 더 이상 인터프리팅 하지 않고 기계어로 직접 실행하는 방식

컴파일된 코드는 캐시에 보관

<img src="https://blog.kakaocdn.net/dn/cEjHLD/btq4YtqCAGY/rrVrI45UWSH2LqslkP8Wg0/img.png" title="" alt="" width="364">

**heap영역**

<img src="https://blog.kakaocdn.net/dn/mxiE4/btq4Y5pwyCR/3nO3XIf20wUUTrzMKvn5yk/img.png" title="" alt="" width="442">

1. new/young generation
   
   생명주기가 짧은 객체를 GC대상으로 하는 영역 -> minor GC
   
   eden 영역이 가득차면 첫번째 가비지 콜렉스 발생
   
   survior 1에 복사하고 나머지 객체 삭제
   
   eden에 참조되는 객체 저장되는 공간

2. **Old 영역**
   
   이곳의 인스턴스들은 추후 가비지 콜렉터에 의해 사라진다.  
   생명 주기가 긴 “오래된 객체”를 GC 대상으로 하는 영역이다.  
   여기서 일어나는 가비지 콜렉트를 <u>Major GC</u> 라고 한다. Minor GC에 비해 속도가 느리다.
   
   New/Young Area에서 일정시간 참조되고 있는, 살아남은 객체들이 저장되는 공간이다.

3. **Permanent Generation**
   
   생성된 객체들의 정보의 주소값이 저장된 공간이다. 클래스 로더에 의해 load되는 Class, Method 등에 대한 Meta 정보가 저장되는 영역
   
   Reflection을 사용하여 동적으로 클래스가 로딩되는 경우에 사용된다.
   
   java8부터 삭제

**reflection과 동적 로딩**

reflection

```java
  try {
            Class myClass = Class.forName("MadLife");
            Method method = myClass.getMethod("sayHello", new Class[]{String.class});
```

아직 로드 안됐을수도 있으니 tryㅡcatch로 감싸자

이미 로딩이 완료된 클래스에서 **또 다른 클래스를 동적으로 로딩**하여 생성자, 멤버 필드 그리고 멤버 메서드 등을 사용할 수 있도록 합니다.

컴파일 시간이 아니라 실행시간에 동적으로 특정 클래스의 정보를 객체화를 통해 분석 추출

 private 멤버도 접근과 조작이 가능하므로 주의

조금 더 유연한 코드를 만들 수 있다.

자바의 리플렉션으로는 클래스의 패키지 정보, 접근 지정자, 수퍼 클래스, 어노테이션(Annotation) 등도 얻을 수 있다
