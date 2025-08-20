SoftIRQ

리눅스 커널에서 하드웨어 인터럽트 처리 이후에 실행되는 소프트웨어 인터럽트 메커니즘


하드 인터럽트: 짧고 빨게 처리해야하는 부분
소프틔: 시간이 조금더 걸리거나 우선순위 낮음

예시
NET_RX_SOFTIRQ : 네트워크 패킷 수신 처리
NET_TX_SOFTIRQ : 네트워크 패킷 송신 처리
TASKLET_SOFTIRQ : tasklet 실행
SCHED_SOFTIRQ : 스케줄링 관련
TIMER_SOFTIRQ : 타이머 이벤트 처리

softir이 높다면 
`watch -n 1 cat /proc/softirqs`
빠르게 증가하는 항목 확인하기