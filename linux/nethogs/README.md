# nethogs 설치 및 netload.py 사용 정리

## 1. 환경
- OS: Rocky Linux 10.2
- 네트워크 인터페이스: `enp2s0` (128.10.31.61)
- 작업 폴더: `/home/kang/test/tcpdump`

## 2. nethogs 설치
nethogs는 기본 저장소에 없어서 EPEL 저장소를 추가해야 합니다.

```bash
sudo dnf install -y epel-release                  # EPEL 저장소 추가
sudo dnf config-manager --set-enabled crb         # CRB 저장소 활성화
sudo dnf install -y nethogs                       # 설치
nethogs -V                                        # 설치 확인
```

### 사용법
```bash
sudo nethogs enp2s0          # 특정 인터페이스만
sudo nethogs -d 2 enp2s0     # 2초마다 갱신
```

실행 중 단축키:
- `m`: 표시 단위 변경
- `s`: 보낸 트래픽 순으로 정렬
- `r`: 받은 트래픽 순으로 정렬
- `q`: 종료

## 3. 부하 생성기 `netload.py`
파이썬만 있으면 돌아가고 추가 패키지는 필요 없습니다. nethogs에서는 `python3` 프로세스로 보입니다.

### 모드
| 모드 | 동작 | 트래픽 방향 |
|---|---|---|
| `download` | Cloudflare 속도 측정 서버에서 다운로드 | 받기 |
| `upload` | Cloudflare 속도 측정 서버로 업로드 | 보내기 |
| `both` | 위 두 가지를 동시에 | 보내기 + 받기 |
| `server` / `client` | 같은 네트워크의 PC 두 대끼리 TCP로 주고받기 | `--dir`로 선택 |

### 옵션 (기본값은 안전하게 설정해 둠)
| 옵션 | 설명 | 기본값 |
|---|---|---|
| `-r` | 연결 하나당 최대 속도 (MB/s), 0이면 무제한 | **1** |
| `-d` | 실행 시간 (초), 0이면 Ctrl+C로 끌 때까지 | **60** |
| `-t` | 방향별 동시 연결 수 | 1 |
| `-p` | 서버/클라이언트 모드 포트 | 5201 |
| `--dir` | 클라이언트 모드 방향: send, recv, both | both |

**전송량 = 속도 × 연결 수 × 시간** (방향별로 각각)
- 아무 옵션 없이 실행하면 60초 동안 방향마다 약 60MB를 주고받고 자동으로 꺼집니다.

### 실행 예시
```bash
# 터미널 1
sudo nethogs enp2s0

# 터미널 2
cd /home/kang/test/tcpdump
python3 netload.py both                 # 1MB/s로 60초
python3 netload.py download -r 3 -d 30  # 받기만, 3MB/s로 30초
python3 netload.py upload -r 2 -d 20    # 보내기만, 2MB/s로 20초
```

PC 두 대로 테스트:
```bash
# 서버 PC
sudo firewall-cmd --add-port=5201/tcp
python3 netload.py server

# 클라이언트 PC
python3 netload.py client 128.10.31.61 --dir send
```

## 4. 주의할 점
- **이 PC 자신에게 보내는 트래픽**(127.0.0.1이나 128.10.31.61)은 `lo` 인터페이스로만 흐르기 때문에 `nethogs enp2s0`에 보이지 않습니다.
- nethogs의 숫자가 스크립트 출력보다 조금 큽니다. nethogs는 패킷 헤더까지 포함해서 세기 때문이고 정상입니다.
- `-r 0`, `-d 0`이나 `-t`를 크게 주면 전송량이 확 늘어납니다. 회사망이라면 조심하세요.

## 5. 화면이 한 줄만 갱신되는 원리
```python
print(f"\r...", end="", flush=True)
```
- `\r`: 커서를 줄 맨 앞으로 돌려보냅니다.
- `end=""`: 줄바꿈을 막습니다.
- `flush=True`: 바로 화면에 출력합니다.

이렇게 같은 줄을 계속 덮어씁니다. nethogs나 top처럼 화면 전체를 갱신하는 프로그램은 ncurses를 씁니다.

## 6. 함께 쓰면 좋은 명령
```bash
sudo tcpdump -i enp2s0 -nn host speed.cloudflare.com   # 패킷 직접 보기
sudo ss -tnp | grep python3                            # 프로세스별 연결 확인
sudo iftop -i enp2s0 -n                                # IP별 대역폭
```
