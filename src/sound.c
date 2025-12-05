#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <limits.h>

// pid_t, fork, kill, waitpid, setpgid 사용을 위한 필수 헤더
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include "sound.h"

// 백그라운드 BGM 프로세스의 PID를 저장할 전역 변수
static pid_t bgm_pid = -1;
static int aplay_available = -1;
static int espeak_available = -1;

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static int command_exists_in_path(const char *cmd)
{
    if (!cmd || !*cmd)
    {
        return 0;
    }

    if (strchr(cmd, '/'))
    {
        return access(cmd, X_OK) == 0;
    }

    const char *path_env = getenv("PATH");
    if (!path_env)
    {
        return 0;
    }

    const size_t cmd_len = strlen(cmd);
    const char *segment = path_env;
    while (*segment)
    {
        const char *delimiter = strchr(segment, ':');
        size_t segment_len = delimiter ? (size_t)(delimiter - segment) : strlen(segment);

        char directory[PATH_MAX];
        if (segment_len == 0)
        {
            strcpy(directory, ".");
        }
        else
        {
            if (segment_len >= sizeof(directory))
            {
                segment_len = sizeof(directory) - 1;
            }
            memcpy(directory, segment, segment_len);
            directory[segment_len] = '\0';
        }

        char full_path[PATH_MAX];
        if (snprintf(full_path, sizeof(full_path), "%s/%s", directory, cmd) < (int)sizeof(full_path))
        {
            if (access(full_path, X_OK) == 0)
            {
                return 1;
            }
        }

        if (!delimiter)
        {
            break;
        }
        segment = delimiter + 1;
    }

    return 0;
}

static int ensure_command_available(const char *cmd, int *cache, const char *install_hint)
{
    if (!cmd || !cache)
    {
        return 0;
    }

    if (*cache == 1)
        return 1;
    if (*cache == 0)
        return 0;

    *cache = command_exists_in_path(cmd) ? 1 : 0;
    if (!*cache)
    {
        if (install_hint)
        {
            fprintf(stderr, "[sound] '%s' 명령을 찾을 수 없습니다. '%s' 패키지를 설치해 사운드를 활성화하세요.\n", cmd, install_hint);
        }
        else
        {
            fprintf(stderr, "[sound] '%s' 명령을 PATH에서 찾을 수 없습니다.\n", cmd);
        }
    }
    return *cache;
}

/**
 * BGM을 백그라운드 프로세스로 실행합니다. (Non-blocking)
 */
void play_bgm(const char *filePath, int loop)
{

    if (bgm_pid != -1)
    {
        fprintf(stderr, "BGM is already playing (PID: %d).\n", bgm_pid);
        return;
    }

    if (!ensure_command_available("aplay", &aplay_available, "alsa-utils"))
    {
        return;
    }

    // 1. fork() 시스템 콜을 사용하여 자식 프로세스 생성
    bgm_pid = fork();

    if (bgm_pid == 0)
    {
        // --- 자식 프로세스 (BGM 재생) 영역 ---

        setpgid(0, 0); // 새로운 프로세스 그룹의 리더가 됨

        // 2. 반복 재생 구현을 위해 system() 루프 사용 (BGM이 끝날 때까지 블로킹 후 재시작)
        while (1)
        {
            char command[256];
            // 2> /dev/null 추가: 에러 메시지 무시
            sprintf(command, "aplay -q %s 2> /dev/null", filePath);

            if (system(command) == -1)
            {
                perror("aplay command failed in BGM loop");
                break;
            }

            if (!loop)
                break;
        }

        exit(0);
    }
    else if (bgm_pid < 0)
    {
        // Fork 실패
        perror("fork failed for BGM");
        bgm_pid = -1;
    }
}

/**
 * 백그라운드에서 재생 중인 BGM 프로세스를 종료합니다. (SIGKILL)
 */
void stop_bgm()
{
    if (bgm_pid > 0)
    {
        // SIGKILL(9) 사용: BGM 프로세스 그룹 전체를 강제 종료
        if (kill(-bgm_pid, SIGKILL) == 0)
        {
            // printf("\nBGM process group (Root PID %d) forcibly terminated by SIGKILL.\n", bgm_pid);
        }
        else
        {
            perror("Error killing BGM process group");
        }

        waitpid(bgm_pid, NULL, 0);
        bgm_pid = -1;
    }
}

// ----------------------------------------------------
// ✅ [추가된 함수] 논블로킹 효과음 재생 (아이템 획득용)
// ----------------------------------------------------

/**
 * 짧은 효과음을 논블로킹(Non-blocking) 방식으로 백그라운드에서 재생합니다.
 * (메인 루프 렉(딜레이) 방지)
 */
void play_sfx_nonblocking(const char *filePath)
{

    if (!ensure_command_available("aplay", &aplay_available, "alsa-utils"))
    {
        return;
    }

    pid_t pid = fork(); //

    if (pid == 0)
    {
        // --- 자식 프로세스 영역 (SFX 재생) ---

        // 쉘 명령어를 실행하고 소리가 끝나면 즉시 종료
        char command[256];
        // &를 붙이면 비동기지만, system()을 사용하고 exit(0)로 마무리하는 것이 더 확실한 정리가 됩니다.
        sprintf(command, "aplay -q %s", filePath);

        system(command);

        // 자식 프로세스는 소리가 끝난 후 반드시 종료되어야 합니다.
        exit(0);
    }
    else if (pid < 0)
    {
        perror("SFX fork failed");
    }
    // 부모 프로세스(메인 루프)는 즉시 리턴하여 작업을 계속합니다.
}

// ----------------------------------------------------
// 기존 블로킹 함수 (게임 오버용)
// ----------------------------------------------------

/**
 * 1. 일반 장애물 발각 시 소리 재생 (Blocking)
 */
void play_obstacle_caught_sound(const char *filePath)
{
    if (!ensure_command_available("aplay", &aplay_available, "alsa-utils"))
    {
        return;
    }

    char command[256];

    sprintf(command, "aplay -q %s", filePath);

    // system() 호출: 소리 재생이 끝날 때까지 메인 프로세스를 블로킹 (게임 오버 연출용)
    if (system(command) == -1)
    {
        perror("Error executing sound command for obstacle");
    }
}