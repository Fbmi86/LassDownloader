// ============================================================================
// LassDownloader - Firefox Extension Background Script
// ============================================================================

const HOST_NAME = "com.lassdownloader.native";

let port = null;
let reconnectTimer = null;
let activeDownloads = new Map();

// ============================================================================
// Native Messaging Connection (Firefox)
// ============================================================================

function connectToNative() {
    if (port) {
        try {
            port.disconnect();
        } catch(e) {}
        port = null;
    }
    
    console.log("[LassDownloader] Connecting to native host...");
    
    try {
        port = browser.runtime.connectNative(HOST_NAME);
        
        port.onMessage.addListener((message) => {
            console.log("[LassDownloader] Received:", message);
            handleNativeMessage(message);
        });
        
        port.onDisconnect.addListener(() => {
            console.log("[LassDownloader] Disconnected from native");
            port = null;
            if (reconnectTimer) clearTimeout(reconnectTimer);
            reconnectTimer = setTimeout(connectToNative, 5000);
        });
        
        port.postMessage({ type: "ready", client: "firefox" });
        updateConnectionStatus(true);
        
    } catch(e) {
        console.error("[LassDownloader] Connection error:", e);
        updateConnectionStatus(false);
    }
}

function handleNativeMessage(message) {
    switch(message.type) {
        case "download_started":
            activeDownloads.set(message.downloadId, {
                filename: message.filename,
                progress: 0
            });
            updateBadge(0);
            break;
            
        case "progress":
            if (activeDownloads.has(message.downloadId)) {
                activeDownloads.get(message.downloadId).progress = message.progress;
                updateBadge(message.progress);
            }
            
            browser.tabs.query({}, (tabs) => {
                tabs.forEach(tab => {
                    browser.tabs.sendMessage(tab.id, {
                        type: "progress",
                        downloadId: message.downloadId,
                        filename: message.filename,
                        progress: message.progress,
                        speed: message.speed
                    }).catch(() => {});
                });
            });
            break;
            
        case "complete":
            if (activeDownloads.has(message.downloadId)) {
                activeDownloads.delete(message.downloadId);
                updateBadge(-1);
            }
            
            browser.notifications.create({
                type: "basic",
                iconUrl: browser.runtime.getURL("icons/icon48.png"),
                title: "✅ Download Complete",
                message: `${message.filename} downloaded successfully!`
            });
            break;
            
        case "error":
            browser.notifications.create({
                type: "basic",
                iconUrl: browser.runtime.getURL("icons/icon48.png"),
                title: "❌ Download Error",
                message: `Failed: ${message.error}`
            });
            break;
    }
}

function updateConnectionStatus(connected) {
    browser.storage.local.set({ nativeConnected: connected });
}

function updateBadge(progress) {
    if (progress === -1) {
        browser.browserAction.setBadgeText({ text: "" });
        return;
    }
    
    if (progress > 0 && progress < 100) {
        browser.browserAction.setBadgeText({ text: `${Math.round(progress)}%` });
        browser.browserAction.setBadgeBackgroundColor({ color: "#0078D7" });
    } else if (progress === 0) {
        browser.browserAction.setBadgeText({ text: "↓" });
        browser.browserAction.setBadgeBackgroundColor({ color: "#0078D7" });
    }
}

// ============================================================================
// Send to Downloader
// ============================================================================

function sendToDownloader(url, filename, referrer, tabId) {
    if (!port) {
        browser.notifications.create({
            type: "basic",
            iconUrl: browser.runtime.getURL("icons/icon48.png"),
            title: "LassDownloader",
            message: "Connecting to LassDownloader app..."
        });
        connectToNative();
        
        setTimeout(() => {
            if (port) sendToDownloader(url, filename, referrer, tabId);
        }, 2000);
        return;
    }
    
    port.postMessage({
        type: "download",
        url: url,
        filename: filename,
        referrer: referrer || "",
        tabId: tabId
    });
}

// ============================================================================
// Auto Capture Downloads (Firefox webRequest)
// ============================================================================

const downloadExtensions = [
    '.zip', '.rar', '.7z', '.tar', '.gz', '.bz2',
    '.exe', '.msi', '.apk', '.dmg', '.deb', '.rpm',
    '.pdf', '.doc', '.docx', '.xls', '.xlsx', '.ppt', '.pptx',
    '.mp4', '.mkv', '.avi', '.mov', '.wmv', '.flv', '.webm',
    '.mp3', '.flac', '.wav', '.aac', '.ogg', '.m4a',
    '.iso', '.img', '.bin', '.torrent'
];

browser.webRequest.onBeforeRequest.addListener(
    (details) => {
        if (details.type !== "main_frame") {
            const url = details.url;
            const isDownload = downloadExtensions.some(ext => 
                url.toLowerCase().includes(ext) || url.toLowerCase().endsWith(ext)
            );
            
            if (isDownload) {
                browser.storage.local.get(['autoCapture'], (result) => {
                    if (result.autoCapture) {
                        let filename = url.split('/').pop().split('?')[0];
                        sendToDownloader(url, filename, details.originUrl, details.tabId);
                    } else {
                        browser.tabs.sendMessage(details.tabId, {
                            type: "showDownloadButton",
                            url: url,
                            filename: url.split('/').pop().split('?')[0]
                        }).catch(() => {});
                    }
                });
            }
        }
    },
    { urls: ["<all_urls>"] },
    ["blocking"]
);

// ============================================================================
// Context Menu (Firefox)
// ============================================================================

browser.runtime.onInstalled.addListener(() => {
    browser.contextMenus.create({
        id: "download-with-lass",
        title: "Download with LassDownloader",
        contexts: ["link", "image", "video", "audio", "selection"]
    });
    
    browser.contextMenus.create({
        id: "download-all-links",
        title: "Download All Links with LassDownloader",
        contexts: ["page"]
    });
});

browser.contextMenus.onClicked.addListener((info, tab) => {
    if (info.menuItemId === "download-with-lass") {
        let url = info.linkUrl || info.srcUrl || info.selectionText || info.pageUrl;
        let filename = url.split('/').pop().split('?')[0];
        sendToDownloader(url, filename, tab.url, tab.id);
    } else if (info.menuItemId === "download-all-links") {
        browser.tabs.executeScript(tab.id, {
            code: `Array.from(document.querySelectorAll('a[href]')).map(a => a.href).filter(h => h.startsWith('http'))`
        }, (results) => {
            if (results && results[0]) {
                results[0].forEach(link => {
                    let filename = link.split('/').pop().split('?')[0];
                    sendToDownloader(link, filename, tab.url, tab.id);
                });
            }
        });
    }
});

// ============================================================================
// Message Handling
// ============================================================================

browser.runtime.onMessage.addListener((message, sender, sendResponse) => {
    if (message.type === "openApp") {
        sendToDownloader("", "", "", null);
        sendResponse({ status: "ok" });
    } else if (message.type === "ping") {
        sendResponse({ status: port ? "ok" : "error", connected: !!port });
    } else if (message.type === "download") {
        sendToDownloader(message.url, message.filename, message.referrer, sender.tab?.id);
        sendResponse({ status: "ok" });
    }
});

// ============================================================================
// Initialize
// ============================================================================

connectToNative();
browser.runtime.onStartup.addListener(() => setTimeout(connectToNative, 1000));
