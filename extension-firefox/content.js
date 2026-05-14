// ============================================================================
// LassDownloader - Firefox Content Script
// ============================================================================

let floatingButtons = new Map();
let progressBars = new Map();

function addDownloadButtons() {
    const videos = document.querySelectorAll('video');
    videos.forEach(video => {
        if (!video.parentElement.querySelector('.lass-video-btn')) {
            const btn = document.createElement('button');
            btn.innerHTML = '⬇️ Download';
            btn.className = 'lass-video-btn';
            btn.style.cssText = `
                position: absolute;
                background: #0078D7;
                color: white;
                border: none;
                border-radius: 4px;
                padding: 4px 8px;
                font-size: 11px;
                cursor: pointer;
                z-index: 9999;
                font-family: Arial, sans-serif;
                box-shadow: 0 1px 3px rgba(0,0,0,0.3);
            `;
            
            btn.onclick = (e) => {
                e.stopPropagation();
                const src = video.src || video.currentSrc || video.querySelector('source')?.src;
                if (src) {
                    browser.runtime.sendMessage({
                        type: "download",
                        url: src,
                        filename: src.split('/').pop().split('?')[0] + '.mp4',
                        referrer: window.location.href
                    });
                }
            };
            
            video.parentElement.style.position = 'relative';
            video.parentElement.appendChild(btn);
            
            const rect = video.getBoundingClientRect();
            btn.style.top = rect.top + window.scrollY + 10 + 'px';
            btn.style.left = rect.left + window.scrollX + 10 + 'px';
            
            floatingButtons.set(video, btn);
        }
    });
}

function showFloatingButton(url, filename) {
    let floatBtn = document.getElementById('lass-float-btn');
    if (floatBtn) floatBtn.remove();
    
    floatBtn = document.createElement('div');
    floatBtn.id = 'lass-float-btn';
    floatBtn.innerHTML = `
        <div style="display: flex; align-items: center; gap: 10px;">
            <span>📥 ${filename.substring(0, 40)}</span>
            <button id="lass-download-yes" style="background:#4caf50; border:none; color:white; padding:4px 12px; border-radius:4px; cursor:pointer;">Download</button>
            <button id="lass-download-no" style="background:#f44336; border:none; color:white; padding:4px 12px; border-radius:4px; cursor:pointer;">Cancel</button>
        </div>
    `;
    floatBtn.style.cssText = `
        position: fixed;
        bottom: 20px;
        right: 20px;
        background: #2d2d2d;
        color: white;
        padding: 10px 15px;
        border-radius: 8px;
        font-family: Arial, sans-serif;
        font-size: 12px;
        z-index: 100000;
        box-shadow: 0 2px 10px rgba(0,0,0,0.3);
        border: 1px solid #444;
    `;
    document.body.appendChild(floatBtn);
    
    document.getElementById('lass-download-yes').onclick = () => {
        browser.runtime.sendMessage({
            type: "download",
            url: url,
            filename: filename,
            referrer: window.location.href
        });
        floatBtn.remove();
    };
    
    document.getElementById('lass-download-no').onclick = () => floatBtn.remove();
    
    setTimeout(() => {
        if (floatBtn && floatBtn.parentNode) floatBtn.remove();
    }, 30000);
}

function showProgressBar(downloadId, filename, progress, speed) {
    let progressBar = progressBars.get(downloadId);
    
    if (!progressBar && progress < 100) {
        progressBar = document.createElement('div');
        progressBar.className = 'lass-progress-bar';
        progressBar.style.cssText = `
            position: fixed;
            bottom: 20px;
            left: 20px;
            width: 300px;
            background: #2d2d2d;
            border-radius: 8px;
            padding: 8px 12px;
            color: white;
            font-family: Arial, sans-serif;
            font-size: 11px;
            z-index: 100000;
            box-shadow: 0 2px 8px rgba(0,0,0,0.3);
            border-left: 3px solid #0078D7;
        `;
        document.body.appendChild(progressBar);
        progressBars.set(downloadId, progressBar);
    }
    
    if (progressBar) {
        progressBar.innerHTML = `
            <div style="display: flex; justify-content: space-between; margin-bottom: 5px;">
                <span>📥 ${filename.substring(0, 30)}</span>
                <span>${Math.round(progress)}%</span>
            </div>
            <div style="background: #444; border-radius: 4px; height: 4px; overflow: hidden;">
                <div style="background: #0078D7; width: ${progress}%; height: 100%; transition: width 0.3s;"></div>
            </div>
            <div style="margin-top: 4px; font-size: 10px; color: #aaa;">${formatSpeed(speed)}</div>
        `;
        
        if (progress >= 100) {
            setTimeout(() => {
                if (progressBar && progressBar.parentNode) progressBar.remove();
                progressBars.delete(downloadId);
            }, 3000);
        }
    }
}

function formatSpeed(bytesPerSec) {
    if (!bytesPerSec) return '0 B/s';
    if (bytesPerSec < 1024) return `${bytesPerSec.toFixed(0)} B/s`;
    if (bytesPerSec < 1024*1024) return `${(bytesPerSec/1024).toFixed(1)} KB/s`;
    return `${(bytesPerSec/(1024*1024)).toFixed(1)} MB/s`;
}

browser.runtime.onMessage.addListener((message) => {
    if (message.type === "progress") {
        browser.storage.local.get(['showProgress'], (result) => {
            if (result.showProgress !== false) {
                showProgressBar(message.downloadId, message.filename, message.progress, message.speed);
            }
        });
    } else if (message.type === "showDownloadButton") {
        browser.storage.local.get(['autoCapture'], (result) => {
            if (!result.autoCapture) showFloatingButton(message.url, message.filename);
        });
    }
});

const observer = new MutationObserver(addDownloadButtons);
observer.observe(document.body, { childList: true, subtree: true });
addDownloadButtons();

window.addEventListener('scroll', () => {
    floatingButtons.forEach((btn, video) => {
        const rect = video.getBoundingClientRect();
        btn.style.top = rect.top + window.scrollY + 10 + 'px';
        btn.style.left = rect.left + window.scrollX + 10 + 'px';
    });
});
