#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h> 

// pid_t, fork, kill, waitpid, setpgid 사용을 위한 필수 헤더
#include <sys/types.h>
#include <unistd.h>     
#include <sys/wait.h> 

#include "sound.h"

// 백그라운드 BGM 프로세스의 PID를 저장할 전역 변수
static pid_t bgm_pid = -1; 

/**
 * BGM을 백그라운드 프로세스로 실행합니다. (Non-blocking)
 * execvp()를 사용하여 PID 추적의 정확도를 높입니다.
 */
void play_bgm(const char *filePath, int loop) {
    
    if (bgm_pid != -1) {
        fprintf(stderr, "BGM is already playing (PID: %d).\n", bgm_pid);
        return;
    }

    // 1. fork() 시스템 콜을 사용하여 자식 프로세스 생성
    bgm_pid = fork(); 
    
    if (bgm_pid == 0) {
        // --- 자식 프로세스 (BGM 재생) 영역 ---
        
        // 1. 새로운 프로세스 그룹의 리더가 됨 (종료를 확실하게 하기 위함)
        setpgid(0, 0); 
        
        // 2. aplay 명령의 인자 준비
        char *aplay_args[] = {"aplay", "-q", (char *)filePath, (char *)NULL};

        // 반복 재생 구현
        while (1) {
            // execvp()를 호출하여 현재 프로세스를 aplay로 대체합니다.
            execvp(aplay_args[0], aplay_args); 
            
            // 🚨 execvp()가 실패했을 때만 아래 코드가 실행됩니다.
            perror("Failed to execute aplay via execvp");
            
            if (!loop) break; // 반복 옵션이 없으면 루프 탈출
            
            usleep(100000); 
        }
        
        exit(0); // BGM 프로세스 종료
        
    } else if (bgm_pid < 0) {
        // Fork 실패
        perror("fork failed for BGM");
        bgm_pid = -1;
    }
}

/**
 * 백그라운드에서 재생 중인 BGM 프로세스를 종료합니다.
 */
void stop_bgm() {
    if (bgm_pid > 0) {
        // ✅ [수정] SIGKILL(9) 사용: SIGTERM 대신 SIGKILL을 사용하여 BGM을 즉시 강제 종료합니다.
        if (kill(-bgm_pid, SIGKILL) == 0) 
        {
            printf("\nBGM process group (Root PID %d) forcibly terminated by SIGKILL.\n", bgm_pid);
        } else {
            perror("Error killing BGM process group");
        }
        
        // waitpid()를 사용하여 자식 프로세스가 완전히 종료될 때까지 기다림
        waitpid(bgm_pid, NULL, 0); 

        bgm_pid = -1; 
    }
}

/**
 * 1. 일반 장애물 발각 시 소리 재생 (Blocking)
 */
void play_obstacle_caught_sound(const char *filePath) {
    char command[256];
    
    // aplay -q [파일명] (WAV 파일 재생)
    sprintf(command, "aplay -q %s", filePath); 
    
    printf("\n🔊 일반 장애물 사운드 재생: %s\n", filePath);
    
    // system() 호출: 소리 재생이 끝날 때까지 메인 프로세스를 블로킹
    if (system(command) == -1) {
        perror("Error executing sound command for obstacle");
    }
}

/**
 * 2. 교수님 발각 시 음성 재생 (TTS 파이프라인 구현, Blocking)
 * textFilePath에서 메시지를 읽어 TTS로 변환 후 재생합니다.
 */
void play_professor_caught_sound(const char *textFilePath) {
    char tts_command[512];
    char message[256] = {0}; 
    FILE *fp;

    // 1. 텍스트 파일에서 교수님 메시지 읽어오기 
    fp = fopen(textFilePath, "r");
    if (fp == NULL) {
        perror("Failed to open professor voice text file");
        return;
    }

    if (fgets(message, sizeof(message), fp) != NULL) {
        size_t len = strlen(message);
        if (len > 0 && message[len - 1] == '\n') {
            message[len - 1] = '\0';
        }
    }
    fclose(fp);

    if (strlen(message) == 0) {
        fprintf(stderr, "Professor message file is empty.\n");
        return;
    }

    // 2. TTS 파이프라인 명령어 생성 (espeak -> aplay)
    snprintf(tts_command, sizeof(tts_command),
             "echo \"%s\" | espeak -ven+f1 -k1 -s130 --stdout | aplay -q", 
             message);

    printf("\n📢 교수님 음성 (TTS) 재생: %s\n", message);

    // 3. system() 호출: 음성 재생이 끝날 때까지 블로킹
    if (system(tts_command) == -1) {
        perror("Error executing TTS pipeline command (espeak/aplay). Check if espeak is installed.");
    }
}