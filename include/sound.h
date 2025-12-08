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

// TTS(텍스트 음성 변환) 기능: 주어진 텍스트를 음성으로 출력 (Blocking)
void speak_tts_blocking(const char *text);

// 백그라운드에서 재생 중인 BGM 프로세스를 종료하는 함수.
void stop_bgm();

// ===============================================
// 실패 사운드 함수 (Blocking)
// ===============================================

// 1. WAV 파일 경로를 받아 장애물에 걸렸을 때 소리를 재생하는 함수 (Blocking)
void play_obstacle_caught_sound(const char *filePath);

#endif // SOUND_H