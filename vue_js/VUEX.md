# VUEX

- 중앙 저장소에서 데이터를 모아서 상태관리

- 계층에 관계없이 중앙 저장소에 접근해서 데이터를 얻거나 변경

### STORE

- 중앙 저장소

- state
  
  - 중앙저장소 데이터
  
  - 기준에 쓰던 data
  
  - `$store.stat`로 접근
  
  - state데이터가 변경되면 자동으로 모두 렌더링됨

- getters:
  
  - computed
  
  -  state를 활용하여 계산된 값을 얻고자 할 때 사용
  
  - state에 영향 X
  
  - 결과는 cache되며 종속된 값이 변경된경우만 재계산
  
  - 첫번째 인자는 state
  
  - 두번째 인자는 getters

- mutations
  
  - state를 변경하는 유일한 방법
  
  - 동기적이어야함( 비동기이면 ㅂ변화시기를 특정할수없음)
  
  - 첫인자를 state를 받으며 commit()메소드로 호출
  
  - 뮤테이션즈임을 강조하기 위해 대문자 언더바로 작명

- actions
  
  - 비동기 작업이 포함됨
  
  - state를 직접 변경X
  
  - 모든 데이터에 접근가능
  
  - state를 변경할 수 있지만 하지않음
  
  - actions는 dispatch()에 의해 호출됨

- 모든 데이터를 모두 vuex에서 관리 할 필요는 없음



### Lifecycle Hooks

- 각 vue 인스턴스 생성과 소멸의 과정중 단계별 초기화 과정을 거침

- 각 단계가 트리거가 되어 특정 로직 실행 가능

- CREATED
  
  - VUE INSTANCE 생성된 후 호출
  
  - DATA, computed 등의 설정이 완료된 상태
  
  - $mount 되지않아 DOM 데이터 사용 X$

- mounted
  
  - DOM  연결후
  
  - html 조작 가능!
  
  - 부모 자식 관계에 따라 순서를 가지고 있지 않음



### 메소드--dispatch()-->action--commit()--->mutations--->state

### LOCAL STORAGE

- window.localStorage
  
  - 브라우저에서 제공하는 저장 공간 중 하나인 Local Storage에 관련된속성
  
  - 브라우저를 종료하고 다시 실행해도 데이터가 보존됨
  
  - 데이터 문자열 행태로 저장됨
  
  - `setItem(key, value)` 키 밸류 형태로 저장(문자열임 JSON)
  
  - `getItem(key)`- key에 저장된 밸류 확인
  
  - `stringify()`로 제이슨 형태로 바꿈
  
  - `JSON.parse`로 오브젝트로 변환



- vuex-presostedstate
  
  - 자동으로 로컬 스토리지에 저장
