### git 작업 되돌리기

- working  Directory
  
  - 수정한 파일 되돌리기
  
  - git resrore
    
    - 작업한 코드는 복원 불가
    
    - `git restore {파일 이름}`
    
    - `git 2.23.0: git checkout -- {파일 이름}`
    
    - 임시보관` git stash`도 있어요!

- staging area (git add를 잘못한경우)
  
  - `git rm --cached {파일 이름}`
    
    - commit을 한번도 한적없으면
    
    - 이미 git으로 관리되는 파일을 더이상 관리하고 싶지 않을 때
    
    - ※`--cached` 없으면 파일도 삭제하니 주의 ※
  
  - `git restore --staged {파일 명}`
    
    - commit 한적이 있으면
    
    - 최근  commit으로 복원

- git repository 작업 되돌리기(commit 수정)
  
  - `git commit --amend`
    
    1. staging area에 새로올라온내용이없으면 직전 커밋 메시지만 수정
    
    2. staging area에 새로올라온내용이있으면 직전 커밋 덮어쓰기
    
    ※이전 commit 을 완전히 고쳐서 새로운 commit으보 변경※
    
    ※ 히스토리 안남음!※

`vim`

입력모드 i

명령모드  esc

저장종료 :wq

강제 종료  :q!

- git reset
  
  - 프로젝트를 특정 버전으로 되돌리고 싶을때
  
  - 이후 버전은 모두 삭제
  
  - `git reset --[option] {커밋id}`
  
  - 옵션:
    
    - `soft` 되돌아간후 커밋이후 파일들은 staging area에 돌려놓음
    
    - `mixed`커밋 이후 파일들을 working directory로 돌려놓음 (기본옵션)
    
    - `hard` 이후 파일들 working directory에서 삭제
  
  - 커밋id: 헤쉬값7자리 이상

- git revert
  
  - 해당 커밋을 취소
  
  - 취소한것을 기록으로 남김
  
  - `git revert {commit ID}`

- git branch
  
  - 작업 공간을 나누어 독립적으로 작업 할 수 있도록 도와주는 git 도구
  
  - 독입공간이므로 원본(master)에 대해 안전함 (test용)
  
  - 체계적 개발가능
  
  - 만드는 속도가 빠르고 적은 용량소비
  
  - `git branch` 브랜치 목록확인
  
  - `git branch -r` 원격저장소 브랜치 목록 확인
  
  - `git branch (name)`  새로운 브랜치 생성
  
  - `git branch (name) {커밋 id}` 특정 커밋에서 기점으로 브랜치 생성
  
  - `git branch -d {name}` merge(병합) 된 브렌치만 삭제 
  
  - `git branch-D {name}` 강제 삭제

- git switch
  
  - `git switch {name}` 다른 브랜치로 이동
  
  - `git switch -c {name}` 생성및 이동
  
  - `git switch -c {name} {commit ID}` 특정커밋 기점으로 브랜치 생성 및 이동
  
  - cat. git/HEAD로  head 브랜치 알수있음
  
  - `(master) ``$git branch hotfix`

- git merge
  
  - `git merge (합칠 브랜치)` <= 사라질 브랜치
  
  - main 브랜치로 와서 실행
  
  - 종류
    
    - fast-forward 브런치가 가리키는 커밋을 앞으로 이동
    
    - 3-way merge 각브랜치의 커밋 두개와 공통 조상하나 를 사용하여  병합
      
      - 같은 부분을 수정하면 merge conflict  발생
      
      - 보통 같은 파일같은 부분을 수정했을때 발생

- work flow
  
  1. 클론받기
  
  2. 브랜치 생성후 기능 구현
  
  3. 본인 브랜치 push
  
  4. pull request를 통해 master에 반영해달라는 요청을 보냄
  
  5. 브랜치 마스터로 변경후 변경된 내용받고 브랜치 삭제

- 오픈 소스 프로젝트인경우(기여하고싶은경우)
1. 내 원격 저장소에 복제 (fork)

2. clone받아서 해당 브런치를 원격저장소에 push

3. 이후 pull request를 통해 원격 저장소에 반영될 수 있도록 요청함

master 제품으로 출시될수있는 브렌치

develop 다음출시버전

feature 기능을 개발하는 브랜치

release이번출시버전 준비라는 브래닟

hotfix 출시 버전에서 밝생한 버그해결용
