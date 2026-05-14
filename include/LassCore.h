/**
 * LassDownloader Core Engine
 * 
 * @file    LassCore.h
 * @author  LassDownloader Team
 * @version 1.0.0
 * @brief   هسته اصلی دانلود منیجر با الگوریتم‌های چندگانه و RLTC
 * 
 * این فایل شامل همه تعاریف عمومی، ساختارها و توابع اصلی
 * کتابخانه LassDownloader می‌باشد.
 */

#ifndef LASSCORE_H
#define LASSCORE_H

#ifdef __cplusplus
extern "C" {
#endif

/* =========================== */
/*    کتابخانه‌های استاندارد    */
/* =========================== */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

/* =========================== */
/*    تعاریف سراسری و ماکروها   */
/* =========================== */

#define LASS_VERSION_MAJOR    1
#define LASS_VERSION_MINOR    0
#define LASS_VERSION_PATCH    0

#define LASS_MAX_URL_LEN      4096
#define LASS_MAX_PATH_LEN     1024
#define LASS_MAX_MIRRORS      32
#define LASS_MAX_SEGMENTS     256
#define LASS_MAX_PLUGINS      64
#define LASS_MAX_STRATEGIES   8

/* کدهای خطا */
typedef enum {
    LASS_OK                  = 0,
    LASS_ERROR_GENERAL       = -1,
    LASS_ERROR_NETWORK       = -2,
    LASS_ERROR_FILE_IO       = -3,
    LASS_ERROR_MEMORY        = -4,
    LASS_ERROR_INVALID_PARAM = -5,
    LASS_ERROR_NOT_FOUND     = -6,
    LASS_ERROR_RESUME_FAILED = -7,
    LASS_ERROR_CHECKSUM      = -8,
    LASS_ERROR_RLTC_CORRUPT  = -9,
    LASS_ERROR_PLUGIN        = -10,
    LASS_ERROR_IPC           = -11
} LassError;

/* =========================== */
/*    ساختارهای اصلی (Opaque)   */
/* =========================== */

/* هندل‌های اصلی (مخفی کردن پیاده‌سازی) */
typedef struct LassSession      LassSession;
typedef struct LassDownload     LassDownload;
typedef struct LassPlugin       LassPlugin;
typedef struct LassMirror       LassMirror;
typedef struct LassSegment      LassSegment;

/* =========================== */
/*    ماژول Dynamic Static Aware (DSA) */
/* =========================== */

typedef enum {
    CONTENT_STATIC,         // فایل ثابت (فیلم، موزیک، آرشیو)
    CONTENT_DYNAMIC,        // محتوای زنده (لایو استریم)
    CONTENT_HYBRID,         // ترکیبی
    CONTENT_UNKNOWN
} ContentNature;

typedef enum {
    STRATEGY_CAAD,          // Content-Aware Adaptive Download
    STRATEGY_ICC,           // Intelligent Congestion Control  
    STRATEGY_NLPD,          // Non-linear Priority Download
    STRATEGY_DMD,           // Dynamic Mirror Discovery
    STRATEGY_HYBRID_MULTI   // ترکیبی از چند استراتژی
} PriorityStrategy;

typedef struct {
    ContentNature nature;
    PriorityStrategy primary_strategy;
    PriorityStrategy backup_strategies[LASS_MAX_STRATEGIES];
    int backup_count;
    float confidence_score;
    char detection_reason[256];
    uint64_t analysis_time_ms;
} DSA_Decision;

/* =========================== */
/*    ماژول RLTC (Reed Logic Track Checksum) */
/* =========================== */

typedef enum {
    CHECKSUM_XXHASH,        // 64-bit, سرعت بالا
    CHECKSUM_SHA256,        // 256-bit, امنیت بالا
    CHECKSUM_BLAKE3,        // 256-bit, سریع و امن
    CHECKSUM_RLTC_HYBRID    // ترکیبی از xxHash + Reed-Solomon
} ChecksumType;

typedef struct {
    int data_shards;        // تعداد شاردهای داده (مثلاً 10)
    int parity_shards;      // شاردهای تصحیح خطا (مثلاً 4)
    uint32_t block_size;    // سایز هر بلوک (بایت)
    ChecksumType checksum_type;
    float redundancy_factor; // مثلاً 1.4 = 40% فضای اضافه
    bool enable_auto_recovery;
    int recovery_attempts;
} RLTC_Config;

/* =========================== */
/*    وضعیت دانلود              */
/* =========================== */

typedef enum {
    DOWNLOAD_STATE_PENDING,
    DOWNLOAD_STATE_ANALYZING,
    DOWNLOAD_STATE_DOWNLOADING,
    DOWNLOAD_STATE_PAUSED,
    DOWNLOAD_STATE_COMPLETED,
    DOWNLOAD_STATE_FAILED,
    DOWNLOAD_STATE_CHECKING,
    DOWNLOAD_STATE_RECOVERING
} DownloadState;

typedef struct {
    uint64_t total_bytes;
    uint64_t downloaded_bytes;
    uint64_t downloaded_bytes_valid;   // تایید شده با RLTC
    uint64_t corrupted_bytes;           // بایت‌های خراب بازیابی شده
    double current_speed;               // بایت بر ثانیه
    double average_speed;
    int active_segments;
    int active_mirrors;
    DownloadState state;
    time_t start_time;
    time_t last_update_time;
    float progress_percent;
} DownloadProgress;

/* =========================== */
/*    مشخّصات دانلود            */
/* =========================== */

typedef struct {
    char url[LASS_MAX_URL_LEN];
    char output_path[LASS_MAX_PATH_LEN];
    char filename[256];
    uint64_t file_size;                 // 0 اگر نامشخص
    int num_segments;                   // تعداد بخش‌ها (پویا)
    int max_mirrors;                    // حداکثر آینه‌ها
    bool allow_resume;
    bool enable_rltc;
    RLTC_Config rltc_config;
    PriorityStrategy forced_strategy;   // اگر کاربر اجباری کرده باشد
    void (*progress_callback)(DownloadProgress* progress, void* user_data);
    void *user_data;
} DownloadConfig;

/* =========================== */
/*    توابع اصلی API            */
/* =========================== */

/* ----------- مدیریت نشست ----------- */
LassSession*   lass_session_create(void);
void           lass_session_destroy(LassSession* session);
LassError      lass_session_set_config(LassSession* session, const char* config_path);
const char*    lass_session_get_version(void);

/* ----------- مدیریت دانلود ----------- */
LassDownload*  lass_download_create(LassSession* session, const DownloadConfig* config);
LassError      lass_download_start(LassDownload* dl);
LassError      lass_download_pause(LassDownload* dl);
LassError      lass_download_resume(LassDownload* dl);
LassError      lass_download_cancel(LassDownload* dl);
LassError      lass_download_get_progress(LassDownload* dl, DownloadProgress* progress);
LassError      lass_download_set_priority(LassDownload* dl, int priority);
void           lass_download_destroy(LassDownload* dl);

/* ----------- مدیریت آینه‌ها ----------- */
LassError      lass_mirror_add(LassDownload* dl, const char* mirror_url);
LassError      lass_mirror_remove(LassDownload* dl, const char* mirror_url);
int            lass_mirror_get_count(LassDownload* dl);
LassMirror*    lass_mirror_get_best(LassDownload* dl);

/* ----------- سیستم RLTC ----------- */
LassError      lass_rltc_verify(const char* filepath, RLTC_Config* config);
LassError      lass_rltc_repair(const char* filepath, RLTC_Config* config);
uint64_t       lass_rltc_get_recovery_stats(LassDownload* dl);

/* ----------- مدیریت پلاگین ----------- */
LassError      lass_plugin_load(const char* plugin_path);
LassError      lass_plugin_unload(const char* plugin_name);
LassPlugin*    lass_plugin_get_by_name(const char* name);

/* ----------- پروتکل IPC بین ماژول‌ها ----------- */
typedef struct {
    uint32_t message_id;
    uint32_t module_id;
    uint32_t command;
    void* data;
    size_t data_size;
    int callback_fd;    // فایل دسکریپتور برای پاسخ
} LassIPC_Message;

LassError      lass_ipc_send(uint32_t target_module, LassIPC_Message* msg);
LassError      lass_ipc_broadcast(LassIPC_Message* msg);
LassError      lass_ipc_register_handler(uint32_t module_id, void (*handler)(LassIPC_Message*));

/* ----------- ابزارها ----------- */
const char*    lass_error_string(LassError code);
void           lass_set_log_level(int level);
void           lass_enable_debug(bool enable);

/* =========================== */
/*    ثابت‌های پروتکل IPC       */
/* =========================== */

#define LASS_IPC_MODULE_DSA       0x0001
#define LASS_IPC_MODULE_CAAD      0x0002
#define LASS_IPC_MODULE_ICC       0x0003
#define LASS_IPC_MODULE_NLPD      0x0004
#define LASS_IPC_MODULE_DMD       0x0005
#define LASS_IPC_MODULE_RLTC      0x0006
#define LASS_IPC_MODULE_STORAGE   0x0007
#define LASS_IPC_MODULE_PLUGIN    0x0008

/* دستورات عمومی IPC */
#define LASS_IPC_CMD_HEARTBEAT    0x1001
#define LASS_IPC_CMD_STATUS       0x1002
#define LASS_IPC_CMD_CONFIG_GET   0x1003
#define LASS_IPC_CMD_CONFIG_SET   0x1004
#define LASS_IPC_CMD_SHUTDOWN     0x1005

#ifdef __cplusplus
}
#endif

#endif /* LASSCORE_H */
