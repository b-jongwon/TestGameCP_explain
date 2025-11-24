# Stealth Adventure - System Programming Project

SDL2 기반 스테이지형 잠입 게임입니다. 터미널에 문자를 뿌리던 기존 방식 대신 맵 타일에 이미지를 매핑해 렌더링합니다.

## 구조

- `src/` : C 소스 코드
- `include/` : 헤더 파일
- `assets/` : 맵 파일 및 기록 파일

## 의존성

- SDL2
- SDL2_image

Ubuntu/Debian 계열이라면 다음 명령으로 설치할 수 있습니다.

```bash
sudo apt install libsdl2-dev libsdl2-image-dev
```

## 빌드 & 실행

```bash
make
./game
```

## 조작법

- `W`, `A`, `S`, `D` : 플레이어 이동
- `q` : 게임 종료
- `Ctrl+C` : 시그널로 안전 종료

## 게임 목표

1. `S`에서 출발해 `G`(가방)을 회수합니다.
2. 가방을 든 상태로 다시 `S` 위치로 돌아가 탈출해야 스테이지가 클리어됩니다.

가방을 주우면 플레이어가 백팩을 멘 스프라이트로 변하고, 시작 지점에는 `exit.png`가 표시되어 탈출 위치를 안내합니다.

## 기능 요약

- 5개의 스테이지 (`assets/scaled_it5_*f.map`)
- SDL2 기반 이미지 렌더링 (floor/wall/player/professor/backpack/exit 텍스처)
- 방향/동작별 플레이어 애니메이션 (가방 유무에 따라 다른 시트 사용)
- 움직이는 장애물(X)에 닿으면 게임 오버
- 모든 스테이지 클리어 시 전체 플레이 시간 기록
- `assets/records.txt`에 최고 기록 저장
- `pthread`로 장애물 이동 스레드 구현
- SDL 이벤트를 통한 논블로킹 입력 처리
- `signal`로 SIGINT, SIGTERM 처리

## 추가 플레이 방법
- 키보드로 k 를 누르면 투사체 발사 가능 투사체의 갯수는 한정 돼있고 장애물을 3번 맞추면 장애물 파괴 (장애물 hp 와 투사체 갯수, 데미지는 코드에서 수정 가능합니다.)
- 아이템을 획득하면 1회 방어 가능 아이템을 가진채로 장애물에 부딧치면 장애물 파괴 가능 
  ( 부딧치면 장애물 파괴가 아니라 단순 쉴드가 사라지는걸로 구현하려고 했는데 자꾸 이상하게 버그가 일어나서 장애물 파괴 로직으로 구현했습니다.)
  
## 이미지 매핑

## 이미지 매핑

| 맵 문자 | 설명 | 텍스처 |
| --- | --- | --- |
| `#`, `@` | 벽 | `assets/image/wall64.png`
| 공백 | 바닥 | `assets/image/floor64.png`
| `G` | 목표 지점 | `assets/image/backpack64.png`
| 플레이어 | `P` 좌표 | `assets/image/player/` & `player_backpack/` 내 방향별 시트
| 장애물 `X` | 교수님 | `assets/image/professor64.png`

맵 파일을 수정하면 해당 문자의 위치에 자동으로 이미지가 렌더링됩니다.
