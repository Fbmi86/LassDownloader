// lass_host.c
// Native Messaging Host - اتصال بین مرورگر و برنامه اصلی LassDownloader

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define BUFFER_SIZE 65536
#define MAX_URL_LEN 2048
#define MAX_FILENAME_LEN 512
#define MAX_DOWNLOADS 100

typedef struct {
    int id;
    char url[MAX_URL_LEN];
    char filename[MAX_FILENAME_LEN];
    int progress;
    double speed;
    HANDLE hProcess;
    int running;
} DownloadItem;

static DownloadItem g_downloads[MAX_DOWNLOADS];
static int g_download_count = 0;
static int g_next_id = 1;
static volatile int g_running = 1;

/* مسیر برنامه اصلی LassDownloader */
#define LASS_APP_PATH "C:\\Program Files\\LassDownloader\\lassdownloader.exe"

/* ============================================================================
   توابع کمکی
   ============================================================================ */

static void log_message(const char* msg) {
    char log_msg[1024];
    snprintf(log_msg, sizeof(log_msg), "[LassHost] %s\n", msg);
    OutputDebugStringA(log_msg);
}

static void send_message_to_browser(const char* json_msg) {
    uint32_t len = strlen(json_msg);
    fwrite(&len, sizeof(uint32_t), 1, stdout);
    fwrite(json_msg, 1, len, stdout);
    fflush(stdout);
}

/* ============================================================================
   ساخت پیام‌های JSON
   ============================================================================ */

static void send_download_started(int id, const char* filename, const char* url) {
    char msg[2048];
    snprintf(msg, sizeof(msg), 
             "{\"type\":\"download_started\",\"downloadId\":%d,\"filename\":\"%s\",\"url\":\"%s\"}",
             id, filename, url);
    send_message_to_browser(msg);
}

static void send_progress(int id, const char* filename, int progress, double speed) {
    char msg[1024];
    snprintf(msg, sizeof(msg),
             "{\"type\":\"progress\",\"downloadId\":%d,\"filename\":\"%s\",\"progress\":%d,\"speed\":%.0f}",
             id, filename, progress, speed);
    send_message_to_browser(msg);
}

static void send_complete(int id, const char* filename) {
    char msg[512];
    snprintf(msg, sizeof(msg),
             "{\"type\":\"complete\",\"downloadId\":%d,\"filename\":\"%s\"}",
             id, filename);
    send_message_to_browser(msg);
}

static void send_error(const char* error) {
    char msg[512];
    snprintf(msg, sizeof(msg),
             "{\"type\":\"error\",\"error\":\"%s\"}", error);
    send_message_to_browser(msg);
}

static void send_ready(void) {
    send_message_to_browser("{\"type\":\"ready\",\"status\":\"connected\"}");
}

/* ============================================================================
   شروع دانلود با اجرای برنامه اصلی
   ============================================================================ */

static int start_download(const char* url, const char* filename, const char* referrer) {
    (void)referrer;
    
    if (g_download_count >= MAX_DOWNLOADS) {
        send_error("Too many active downloads");
        return -1;
    }
    
    /* پیدا کردن اسلات خالی */
    int slot = -1;
    for (int i = 0; i < MAX_DOWNLOADS; i++) {
        if (g_downloads[i].id == 0) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        send_error("No available slot");
        return -1;
    }
    
    /* مقداردهی دانلود جدید */
    DownloadItem* dl = &g_downloads[slot];
    dl->id = g_next_id++;
    strncpy(dl->url, url, MAX_URL_LEN - 1);
    strncpy(dl->filename, filename, MAX_FILENAME_LEN - 1);
    dl->progress = 0;
    dl->speed = 0;
    dl->running = 1;
    
    /* ارسال پیام شروع */
    send_download_started(dl->id, dl->filename, dl->url);
    
    /* اجرای برنامه اصلی LassDownloader با پارامتر URL */
    char cmd_line[4096];
    snprintf(cmd_line, sizeof(cmd_line), 
             "\"%s\" --add-url \"%s\" --save-path \"C:\\Downloads\" --silent",
             LASS_APP_PATH, url);
    
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    
    if (CreateProcessA(NULL, cmd_line, NULL, NULL, FALSE, 
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hThread);
        dl->hProcess = pi.hProcess;
        
        /* شبیه‌سازی پیشرفت (در نسخه واقعی از Named Pipe استفاده می‌شود) */
        for (int p = 0; p <= 100; p += 10) {
            if (!dl->running) break;
            dl->progress = p;
            dl->speed = 1024 * 1024 * (p / 10.0 + 0.5);
            send_progress(dl->id, dl->filename, dl->progress, dl->speed);
            Sleep(200);
        }
        
        dl->progress = 100;
        send_complete(dl->id, dl->filename);
        
        WaitForSingleObject(dl->hProcess, 5000);
        CloseHandle(dl->hProcess);
    } else {
        send_error("Failed to launch LassDownloader");
        return -1;
    }
    
    /* پاکسازی */
    memset(dl, 0, sizeof(DownloadItem));
    g_download_count--;
    
    return dl->id;
}

/* ============================================================================
   پردازش پیام دریافتی از مرورگر
   ============================================================================ */

static void process_message(const char* json_msg) {
    log_message(json_msg);
    
    if (strstr(json_msg, "\"type\":\"download\"")) {
        char url[MAX_URL_LEN] = {0};
        char filename[MAX_FILENAME_LEN] = {0};
        char referrer[MAX_URL_LEN] = {0};
        
        /* استخراج URL */
        const char* url_start = strstr(json_msg, "\"url\":\"");
        if (url_start) {
            url_start += 7;
            const char* url_end = strchr(url_start, '\"');
            if (url_end) {
                int len = url_end - url_start;
                if (len < MAX_URL_LEN) strncpy(url, url_start, len);
            }
        }
        
        /* استخراج filename */
        const char* filename_start = strstr(json_msg, "\"filename\":\"");
        if (filename_start) {
            filename_start += 11;
            const char* filename_end = strchr(filename_start, '\"');
            if (filename_end) {
                int len = filename_end - filename_start;
                if (len < MAX_FILENAME_LEN) strncpy(filename, filename_start, len);
            }
        }
        
        /* استخراج referrer */
        const char* referrer_start = strstr(json_msg, "\"referrer\":\"");
        if (referrer_start) {
            referrer_start += 11;
            const char* referrer_end = strchr(referrer_start, '\"');
            if (referrer_end) {
                int len = referrer_end - referrer_start;
                if (len < MAX_URL_LEN) strncpy(referrer, referrer_start, len);
            }
        }
        
        if (strlen(url) > 0) {
            if (strlen(filename) == 0) {
                const char* last_slash = strrchr(url, '/');
                if (last_slash) strncpy(filename, last_slash + 1, MAX_FILENAME_LEN - 1);
                else strcpy(filename, "download");
            }
            start_download(url, filename, referrer);
        }
    }
    else if (strstr(json_msg, "\"type\":\"ready\"") || strstr(json_msg, "\"type\":\"ping\"")) {
        send_ready();
    }
    else if (strstr(json_msg, "\"type\":\"stop\"")) {
        g_running = 0;
    }
}

/* ============================================================================
   دریافت پیام از مرورگر
   ============================================================================ */

static char* receive_message(void) {
    uint32_t length;
    
    if (fread(&length, sizeof(uint32_t), 1, stdin) != 1) return NULL;
    if (length == 0 || length > BUFFER_SIZE) return NULL;
    
    char* buffer = (char*)malloc(length + 1);
    if (!buffer) return NULL;
    
    if (fread(buffer, 1, length, stdin) != length) {
        free(buffer);
        return NULL;
    }
    
    buffer[length] = '\0';
    return buffer;
}

/* ============================================================================
   تابع اصلی
   ============================================================================ */

int main(void) {
    log_message("LassDownloader Native Host starting...");
    
    memset(g_downloads, 0, sizeof(g_downloads));
    send_ready();
    
    while (g_running) {
        char* message = receive_message();
        if (message) {
            process_message(message);
            free(message);
        } else {
            if (feof(stdin)) break;
            Sleep(100);
        }
    }
    
    log_message("LassDownloader Native Host stopped");
    return 0;
}
