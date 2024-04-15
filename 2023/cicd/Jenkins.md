# jenkins

- 소프트웨어 개발시 지속적 통합(continuous integration) 서비스를 제공하는 도구입니다.
- 다수의 개발자들이 하나의 프로그램을 개발할 때 버전 충돌을 방지하기 위해 각자 작업한 내용을 공유 영역에 있는 Git, Svn 등의 저장소에 빈번히 업로드함으로써 지속적 통합이 가능하도록 지원합니다.

## multiproject pipeline

#### 플러그인 설치

- Pipeline 및 Github 활용을 위해 다음 플러그인 설치 필요
  - GitHub
  - GitHub Pipeline for Blue Ocean
  - Pipeline: GitHub Groovy Libraries
- tools 에서 사용하기 위해서 필요
  - Gradle

#### 깃헙 토큰 발행

`process access token(classic)`

repo, admin, repo_hook

`Add Credentials`하기

### new item

`multibranch pipeline`선택

`single repo`

1. 이용가능한 agent가 이 파이프라인 또는 파이프라인의 어느 스테이지를 실행한다.

2. "Bulid" 스테이지를 정의한다.

3. 빌드와 관련된 몇가지 스텝을 수행한다.

4. "Test" 스테이지를 정의한다.

5. 테스트와 관련된 몇 가지 스텝을 수행한다.

6. "Deploy" 스테이지를 정의한다.

7. 배포와 관련된 몇 가지 스텝을 수행한다.
- pipeline option
  
  - buildDiscarder
    
    - 빌드 및 아티팩트의 날짜와 수를 기준으로 `logRotator` 를 설정할 수 있습니다.
    
    - 오래된 빌드 및 아티팩트를 삭제하는데 사용.
  
  - disableConcurrentBuilds
    
    - 파이프라인 동시 실행을 허용하지 않습니다.
  
  - disableResume
    
    - 컨트롤러(Jenkins 마스터)가 다시 시작되면 빌드 재개 기능을 비활성화합니다.
  
  - retry
    
    - 실패 시 전체 파이프라인을 지정된 횟수만큼 재시도합니다
  
  - timeout
    
    - 파이프라인 실행에 대한 시간 초과 기간을 설정합니다.
  
  - parallelsAlwaysFailFast
    
    - 병렬이 포함된 단계에 failtFast true를 추가하여 병렬 단계 중 하나가 실패하면 병렬 단게가 모두 중단되도록 강제할 수 있는 옵션입니다.

- `jenkins` 는 환경변수를 여러가지 방법으로 설정할 수 있도록 제공합니다.

- `step` 구간은 필수로 존재해야 합니다.

- `when` 은 주어진 조건에 따라 `stage` 실행해야 하는지 여부를 결정할 수 있습니다. 특히 `changeset` 은 자주 사용하는 옵션입니다.
  
  - `anyOf, allOf, not, changelog branch `

- `parallel` 을 이용하여 병렬적으로 단계를 실행할 수 있습니다.

```
pipeline {     
    agent any      
    tools {         
        gradle 'gradle-7.4.1'
            }
      environment {
         BUILD_TARGET_HOME = '${BUILD_HOME}'
         PROJECT = 'gradle-multi'
         APP_MEMBER = 'app-member'
         APP_ORDER = 'app-order'
         APP_PRODUCT = 'app-product'
     }
      stages {
         stage('Build') {
             parallel {
                 stage('build-app-member') {
                     when {
                         anyOf {
                             changeset "app-common/**/*"
                             changeset "app-member/**/*"
                         }
                     }
                     steps {
                         echo 'Build Start "${APP_MEMBER}"'
                         sh './gradlew ${APP_MEMBER}:build -x test'
                         echo 'Build End "${APP_MEMBER}"'
                     }
                 }
                 stage('build-app-order') {
                     when {
                         anyOf {
                             changeset "app-common/**/*"
                             changeset "app-order/**/*"
                         }
                     }
                     steps {
                         echo 'Build Start "${APP_ORDER}"'
                         sh './gradlew ${APP_ORDER}:build -x test'
                         echo 'Build End "${APP_ORDER}"'
                     }
                 }
                 stage('build-app-product') {
                     when {
                         anyOf {
                             changeset "app-common/**/*"
                             changeset "app-product/**/*"
                         }
                     }
                     steps {
                         echo 'Build Start "${APP_PRODUCT}"'
                         sh './gradlew ${APP_PRODUCT}:build -x test'
                         echo 'Build End "${APP_PRODUCT}"'
                     }
                 }
             }
         }
         stage('Backup & Copy') {
             parallel {
                 stage('back-copy-app-member') {
                     when {
                         anyOf {
                             changeset "app-common/**/*"
                             changeset "app-member/**/*"
                         }
                     }
                     steps {
                         echo 'Backup & Copy Start "${APP_MEMBER}"'
                         echo 'Backup & Copy End "${APP_MEMBER}"'
                     }
                 }
                 stage('back-copy-app-order') {
                     when {
                         anyOf {
                             changeset "app-common/**/*"
                             changeset "app-order/**/*"
                         }
                     }
                     steps {
                         echo 'Backup & Copy Start "${APP_ORDER}"'
                         echo 'Backup & Copy End "${APP_ORDER}"'
                     }
                 }
                 stage('back-copy-app-product') {
                     when {
                         anyOf {
                             changeset "app-common/**/*"
                             changeset "app-product/**/*"
                         }
                     }
                     steps {
                         echo 'Backup & Copy Start "${APP_PRODUCT}"'
                         echo 'Backup & Copy End "${APP_PRODUCT}"'
                     }
                 }
             }
         }
         stage('Deploy') {
             parallel {
                 stage('deploy-app-member') {
                     when {
                         anyOf {
                             changeset "app-common/**/*"
                             changeset "app-member/**/*"
                         }
                     }
                     steps {
                         echo 'Deploy Start "${APP_MEMBER}"'
                         echo 'Deploy End "${APP_MEMBER}"'
                     }
                 }
                 stage('deploy-app-order') {
                     when {
                         anyOf {
                             changeset "app-common/**/*"
                             changeset "app-order/**/*"
                         }
                     }
                     steps {
                         echo 'Deploy Start "${APP_ORDER}"'
                         echo 'Deploy End "${APP_ORDER}"'
                     }
                 }
                 stage('deploy-app-product') {
                     when {
                         anyOf {
                             changeset "app-common/**/*"
                             changeset "app-product/**/*"
                         }
                     }
                     steps {
                         echo 'Deploy Start "${APP_PRODUCT}"'
                         echo 'Deploy End "${APP_PRODUCT}"'
                     }
                 }
             }
         }
     }
 }
```

출처

[[Jenkins] 1.설치와 초기 설정](https://lovethefeel.tistory.com/94)

[[Jenkins] 4.Multiproject Pipeline 적용해보기](https://lovethefeel.tistory.com/97)
