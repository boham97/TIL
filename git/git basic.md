# TIL->github
## repository
- local: 사용중인 컴풔트
- git init: 폴더를 로컬저장소로 생성
- git: 디렉토리에 버전관리에 필요한 모든것이 들어있음
- touch: 파일 생성
- mkdir 폴더생성
- rm -rf 파일 삭제(영구)
- ls 목록
- cd 디렉토리 변경
- start 폴더 파일 오픈

##  markdown cheet sheet 확인!
##  README.md는 대문자!

#  git 기본기
## git 영역
- working directory
  - 작업하고있는 디렉토리(git init)
  - 추적되지 않는 파일 있음
  - git add:  staging area로 파일 이동 
  - git add . : 파일내 모든 파일  staging area로
- staging
  - 특정버전으로 관리 하고 싶은 파일이 있는곳(중간 확인공간)
  - 버전 관리 대상 선포
  - git commit: repository에 저장됨
- repository
  - 버전관리(commit)이 저장되는곳

## remote
git remote add origin 깃헙주소: 깃헙에 연결
git push origin master : 깃헙에 올린다 
git remote -v : 등록된 remote의 정보 확인
git remote 레포 별명 branch: 

git status: git의 상태를 확인

### git commit
- command mod, edit mod
- :wq  저장후 나감
이미 등록된파일을 수정하게 되면 modified 로 뜨게 됩니다

- git commit -m "커밋 메세지남겨요"
- git log  커밋 기록
- git diff 바뀐점 확인
- git log --oneline log 한줄로 확인
- git clone 주소 :받아오기(로컬로 복제, .git도 복제=git 설정있어 remote주소도 복제)
- git pull origin master : remote repository와 동일한 버전을 다운, remote 정보 필요, .git 필요
- 처음 git commit시 깃허브 메일 이름 입력하기