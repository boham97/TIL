# 스프링부트와AWS로혼자구현하는웹서비스

#### ch 02. 스프링 부트에서 테스트코드를 작성하자

TDD  와 유닛 테스트는 다른 이야기

**TDD**

- 테스트가 주도하는 개발, 테스트 코드를 먼저 작성하는 것부터 시작

- 항상 실패하는 테스트 작성

- 테스트 통과하는 프로덕션 코드 작성

- 테스트 통과시 리팩토링

단위 테스트는 TDD의 첫번쨰 기능 단위의 테스트 코드 작성을 의미

장점

- 개발 초기에 문제 발견

- 이후 리팩토링및 라이브러리 업그레이드 시 기존 기느의 작동확인

- 불확실성 감소

- 시스템에 대한 실제 문서를 제공 -> 단위 테스트 자체가 문서로 사용

```java
@RunWith(SpringRunner.class)
@WebMvcTest(controllers = HelloController.class,
        excludeFilters = {
        @ComponentScan.Filter(type = FilterType.ASSIGNABLE_TYPE, classes = SecurityConfig.class)
        }
)
public class HelloControllerTest {

    @Autowired
    private MockMvc mvc;


    @WithMockUser(roles="USER")
    @Test
    public void helloDto가_리턴된다() throws Exception {
        String name = "hello";
        int amount = 1000;

        mvc.perform(
                    get("/hello/dto")
                            .param("name", name)
                            .param("amount", String.valueOf(amount)))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.name", is(name)))
                .andExpect(jsonPath("$.amount", is(amount)));
    }
```

`@RunWith` 

테스트 진행시 JUnit 내장된 실행자 외 다른 실행자 실행

여기서는 스프링 실행자 사용

`@WebMvcTest`

web(Spring MVC)에 집중할수있는 어노테이션

@controller 등의 어노테이션 사용가능

` private MockMvc mvc;` 

웹 api 테스트시 사용

http get, post 등 사용가능

`mvc.perform(get("/hello/dto")`

MockMvc를 통해 /hello/dto 주소롤 http get 메소드 요청

체이닝이 지원되어 여러 검증가능

`.andExpect(status().isOk())` 

http  header의 status 검증

ok = 200

`.andExpect(jsonPath("$.name", is(name)))`

`jsonpath`

json 응답값의 필드별로 검증할수 있는 메소드

#### ch03 스프링부트에서 JPA로 데이터베이스를 다뤄보자

**spring Data JPA**

등장이유

- 구현체의 교체의 용이성
  
  - Hibernate 이외에 다른 구현체로 쉽게 교체하기 위함

- 저장소 교체의 용이성
  
  - 관계형 DB 외에 다른 저장소로 쉽게 교체 가능

```java
@Getter
@NoArgsConstructor
@Entity
public class Posts extends BaseTimeEntity {
```

주요 어노테이션을 클래스 가깝게 두자

새언어로 전환시 쉽게 삭제가능

`@Entity`

기본값으로 클래스의 카멜케이스를 언더스코어 네이밍으로 테이블과 매칭

`@NoArgsConstuctor`

기본 생성자

`@RequiredArgsConstuctor`

선언된 모든final 필드가 포함된 생성자를 생성

Entity에는 절대 setter 메소드를 만들지 말ㄹ자

`@After`

Junit에서 단위 테스트가 끝날때마다 수행하는 메소드 지정

보통은 테스트간 데이터 침범을 막기위해 사용

httpclient로 테스트

`BaseTimeEntity`

```java
@Getter
@MappedSuperclass
@EntityListeners(AuditingEntityListener.class)
public abstract class BaseTimeEntity {

    @CreatedDate
    private LocalDateTime createdDate;

    @LastModifiedDate
    private LocalDateTime modifiedDate;
```

java 8 부터 localdate 생김 이전에는 10월의 숫자값 9 등 문제가 많았다

`@MappedSuperclas`

jpa 엔티티들이 상속시 필드들을 칼럼을 인식하도록

`@EntityListeners(AuditingEntityListener.clas`

auditing 기능 포함

#### ch10 24, 365 중단없느 서비스를 만들자

```yaml
#appspec.yml
version: 0.0
os: linux
files:
  - source:  /
    destination: /home/ec2-user/app/step3/zip/
    overwrite: yes

permissions:
  - object: /
    pattern: "**"
    owner: ec2-user
    group: ec2-user

hooks:
  AfterInstall:
    - location: stop.sh # 엔진엑스와 연결되어 있지 않은 스프링 부트를 종료합니다.
      timeout: 60
      runas: ec2-user
  ApplicationStart:
    - location: start.sh # 엔진엑스와 연결되어 있지 않은 Port로 새 버전의 스프링 부트를 시작합니다.
      timeout: 60
      runas: ec2-user
  ValidateService:
    - location: health.sh # 새 스프링 부트가 정상적으로 실행됐는지 확인 합니다.
      timeout: 60
      runas: ec2-user
```

```shell
#!/usr/bin/env bash
#profile.sh
# bash는 return value가 안되니 *제일 마지막줄에 echo로 해서 결과 출력*후, 클라이언트에서 값을 사용한다

# 쉬고 있는 profile 찾기: real1이 사용중이면 real2가 쉬고 있고, 반대면 real1이 쉬고 있음
function find_idle_profile()
{
    RESPONSE_CODE=$(curl -s -o /dev/null -w "%{http_code}" http://localhost/profile)

    if [ ${RESPONSE_CODE} -ge 400 ] # 400 보다 크면 (즉, 40x/50x 에러 모두 포함)
    then
        CURRENT_PROFILE=real2   #기본값
    else
        CURRENT_PROFILE=$(curl -s http://localhost/profile)
    fi

    if [ ${CURRENT_PROFILE} == real1 ]
    then
      IDLE_PROFILE=real2
    else
      IDLE_PROFILE=real1
    fi

    echo "${IDLE_PROFILE}"
}

# 쉬고 있는 profile의 port 찾기
function find_idle_port()
{
    IDLE_PROFILE=$(find_idle_profile)

    if [ ${IDLE_PROFILE} == real1 ]
    then
      echo "8081"
    else
      echo "8082"
    fi
}
```

```shell
#stop.sh

#!/usr/bin/env bash

ABSPATH=$(readlink -f $0)  
#stop.sh 의 경로 찾기
ABSDIR=$(dirname $ABSPATH)
#마지막'/' 이전의 이름 출력 
source ${ABSDIR}/profile.sh
# profile.sh 의 여러 함수 사용하겠다 
IDLE_PORT=$(find_idle_port)
#
echo "> $IDLE_PORT 에서 구동중인 애플리케이션 pid 확인"
IDLE_PID=$(lsof -ti tcp:${IDLE_PORT})

if [ -z ${IDLE_PID} ]  # -z: 문자열이 비었다면?
then
  echo "> 현재 구동중인 애플리케이션이 없으므로 종료하지 않습니다."
else
  echo "> kill -15 $IDLE_PID"
  kill -15 ${IDLE_PID}
  sleep 5
fi
```

  `lsof -ti`

list open files 열려있는 파일 목록

-t: pid만 출력

-i: 네트워크 연결을 검색

`kill -15` :  정상적인 종료 요청

```shell
#!/usr/bin/env bash
#start.sh

ABSPATH=$(readlink -f $0)
ABSDIR=$(dirname $ABSPATH)
source ${ABSDIR}/profile.sh

REPOSITORY=/home/ec2-user/app/step3
PROJECT_NAME=freelec-springboot2-webservice

echo "> Build 파일 복사"
echo "> cp $REPOSITORY/zip/*.jar $REPOSITORY/"

cp $REPOSITORY/zip/*.jar $REPOSITORY/

echo "> 새 어플리케이션 배포"
JAR_NAME=$(ls -tr $REPOSITORY/*.jar | tail -n 1)

echo "> JAR Name: $JAR_NAME"

echo "> $JAR_NAME 에 실행권한 추가"

chmod +x $JAR_NAME

echo "> $JAR_NAME 실행"

IDLE_PROFILE=$(find_idle_profile)

echo "> $JAR_NAME 를 profile=$IDLE_PROFILE 로 실행합니다."
nohup java -jar \
    -Dspring.config.location=classpath:/application.properties,classpath:/application-$IDLE_PROFILE.properties,/home/ec2-user/app/application-oauth.properties,/home/ec2-user/app/application-real-db.properties \
    -Dspring.profiles.active=$IDLE_PROFILE \
    $JAR_NAME > $REPOSITORY/nohup.out 2>&1 &

#대충 백그라운드 실행한다는 뜻
```

```shell
#!/usr/bin/env bash
#health.sh

ABSPATH=$(readlink -f $0)
ABSDIR=$(dirname $ABSPATH)
source ${ABSDIR}/profile.sh
source ${ABSDIR}/switch.sh

IDLE_PORT=$(find_idle_port)

echo "> Health Check Start!"
echo "> IDLE_PORT: $IDLE_PORT"
echo "> curl -s http://localhost:$IDLE_PORT/profile "
sleep 10

for RETRY_COUNT in {1..10}
do
  RESPONSE=$(curl -s http://localhost:${IDLE_PORT}/profile)
  UP_COUNT=$(echo ${RESPONSE} | grep 'real' | wc -l)

  if [ ${UP_COUNT} -ge 1 ]
  then # $up_count >= 1 ("real" 문자열이 있는지 검증)
      echo "> Health check 성공"
      switch_proxy
      break
  else
      echo "> Health check의 응답을 알 수 없거나 혹은 실행 상태가 아닙니다."
      echo "> Health check: ${RESPONSE}"
  fi

  if [ ${RETRY_COUNT} -eq 10 ]
  then
    echo "> Health check 실패. "
    echo "> 엔진엑스에 연결하지 않고 배포를 종료합니다."
    exit 1
  fi

  echo "> Health check 연결 실패. 재시도..."
  sleep 10
done
```

```shell
#!/usr/bin/env bash
#switch.sh

ABSPATH=$(readlink -f $0)
ABSDIR=$(dirname $ABSPATH)
source ${ABSDIR}/profile.sh

function switch_proxy() {
    IDLE_PORT=$(find_idle_port)

    echo "> 전환할 Port: $IDLE_PORT"
    echo "> Port 전환"
    echo "set \$service_url http://127.0.0.1:${IDLE_PORT};" | sudo tee /etc/nginx/conf.d/service-url.inc

    echo "> 엔진엑스 Reload"
    sudo service nginx reload
}
```
