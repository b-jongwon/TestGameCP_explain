#ifndef SOUND_H
#define SOUND_H

// ===============================================
// BGM 및 제어 함수 (Non-blocking)
// ===============================================

// BGM을 백그라운드에서 실행하는 함수.
// filePath: 재생할 오디오 파일 경로 (예: "bgm/my_bgm_track.wav")
// loop: 1이면 반복 재생
void play_bgm(const char *filePath, int loop);

// 논블로킹 효과음 재생 함수
void play_sfx_nonblocking(const char *filePath);

// 백그라운드에서 재생 중인 BGM 프로세스를 종료하는 함수.
void stop_bgm();

// ===============================================
// 실패 사운드 함수 (Blocking)
// ===============================================

// 1. 일반 장애물 발각 시 소리 재생 (WAV 파일 사용)
void play_obstacle_caught_sound(const char *filePath);

// 2. 교수님 발각 시 음성 재생 (TTS 로직 구현 완료)
// TTS 메시지 텍스트 파일 경로를 인자로 받습니다.
void play_professor_caught_sound(const char *textFilePath);

#endif // SOUND_H