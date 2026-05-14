// polyfill.js
// یکسان‌سازی API بین Chrome و Firefox (برای Firefox)
// در Firefox از browser استفاده می‌شود، نیازی به polyfill نیست

if (typeof chrome === 'undefined' && typeof browser !== 'undefined') {
    window.chrome = browser;
}
