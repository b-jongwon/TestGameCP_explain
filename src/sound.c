#include <SDL2/SDL.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdint.h>

#include "sound.h"

// 사운드
// - 효과음: SDL 오디오 콜백에서 믹싱
// - 끊김 방지: 워커 프로세스로 재생 요청만 전달
// - BGM: 외부 플레이어 사용(가능한 명령만 골라서)

// 백그라운드 BGM 프로세스의 PID를 저장할 전역 변수
static pid_t bgm_pid = -1;
static int aplay_available = -1;
static int espeak_available = -1;
static int say_available = -1; // macOS 'say' 명령어의 가용성 캐시
static pid_t g_sound_worker_pid = -1;
static int g_sound_pipe[2] = {-1, -1};
static int g_sound_worker_started = 0;
static SDL_AudioDeviceID g_sound_device = 0;

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

static int write_full(int fd, const void *buf, size_t len)
{
    const uint8_t *ptr = (const uint8_t *)buf;
    while (len > 0)
    {
        ssize_t written = write(fd, ptr, len);
        if (written < 0)
        {
            if (errno == EINTR)
                continue;
            return 0;
        }
        ptr += written;
        len -= (size_t)written;
    }
    return 1;
}

static int read_full(int fd, void *buf, size_t len)
{
    uint8_t *ptr = (uint8_t *)buf;
    while (len > 0)
    {
        ssize_t r = read(fd, ptr, len);
        if (r == 0)
        {
            return 0;
        }
        if (r < 0)
        {
            if (errno == EINTR)
                continue;
            return 0;
        }
        ptr += r;
        len -= (size_t)r;
    }
    return 1;
}

#define SOUND_PATH_MAX 1024

typedef struct
{
    uint16_t type;
    uint16_t path_len;
} __attribute__((packed)) SoundCommandHeader;

enum
{
    SOUND_CMD_PLAY = 1,
    SOUND_CMD_QUIT = 2,
    SOUND_CMD_PRELOAD = 3
};

typedef struct
{
    char path[SOUND_PATH_MAX + 1];
    Uint8 *data;
    Uint32 length;
} SoundCacheEntry;

#define SOUND_CACHE_MAX 64

typedef struct
{
    const SoundCacheEntry *sound;
    Uint32 offset;
    int active;
    pthread_cond_t cond;
} PlaybackSlot;

#define MAX_ACTIVE_PLAYBACKS 16

static PlaybackSlot g_playback_slots[MAX_ACTIVE_PLAYBACKS];
static pthread_mutex_t g_playback_mutex = PTHREAD_MUTEX_INITIALIZER;

static SoundCacheEntry *find_cached_sound(SoundCacheEntry *cache, int cache_count, const char *path)
{
    for (int i = 0; i < cache_count; ++i)
    {
        if (strcmp(cache[i].path, path) == 0)
        {
            return &cache[i];
        }
    }
    return NULL;
}

static SoundCacheEntry *load_sound_into_cache(const char *path, const SDL_AudioSpec *target_spec,
                                              SoundCacheEntry *cache, int *cache_count)
{
    if (*cache_count >= SOUND_CACHE_MAX)
    {
        return NULL;
    }

    SDL_AudioSpec wav_spec;
    Uint8 *wav_buf = NULL;
    Uint32 wav_len = 0;
    if (!SDL_LoadWAV(path, &wav_spec, &wav_buf, &wav_len))
    {
        return NULL;
    }

    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt, wav_spec.format, wav_spec.channels, wav_spec.freq,
                          target_spec->format, target_spec->channels, target_spec->freq) < 0)
    {
        SDL_FreeWAV(wav_buf);
        return NULL;
    }

    Uint8 *final_buf = NULL;
    Uint32 final_len = 0;

    if (cvt.needed)
    {
        cvt.len = (int)wav_len;
        cvt.buf = (Uint8 *)SDL_malloc(wav_len * cvt.len_mult);
        if (!cvt.buf)
        {
            SDL_FreeWAV(wav_buf);
            return NULL;
        }
        memcpy(cvt.buf, wav_buf, wav_len);
        SDL_FreeWAV(wav_buf);
        if (SDL_ConvertAudio(&cvt) != 0)
        {
            SDL_free(cvt.buf);
            return NULL;
        }
        final_buf = cvt.buf;
        final_len = (Uint32)cvt.len_cvt;
    }
    else
    {
        final_buf = (Uint8 *)SDL_malloc(wav_len);
        if (!final_buf)
        {
            SDL_FreeWAV(wav_buf);
            return NULL;
        }
        memcpy(final_buf, wav_buf, wav_len);
        final_len = wav_len;
        SDL_FreeWAV(wav_buf);
    }

    SoundCacheEntry *entry = &cache[(*cache_count)++];
    strncpy(entry->path, path, sizeof(entry->path) - 1);
    entry->path[sizeof(entry->path) - 1] = '\0';
    entry->data = final_buf;
    entry->length = final_len;
    return entry;
}

static void audio_mix_callback(void *userdata, Uint8 *stream, int len)
{
    (void)userdata;
    SDL_memset(stream, 0, len);
    pthread_mutex_lock(&g_playback_mutex);
    int16_t *dst = (int16_t *)stream;
    int total_samples = len / (int)sizeof(int16_t);

    for (int i = 0; i < MAX_ACTIVE_PLAYBACKS; ++i)
    {
        PlaybackSlot *slot = &g_playback_slots[i];
        if (!slot->active || !slot->sound)
        {
            continue;
        }

        Uint32 remaining_bytes = slot->sound->length - slot->offset;
        if (remaining_bytes == 0)
        {
            slot->active = 0;
            pthread_cond_signal(&slot->cond);
            continue;
        }

        int bytes_to_mix = (int)((remaining_bytes < (Uint32)len) ? remaining_bytes : (Uint32)len);
        int samples_to_mix = bytes_to_mix / (int)sizeof(int16_t);
        const int16_t *src = (const int16_t *)(slot->sound->data + slot->offset);

        for (int s = 0; s < samples_to_mix && s < total_samples; ++s)
        {
            int mixed = dst[s] + src[s];
            if (mixed > 32767)
                mixed = 32767;
            else if (mixed < -32768)
                mixed = -32768;
            dst[s] = (int16_t)mixed;
        }

        slot->offset += (Uint32)(samples_to_mix * (int)sizeof(int16_t));
        if (slot->offset >= slot->sound->length)
        {
            slot->active = 0;
            pthread_cond_signal(&slot->cond);
        }
    }

    pthread_mutex_unlock(&g_playback_mutex);
}

static void *playback_thread_main(void *arg)
{
    int slot_index = (int)(intptr_t)arg;
    pthread_mutex_lock(&g_playback_mutex);
    while (g_playback_slots[slot_index].active)
    {
        pthread_cond_wait(&g_playback_slots[slot_index].cond, &g_playback_mutex);
    }
    pthread_mutex_unlock(&g_playback_mutex);
    return NULL;
}

static void start_playback_in_worker(const SoundCacheEntry *entry)
{
    pthread_mutex_lock(&g_playback_mutex);
    int slot = -1;
    for (int i = 0; i < MAX_ACTIVE_PLAYBACKS; ++i)
    {
        if (!g_playback_slots[i].active)
        {
            slot = i;
            break;
        }
    }

    if (slot == -1)
    {
        pthread_mutex_unlock(&g_playback_mutex);
        return;
    }

    PlaybackSlot *ps = &g_playback_slots[slot];
    ps->sound = entry;
    ps->offset = 0;
    ps->active = 1;
    pthread_mutex_unlock(&g_playback_mutex);

    pthread_t thread;
    if (pthread_create(&thread, NULL, playback_thread_main, (void *)(intptr_t)slot) == 0)
    {
        pthread_detach(thread);
    }
    else
    {
        pthread_mutex_lock(&g_playback_mutex);
        ps->active = 0;
        pthread_mutex_unlock(&g_playback_mutex);
    }
}

static void sound_worker_loop(int read_fd)
{
    SDL_SetHint(SDL_HINT_AUDIO_RESAMPLING_MODE, "medium");
    if (SDL_Init(SDL_INIT_AUDIO) != 0)
    {
        _exit(1);
    }

    SDL_AudioSpec desired;
    SDL_zero(desired);
    desired.freq = 44100;
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples = 1024;
    desired.callback = audio_mix_callback;

    SDL_AudioDeviceID device = SDL_OpenAudioDevice(NULL, 0, &desired, NULL, 0);
    if (device == 0)
    {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        _exit(1);
    }
    g_sound_device = device;
    SDL_PauseAudioDevice(device, 0);

    for (int i = 0; i < MAX_ACTIVE_PLAYBACKS; ++i)
    {
        g_playback_slots[i].sound = NULL;
        g_playback_slots[i].offset = 0;
        g_playback_slots[i].active = 0;
        pthread_cond_init(&g_playback_slots[i].cond, NULL);
    }

    SoundCacheEntry cache[SOUND_CACHE_MAX];
    int cache_count = 0;

    SoundCommandHeader header;
    while (read_full(read_fd, &header, sizeof(header)))
    {
        if (header.type == SOUND_CMD_PLAY)
        {
            uint16_t len = header.path_len;
            if (len > SOUND_PATH_MAX)
            {
                len = SOUND_PATH_MAX;
            }
            char path[SOUND_PATH_MAX + 1];
            if (!read_full(read_fd, path, len))
            {
                break;
            }
            path[len] = '\0';

            SoundCacheEntry *entry = find_cached_sound(cache, cache_count, path);
            if (!entry)
            {
                entry = load_sound_into_cache(path, &desired, cache, &cache_count);
            }
            if (entry)
            {
                start_playback_in_worker(entry);
            }
        }
        else if (header.type == SOUND_CMD_PRELOAD)
        {
            uint16_t len = header.path_len;
            if (len > SOUND_PATH_MAX)
            {
                len = SOUND_PATH_MAX;
            }
            char path[SOUND_PATH_MAX + 1];
            if (!read_full(read_fd, path, len))
            {
                break;
            }
            path[len] = '\0';

            SoundCacheEntry *entry = find_cached_sound(cache, cache_count, path);
            if (!entry)
            {
                load_sound_into_cache(path, &desired, cache, &cache_count);
            }
        }
        else if (header.type == SOUND_CMD_QUIT)
        {
            break;
        }
    }

    close(read_fd);
    for (int i = 0; i < cache_count; ++i)
    {
        SDL_free(cache[i].data);
    }

    SDL_CloseAudioDevice(device);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    _exit(0);
}

static void shutdown_sound_worker(void)
{
    if (g_sound_worker_pid <= 0)
    {
        return;
    }

    SoundCommandHeader header = {SOUND_CMD_QUIT, 0};
    write_full(g_sound_pipe[1], &header, sizeof(header));
    close(g_sound_pipe[1]);
    g_sound_pipe[1] = -1;
    waitpid(g_sound_worker_pid, NULL, 0);
    g_sound_worker_pid = -1;
    g_sound_worker_started = 0;
}

static int ensure_sound_worker_started(void)
{
    if (g_sound_worker_started)
    {
        return 1;
    }

    if (pipe(g_sound_pipe) != 0)
    {
        return 0;
    }

    fcntl(g_sound_pipe[0], F_SETFD, FD_CLOEXEC);
    fcntl(g_sound_pipe[1], F_SETFD, FD_CLOEXEC);

    pid_t pid = fork();
    if (pid == 0)
    {
        close(g_sound_pipe[1]);
        sound_worker_loop(g_sound_pipe[0]);
        _exit(0);
    }
    else if (pid < 0)
    {
        close(g_sound_pipe[0]);
        close(g_sound_pipe[1]);
        g_sound_pipe[0] = g_sound_pipe[1] = -1;
        return 0;
    }

    close(g_sound_pipe[0]);
    g_sound_pipe[0] = -1;
    g_sound_worker_pid = pid;
    g_sound_worker_started = 1;
    atexit(shutdown_sound_worker);
    return 1;
}

static int send_sound_command(uint16_t type, const char *path)
{
    if (g_sound_pipe[1] == -1)
    {
        return 0;
    }

    SoundCommandHeader header;
    header.type = type;
    header.path_len = 0;

    uint16_t len = 0;
    if (path)
    {
        size_t raw_len = strlen(path);
        if (raw_len > SOUND_PATH_MAX)
        {
            raw_len = SOUND_PATH_MAX;
        }
        len = (uint16_t)raw_len;
    }
    header.path_len = len;

    if (!write_full(g_sound_pipe[1], &header, sizeof(header)))
    {
        return 0;
    }

    if (len > 0 && path)
    {
        if (!write_full(g_sound_pipe[1], path, len))
        {
            return 0;
        }
    }

    return 1;
}

static void preload_known_sounds(void)
{
    static int done = 0;
    if (done)
    {
        return;
    }

    if (!ensure_sound_worker_started())
    {
        return;
    }

    static const char *kPreloadList[] = {
        "bgm/Get_Bag.wav",
        "bgm/Get_Item.wav",
        "bgm/Use_Item.wav",
        "bgm/Next_Level.wav",
        "bgm/No_Item.wav",
        "bgm/Walking.wav",
        "bgm/B1_SkillA.wav",
        "bgm/B1_SkillB.wav",
        "bgm/Professor_lv2.wav",
        "bgm/Professor_lv3.wav",
        "bgm/Professor_lv5.wav",
        "bgm/Professor_lv6.wav"};

    const size_t count = sizeof(kPreloadList) / sizeof(kPreloadList[0]);
    for (size_t i = 0; i < count; ++i)
    {
        send_sound_command(SOUND_CMD_PRELOAD, kPreloadList[i]);
    }

    done = 1;
}

void init_sound_system(void)
{
    if (!ensure_sound_worker_started())
    {
        return;
    }
    preload_known_sounds();
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
void stop_bgm(void)
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
// 논블로킹 효과음 재생 (아이템 획득용)
// ----------------------------------------------------

/**
 * 짧은 효과음을 논블로킹(Non-blocking) 방식으로 백그라운드에서 재생합니다.
 * (메인 루프 렉(딜레이) 방지)
 */
void play_sfx_nonblocking(const char *filePath)
{
    if (filePath && ensure_sound_worker_started())
    {
        if (send_sound_command(SOUND_CMD_PLAY, filePath))
        {
            return;
        }
    }

    if (!ensure_command_available("aplay", &aplay_available, "alsa-utils"))
    {
        return;
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        execlp("aplay", "aplay", "-q", filePath, (char *)NULL);
        perror("aplay sfx");
        _exit(1);
    }
    else if (pid < 0)
    {
        perror("SFX fork failed");
    }
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

// ----------------------------------------------------
// TTS 음성 출력 (Blocking)
// ----------------------------------------------------

/**
 * TTS 엔진을 사용하여 텍스트를 음성으로 출력합니다. (Blocking)
 * 🚨 OS 환경에 따라 'say', 'espeak' 등의 명령어를 자동으로 시도합니다.
 */
void speak_tts_blocking(const char *text)
{
    char command[512];

    // 1. macOS 'say' 명령어 시도 (말하는 속도 -r 200 설정으로 명료성 증대)
    if (ensure_command_available("say", &say_available, NULL))
    {
        if (snprintf(command, sizeof(command), "say -r 200 '%s'", text) < (int)sizeof(command))
        {
            goto execute;
        }
    }

    // 2. Linux 'espeak' 명령어 시도 (볼륨 -a 180 설정으로 음량 증폭)
    else if (ensure_command_available("espeak", &espeak_available, "espeak"))
    {
        if (snprintf(command, sizeof(command), "espeak -v en-uk 200 '%s'", text) < (int)sizeof(command))
        {
            goto execute;
        }
    }

    // 3. TTS 명령어 없음
    else
    {
        fprintf(stderr, "TTS 명령을 실행할 수 없습니다. (say 또는 espeak 설치 필요)\n");
        return;
    }

execute:
    // system() 호출: TTS 재생이 끝날 때까지 메인 프로세스를 블로킹
    if (system(command) == -1)
    {
        fprintf(stderr, "Error executing TTS command: %s\n", command);
    }
}
