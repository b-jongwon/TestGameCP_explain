# Stealth Adventure - System Programming Project

터미널 기반 스테이지형 잠입 게임입니다.

## 구조

- `src/` : C 소스 코드
- `include/` : 헤더 파일
- `assets/` : 맵 파일 및 기록 파일

## 빌드 & 실행

```bash
make
./game
```

## 조작법

- `W`, `A`, `S`, `D` : 플레이어 이동
- `q` : 게임 종료
- `Ctrl+C` : 시그널로 안전 종료

## 기능 요약

- 3개의 스테이지 (`assets/stage1.map`, `stage2.map`, `stage3.map`)
- 움직이는 장애물(X)에 닿으면 게임 오버
- `G` 위치에 도달하면 스테이지 클리어
- 모든 스테이지 클리어 시 전체 플레이 시간 기록
- `assets/records.txt`에 최고 기록 저장
- `pthread`로 장애물 이동 스레드 구현
- `termios` + `fcntl`로 논블로킹 입력 처리
- `signal`로 SIGINT, SIGTERM 처리
