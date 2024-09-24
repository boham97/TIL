# > /dev/null 2>&1



### /dev/null

표준 출력을 /dev/null로 redirection 하라는 의미

= 표준 출력 버림

### 2>&1

표준에러를 표준출력으로 redirection

0 : 표준입력

1 : 표준출력

2 : 표준에러

표준출력과 표준에러를 분리해서 파일로 저장하고 싶을 때

```shell
$ foo.sh 1>output.log 2>error.log
```

