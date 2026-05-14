// create_icon.c
// ساخت آیکون ساده در زمان اجرا

#include <windows.h>

HICON CreateLassDownloaderIcon(void) {
    // ایجاد Bitmap ساده برای آیکون
    HBITMAP hbmColor = CreateBitmap(32, 32, 1, 32, NULL);
    HBITMAP hbmMask = CreateBitmap(32, 32, 1, 1, NULL);
    
    ICONINFO ii = {0};
    ii.fIcon = TRUE;
    ii.hbmColor = hbmColor;
    ii.hbmMask = hbmMask;
    
    HICON hIcon = CreateIconIndirect(&ii);
    
    DeleteObject(hbmColor);
    DeleteObject(hbmMask);
    
    return hIcon;
}

// استفاده در gui_window.c:
// g_hAppIcon = CreateLassDownloaderIcon();
