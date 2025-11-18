#include <fcntl.h>              // open(), O_RDONLY, O_WRONLY 등 파일 열기 관련 함수/매크로
#include <unistd.h>             // read(), write(), close() 같은 POSIX 저수준 파일 I/O 함수
#include <stdio.h>              // printf(), perror(), snprintf() 등 표준 입출력 함수
#include <stdlib.h>             // atof() 문자열→실수 변환 함수

// 기록 파일의 위치를 하드코딩한 매크로.
// 게임이 실행되면 여기서 최고 기록을 읽거나 쓴다.
#define RECORD_FILE "assets/records.txt"


// --------------------------------------------------------------
// load_best_record()
// --------------------------------------------------------------
// * 기록 파일을 열어서 저장된 최고 기록을 불러온다.
// * 기록 파일이 없거나 내용이 비정상적이면 0.0을 반환한다.
// * 0.0은 "기록 없음"을 의미하도록 설계되어 있음.
// --------------------------------------------------------------
double load_best_record(void) {

    // RECORD_FILE 파일을 읽기 전용 모드(O_RDONLY)로 연다.
    // open()은 성공 시 파일 디스크립터(fd)를 반환하고 실패 시 -1 반환.
    int fd = open(RECORD_FILE, O_RDONLY);

    // 파일이 존재하지 않거나 열 수 없는 경우.
    // ex) 최초 실행 시 파일이 없을 때.
    if (fd < 0) return 0.0;

    // 파일에서 읽어올 데이터를 저장할 임시 버퍼.
    char buf[64];

    // read(fd, buf, 최대크기) : 파일에서 buf로 데이터를 읽는다.
    // 반환값 n = 실제 읽은 바이트 수.
    // 파일 끝이거나 에러 시 0 이하.
    ssize_t n = read(fd, buf, sizeof(buf) - 1);

    // 파일 디스크립터 반드시 닫기.
    close(fd);

    // 파일에 내용이 없거나 read 실패 → 기록 없음 처리
    if (n <= 0) return 0.0;

    // 문자열 끝에 '\0'을 넣어 정상적인 C 문자열로 만든다.
    buf[n] = '\0';

    // buf 문자열을 실수로 변환해 반환(예: "12.34" → 12.34).
    return atof(buf);
}


// --------------------------------------------------------------
// update_record_if_better(double new_time)
// --------------------------------------------------------------
// * new_time(이번 플레이 시간)이 최고 기록보다 좋으면 갱신.
// * 최고 기록(best)이 존재하고 (best > 0),
//   best <= new_time 이면 기록 갱신 안 함.
// * 반대로 best 가 없거나(new==0) new_time이 더 작으면 파일에 새 기록 저장.
// --------------------------------------------------------------
void update_record_if_better(double new_time) {

    // 현재까지 저장된 기록 불러오기
    double best = load_best_record();

    // 기록이 있고, 기존 기록이 더 좋거나 같으면 갱신 불필요
    if (best > 0.0 && best <= new_time) {
        printf("Best Record: %.3fs | Your Time: %.3fs\n", best, new_time);
        return;
    }

    // 새 기록을 저장하기 위한 파일 열기
    // O_CREAT : 파일 없으면 생성
    // O_TRUNC : 기존 내용 삭제 후 새로 씀
    // 권한 0644 : rw-r--r--
    int fd = open(RECORD_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    // 파일 열기 실패 시 오류 출력
    if (fd < 0) {
        perror("open");
        return;
    }

    // 기록을 문자열로 변환해서 파일에 쓸 준비
    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%.3f\n", new_time);

    // 파일에 기록 시간 쓰기
    write(fd, buf, len);

    // 파일 닫기
    close(fd);

    // 이전 기록이 없었다면 첫 기록으로 출력
    if (best <= 0.0) {
        printf("First Record! Time: %.3fs\n", new_time);
    }
    // 기존 기록보다 좋은 기록이면 갱신 로그 출력
    else {
        printf("New Record! Old: %.3fs -> New: %.3fs\n", best, new_time);
    }
}
