# GITHUB ACTION CI/CD



### CI/CD란?

Continuous Integration(지속적 통합)/Continuous Delivery(지속적 전달)



###### Github Action의 코어 개념

**Workflow**  
자동화된 전체 프로세스. 하나 이상의 Job으로 구성되고, Event에 의해 예약되거나 트리거될 수 있는 자동화된 절차를 말한다.  
Workflow 파일은 YAML으로 작성되고, Github Repository의 `.github/workflows` 폴더 아래에 저장된다. 

**Event**  
Workflow를 트리거(실행)하는 특정 활동이나 규칙. 예를 들어, 누군가가 커밋을 리포지토리에 푸시하거나 풀 요청이 생성 될 때 GitHub에서 활동이 시작될 수 있다.

**Job**  
Job은 여러 Step으로 구성되고, 단일 가상 환경에서 실행된다. 다른 Job에 의존 관계를 가질 수도 있고, 독립적으로 **병렬**로 실행될 수도 있다.

**Step**  
Job 안에서 순차적으로 실행되는 프로세스 단위. step에서 명령을 내리거나, action을 실행할 수 있다.

**Action**  
job을 구성하기 위한 step들의 조합으로 구성된 독립적인 명령. workflow의 가장 작은 빌드 단위이다.

 workflow에서 action을 사용하기 위해서는 action이 step을 포함해야 한다.

 action을 구성하기 위해서 레포지토리와 상호작용하는 커스텀 코드를 만들 수도 있다.

**Runner**  
Gitbub Action Runner 어플리케이션이 설치된 머신으로, **Workflow가 실행될 인스턴스**



### 1. YAML file 생성

프로젝트 경로에 `.github/workflows/asd.yml` 생성

```yml
name: learn-github-actions 
```

workflow 이름 명시

`on:[push, pull]` 이벤트 발생시 job 실행

```yml
name: example CI
on:
    push:
        branches: ["master"]
    pull_request:
        branches: ["master"]

jobs:
  build:                                    #job 이름 
    strategy:                              #각각 다른 버전에서 작동하는지 검
      matrix:
        node-version: [10.x, 12.x]

    runs-on: ubuntu-latest                #어떤 runner를 사용할건지 
    steps:
      - name: Checkout source code       #step 이름
        uses: actions/checkout@v2        #github에서 제공하는 엑션 사용
        with:
              node-version: ${{matrix.node-version}}      #버전 명시 
      - name: My First Step
        run:                            #명령
          npm install
          npm test
          npm build
```

커밋마다 성공한 테스트, 빌드 시간 확인 가능
