# Stealth Adventure - System Programming Project

SDL2 기반 스테이지형 잠입 게임입니다. 터미널에 문자를 뿌리던 기존 방식 대신 맵 타일에 이미지를 매핑해 렌더링합니다.

## 구조

- `src/` : 게임의 핵심 로직이 담긴 C 소스 코드
- `include/` : 헤더 파일 모음
- `assets/` : 맵 파일(.map), 이미지 리소스(.png), 사운드 파일(.wav) 및 기록 파일

## 의존성

- SDL2
- SDL2_image
- alsa-utils (`aplay` 기반 WAV 출력)
- espeak (교수님 발각 TTS 파이프라인)


### 필수 패키지 설치

Ubuntu/Debian 계열이라면 다음 명령으로 설치할 수 있습니다.

```bash
sudo apt-get update
sudo apt-get install build-essential libsdl2-dev libsdl2-image-dev alsa-utils
```

WSL/VM처럼 ALSA 기본 장치가 없는 환경에서는 `aplay`가 실패할 수 있으니 PulseAudio를 추가 설치하고 Windows(또는 호스트)에서 PulseAudio 서버를 실행해 주세요.

```bash
sudo apt install pulseaudio
```

## TTS bash (TTS 소리 출력, install 필요함)
sudo apt-get install espeak
sudo apt install libsdl2-ttf-dev


## 빌드 & 실행

```bash
make
./game
```

## 조작법

- `W`, `A`, `S`, `D`, `또는 방향키` : 플레이어 이동
- `K`, `spacebar` : 투사체 발사
- `q` : 게임 종료
- `Ctrl+C` : 시그널로 안전 종료


## 게임 목표

1. `S`에서 출발해 `G`(가방)을 회수합니다.
2. 가방을 든 상태로 다시 `F` 위치로 돌아가 탈출해야 스테이지가 클리어됩니다.

가방을 주우면 플레이어가 백팩을 멘 스프라이트로 변하고, 시작 지점에는 `exit.png`가 표시되어 탈출 위치를 안내합니다.

## 기능 요약

- 6개의 스테이지 (`assets/b1.map`, `assets/1f.map`, `assets/2f.map`, `assets/3f.map`, `assets/4f.map`, `assets/5f.map`) — 진행 순서: B1 → 1F → 2F → 3F → 4F → 5F
- SDL2 기반 이미지 렌더링 (floor/wall/player/professor/backpack/exit 텍스처)
- 방향/동작별 플레이어 애니메이션 (가방 유무에 따라 다른 시트 사용)
- 움직이는 장애물(X)에 닿으면 게임 오버
- 모든 스테이지 클리어 시 전체 플레이 시간 기록
- `assets/records.txt`에 최고 기록 저장
- `pthread`로 장애물 이동 스레드 구현
- SDL 이벤트를 통한 논블로킹 입력 처리
- `signal`로 SIGINT, SIGTERM 처리

## 아이템, 장애물 설명
- 쉴드 : 장애물(교수님 포함)과 부딧치면 1회 생존하고 동시에 장애물을 제거합니다.
- 스쿠터 : 20초 동안 이동 속도가 2배 증가합니다.
- 투사체 보급 : 투사체 5개를 추가로 얻습니다.
- 깨지는 벽 : 모습은 일반 벽과 동일합니다. 투사체로 3번 공격시 사라집니다.
- 트랩 : 모습은 일반 바닥과 동일합니다. 곳곳에 숨어있는 트랩을 지나가면 게임이 종료됩니다.

## 스테이지별 플레이 방법
- Stage1 : 김명석 교수님의 강의실에서 가방을 되찾고 계단까지 도망치세요! 교수님이 만들어내는 분신을 피해서 도망쳐야 합니다.
- Stage2 : 교수님의 시야에 들어오는 순간 플레이어의 게임 화면이 어두워집니다.
- Stage3 : 교수님의 시야에 들어오는 순간 교수님이 순간이동 하여 플레이어의 2칸 뒤로 이동합니다. 교수님의 순간이동은 플레이어가 발각된 순간부터 다음 스테이지로 넘어가기 전까지 반복됩니다.
- Stage4 : 김명옥 교수님의 강의실에서 가방을 되찾고 계단까지 도망치세요! 교수님은 교대급수 판정법으로 플레이어와 위치를 맞바꿀 수 있습니다.
- Stage5 : 김정근 교수님께서 Latency! 를 외치면 플레이어의 속도가 느려졌다가 4초간 서서히 회복합니다.
- Stage6 : 유일하게 교수님을 처치할수 있는 스테이지 입니다. 가방을 찾아서 오른쪽 끝의 부서지는 벽을 넘어 보스 공간으로 가면 교수님을 만날 수 있고 여러 스테이지의 패턴이 종합해서 등장합니다. 교수님을 투사체로 18번 공격해서 처치 시 탈출 가능합니다.


## 이미지 매핑

| 맵 문자 | 설명 | 텍스처 |
| --- | --- | --- |
| `#`, `@` | 벽 | `assets/image/wall64.png`
| 공백 | 바닥 | `assets/image/floor64.png`
| `G` | 목표 지점 | `assets/image/backpack64.png`
| `S` | 플레이어 시작위치 | `assets/image/player/`  & `player_backpack/`
| `F` | 출구 위치 | `assets/image/exit.PNG`
| `P` | 교수님 | `assets/image/김명석교수님.png` & `assets/image/이종택교수님.png` & `assets/image/김진욱교수님.png` & `assets/image/김명옥교수님.png` & `assets/image/김정근교수님.png` & `assets/image/한명균교수님.png`
| `V` | 세로 유령 | `assets/image/professor64.png`
| `H` | 가로 유령 | `assets/image/professor64.png`
| `R` | 원 유령 | `assets/image/professor64.png`
| `B` | 깨지는 벽 | `assets/image/wall64.png`
| `I` | 쉴드 아이템 | `assets/image/shield64.png`
| `E` | 스쿠터 아이템 | `assets/image/scooter64.png`
| `A` | 투사체 보급 아이템 | `assets/image/supply.png`

맵 파일을 수정하면 해당 문자의 위치에 자동으로 이미지가 렌더링됩니다.

