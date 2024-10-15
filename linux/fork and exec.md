# fork and exec

fork

새로운 프로세스를 생성한 뒤에 현재 프로세스의 메모리 상태를 새로운 프로세스에 복사

코드는 공유하지만 메모리 pid 공유 X

fork() 리턴값이 0 이면 자식 프로세스



exec 계열

execl, execlp, execle, execv, execvp, execvpe

l : argument가 list형태로 받는다는 뜻
v : argument가 배열형태로 받는다는 뜻
p : 첫번째 파라미터가 PATH경로에 존재한다면 상대경로나 절대경로를 주지 않아도 된다는 뜻
e : 환경변수를 인자로 받을 수 있다는 뜻

execv -> args를 배열로 받음


