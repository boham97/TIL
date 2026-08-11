# 윈도우 OpenSSH 서버 설정 정리 (맥 → 윈도우 도커 접속)

## 환경 정보

| 항목 | 값 |
|---|---|
| 윈도우 계정 | `bosung` (관리자 그룹) |
| 컴퓨터명 | `desktop-uqgaats` |
| 윈도우 IP | `192.168.100.136` |
| 맥 IP | `192.168.100.101` |
| 게이트웨이 | `192.168.100.1` |
| 인증 방식 | 공개키 (ed25519) |

접속 구조:

```
맥북 → ssh(22) → 윈도우 호스트 → docker exec → 컨테이너 내부
```

컨테이너 포트를 따로 열 필요 없음. 호스트에 로그인해서 호스트가 도커를 조작하는 방식.

---

## 1. OpenSSH 서버 설치

설정 → 시스템 → 선택적 기능 → "사용 가능한 기능 보기" → OpenSSH 서버

또는 관리자 PowerShell에서:

```powershell
# 설치 여부 확인
Get-WindowsCapability -Online -Name OpenSSH.Server*

# 설치
Add-WindowsCapability -Online -Name OpenSSH.Server~~~~0.0.1.0
```

## 2. 서비스 시작 + 자동 시작 등록

```powershell
Start-Service sshd
Set-Service -Name sshd -StartupType 'Automatic'

Get-Service sshd    # 상태 확인
```

## 3. 방화벽 설정

> 여기서 한 번 막혔던 부분. `Connection refused`가 아니라 **무응답으로 멈추면** 방화벽 문제.

```powershell
# 현재 네트워크 프로파일 확인 (Public이면 인바운드가 막힘)
Get-NetConnectionProfile

# 개인용 네트워크로 전환
Set-NetConnectionProfile -InterfaceAlias "이더넷 2" -NetworkCategory Private

# 규칙 상태 확인
Get-NetFirewallRule -Name *OpenSSH* | Select-Object Name, Enabled, Profile

# 모든 프로파일에서 활성화
Set-NetFirewallRule -Name "OpenSSH-Server-In-TCP" -Profile Any -Enabled True
```

규칙 자체가 없으면 생성:

```powershell
New-NetFirewallRule -Name 'OpenSSH-Server-In-TCP' -DisplayName 'OpenSSH Server (sshd)' `
  -Enabled True -Direction Inbound -Protocol TCP -Action Allow -LocalPort 22 -Profile Any
```

## 4. 공개키 등록

### 맥에서 키 생성

```bash
ssh-keygen -t ed25519
cat ~/.ssh/id_ed25519.pub
```

### 윈도우에 등록 — 관리자 계정이므로 경로 주의

관리자 그룹 계정은 `~/.ssh/authorized_keys`가 **무시된다.** 반드시 아래 경로 사용.

```powershell
$key = "ssh-ed25519 AAAAC3Nza...복사한_공개키"

Add-Content -Path C:\ProgramData\ssh\administrators_authorized_keys -Value $key -Encoding utf8

# 권한 잠그기 (안 하면 sshd가 키 파일을 무시함)
icacls C:\ProgramData\ssh\administrators_authorized_keys /inheritance:r `
  /grant "Administrators:F" /grant "SYSTEM:F"

Restart-Service sshd
```

### 확인

```powershell
Get-Content C:\ProgramData\ssh\administrators_authorized_keys
icacls C:\ProgramData\ssh\administrators_authorized_keys
```

권한은 `Administrators:(F)`, `SYSTEM:(F)` **딱 둘만** 있어야 함.

## 5. 접속

```bash
ssh bosung@192.168.100.136

# 접속 후
docker ps
docker exec -it 컨테이너명 bash

# 한 줄로 바로 들어가기
ssh -t bosung@192.168.100.136 "docker exec -it 컨테이너명 bash"
```

---

## 보안 강화 (권장)

계정에 비밀번호가 없어서 지금도 원격 비밀번호 로그인은 윈도우 정책상 막혀 있지만, **정책에 의존하지 말고 sshd에서 직접 차단**하는 게 안전하다.

`C:\ProgramData\ssh\sshd_config` 편집 (관리자 권한 메모장):

```
PasswordAuthentication no
PubkeyAuthentication yes
PermitEmptyPasswords no
```

적용:

```powershell
sshd -t                  # 문법 검사, 아무것도 안 나오면 정상
Restart-Service sshd
```

> 주의: 적용 후 **기존 세션을 끊지 말고** 새 터미널에서 접속 테스트할 것. 설정이 잘못되면 못 들어간다.

### 추가 권장 사항

- 공유기에서 22번 포트를 인터넷으로 포워딩하지 말 것. 외부 접속이 필요하면 Tailscale 같은 VPN 사용
- 공유기 관리 페이지(`192.168.100.1`)에서 이 PC에 **IP 고정 할당** 걸어두기 (재부팅 시 IP 변동 방지)
- 안 쓰는 `Administrator` 계정은 비활성화 검토

---

## 트러블슈팅

| 증상 | 원인 | 확인 명령 |
|---|---|---|
| 응답 없이 멈춤 | 방화벽 차단 | `Get-NetConnectionProfile` |
| `Connection refused` | sshd 미실행 | `Get-Service sshd` |
| 비밀번호를 물어봄 | 키 인증 실패 | `icacls ...administrators_authorized_keys` |
| `bad ownership or modes` | 키 파일 권한 문제 | `icacls`로 재설정 |
| 접속 자체가 안 됨 | 네트워크 분리 | 양방향 `ping` 테스트 |

로그 확인:

```powershell
Get-WinEvent -LogName "OpenSSH/Operational" -MaxEvents 20 | Format-List TimeCreated, Message
```

클라이언트 쪽 상세 로그:

```bash
ssh -vvv bosung@192.168.100.136
```

---

## 참고 링크

- [Get started with OpenSSH Server for Windows](https://learn.microsoft.com/en-us/windows-server/administration/openssh/openssh_install_firstuse)
- [Windows OpenSSH 키 관리](https://learn.microsoft.com/en-us/windows-server/administration/openssh/openssh_keymanagement)
- [Windows OpenSSH 서버 구성](https://learn.microsoft.com/en-us/windows-server/administration/openssh/openssh_server_configuration)
- [빈 비밀번호 로컬 계정 정책](https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-10/security/threat-protection/security-policy-settings/accounts-limit-local-account-use-of-blank-passwords-to-console-logon-only)
