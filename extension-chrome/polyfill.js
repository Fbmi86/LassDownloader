// popup.js - Chrome Extension

document.addEventListener('DOMContentLoaded', () => {
    // بارگذاری تنظیمات
    chrome.storage.local.get(['autoCapture', 'notifications', 'showProgress'], (result) => {
        document.getElementById('autoCapture').checked = result.autoCapture || false;
        document.getElementById('notifications').checked = result.notifications !== false;
        document.getElementById('showProgress').checked = result.showProgress !== false;
    });
    
    // ذخیره تنظیمات
    document.getElementById('autoCapture').addEventListener('change', (e) => {
        chrome.storage.local.set({ autoCapture: e.target.checked });
    });
    
    document.getElementById('notifications').addEventListener('change', (e) => {
        chrome.storage.local.set({ notifications: e.target.checked });
    });
    
    document.getElementById('showProgress').addEventListener('change', (e) => {
        chrome.storage.local.set({ showProgress: e.target.checked });
    });
    
    // دکمه‌ها
    document.getElementById('openApp').addEventListener('click', () => {
        chrome.runtime.sendMessage({ type: "openApp" });
    });
    
    document.getElementById('testConnection').addEventListener('click', () => {
        const statusSpan = document.getElementById('connStatus');
        statusSpan.textContent = "🔄 Testing...";
        
        chrome.runtime.sendMessage({ type: "ping" }, (response) => {
            if (response && response.status === "ok") {
                statusSpan.innerHTML = "✅ Connected";
                statusSpan.className = "connected";
                setTimeout(() => {
                    statusSpan.innerHTML = "🟢 Connected";
                    statusSpan.className = "connected";
                }, 2000);
            } else {
                statusSpan.innerHTML = "❌ Not connected";
                statusSpan.className = "disconnected";
                setTimeout(() => {
                    statusSpan.innerHTML = "🔴 Disconnected";
                    statusSpan.className = "disconnected";
                }, 2000);
            }
        });
    });
    
    document.getElementById('addUrl').addEventListener('click', () => {
        const url = document.getElementById('quickUrl').value.trim();
        if (url) {
            chrome.runtime.sendMessage({
                type: "download",
                url: url,
                filename: url.split('/').pop().split('?')[0] || 'download',
                referrer: ""
            });
            document.getElementById('quickUrl').value = '';
            alert("URL added to download queue!");
        } else {
            alert("Please enter a valid URL");
        }
    });
    
    document.getElementById('about').addEventListener('click', (e) => {
        e.preventDefault();
        alert("LassDownloader Browser Extension\nVersion 1.0.0\n\n© 2024 LassDownloader Team");
    });
});
