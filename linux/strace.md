# strace

기본 사용법 
```
# 프로그램 실행하면서 추적
strace ./my_program

# 실행 중인 프로세스에 붙기
strace -p <PID>

# 특정 시스템 콜만 추적
strace -e trace=open,read,write ./my_program

# 파일 관련 시스템 콜만
strace -e trace=file ./my_program
```


유용한 옵션들

c: 시스템 콜 통계 요약 (호출 횟수, 시간, 에러)
t: 각 시스템 콜에 타임스탬프 표시
T: 각 시스템 콜의 소요 시간 표시
f: fork된 자식 프로세스도 추적
o filename: 출력을 파일로 저장
s size: 문자열 출력 최대 길이 설정


