#include "WebControl.h"
#include <WiFi.h>
#include <ESPmDNS.h> 
#include "Display.h" 
#include "Pomodoro.h" 
#include "Reminders.h"
#include "Eyes.h" 

WebControl webControl;
extern int currentMode; 
extern Pomodoro pomodoro;
extern Reminders reminders;
extern Eyes eyes; 

namespace {
String htmlEscape(const String &value) {
    String escaped = "";
    escaped.reserve(value.length());

    for (uint16_t i = 0; i < value.length(); i++) {
        char c = value[i];
        if (c == '&') escaped += "&amp;";
        else if (c == '<') escaped += "&lt;";
        else if (c == '>') escaped += "&gt;";
        else if (c == '"') escaped += "&quot;";
        else if (c == '\'') escaped += "&#39;";
        else escaped += c;
    }

    return escaped;
}
}

void WebControl::begin() {
    preferences.begin("bot-config", false);
    
    targetSSID = preferences.getString("ssid", WIFI_SSID_DEFAULT); 
    targetPass = preferences.getString("pass", WIFI_PASS_DEFAULT);

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("PetBot-Setup", "12345678", 1); 

    if (targetSSID != "" && targetSSID != "YOUR_WIFI_NAME") {
        WiFi.begin(targetSSID.c_str(), targetPass.c_str());
    }

    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/save", HTTP_POST, [this]() { handleSaveWiFi(); });
    server.on("/mode", HTTP_GET, [this]() { handleSetMode(); });

    // --- Train Simba page ---
    server.on("/train", HTTP_GET, [this]() { handleTrainPage(); });
    server.on("/reminders", HTTP_GET, [this]() { handleReminderPage(); });
    server.on("/saveReminders", HTTP_POST, [this]() { handleSaveReminders(); });

    server.on("/showReminders", HTTP_GET, [this]() {
        currentMode = MODE_REMINDERS;
        reminders.startManualList();
        server.send(200, "text/plain", "OK");
    });

    // --- Set a specific eye animation ---
    // Usage: /setEye?id=0  (id maps to EyeState index below)
    server.on("/setEye", HTTP_GET, [this]() {
        if (server.hasArg("id")) {
            int id = server.arg("id").toInt();
            EyeState stateMap[] = {
                E_NEUTRAL,   // 0
                E_SLEEP,     // 1
                E_WAKEUP,    // 2
                E_HAPPY,     // 3
                E_SQUINT,    // 4
                E_SCAN,      // 5
                E_WINK_L,    // 6
                E_WINK_R,    // 7
                E_OFFENDED,  // 8
                E_SURPRISED, // 9
                E_SAD,       // 10
                E_ANGRY,     // 11
                E_CURIOUS,   // 12
                E_PETTING    // 13
            };
            const int stateCount = sizeof(stateMap) / sizeof(stateMap[0]);
            if (id >= 0 && id < stateCount) {
                currentMode = MODE_EYES;
                eyes.setState(stateMap[id]);
                server.send(200, "text/plain", "OK");
            } else {
                server.send(400, "text/plain", "Invalid eye id");
            }
        } else {
            server.send(400, "text/plain", "Missing id");
        }
    });
    
    server.on("/startPomo", HTTP_GET, [this]() { 
        if (server.hasArg("min")) {
            int m = server.arg("min").toInt();
            pomodoro.setDuration(m); 
            pomodoro.start();
            currentMode = MODE_POMODORO; 
        }
        server.send(200, "text/plain", "OK");
    });

    server.on("/stopPomo", HTTP_GET, [this]() { 
        pomodoro.stop();
        server.send(200, "text/plain", "OK");
    });

    server.on("/pet", HTTP_GET, [this]() { 
        currentMode = MODE_EYES; 
        eyes.setState(E_PETTING); 
        server.send(200, "text/plain", "OK");
    });
    
    server.begin();
}

void WebControl::update() {
    server.handleClient();

    if (millis() - lastStatusCheck > 5000) {
        lastStatusCheck = millis();
        
        if (WiFi.status() == WL_CONNECTED) {
            if (!isConnected) {
                isConnected = true;
                IPAddress ip = WiFi.localIP();
                
                if (MDNS.begin("simba")) {
                    // mDNS started
                }

                clearDisplayFast();
                display.setTextSize(1);
                drawCenteredText("WIFI CONNECTED!", 15);
                
                display.setTextSize(1);
                drawCenteredText("IP Address:", 35);
                drawCenteredText(ip.toString(), 50);
                
                display.display();
                delay(5000);
                clearDisplayFast();
                display.display();
            }
        } 
        else {
            if (isConnected) {
                isConnected = false;
            }
        }
    }
}

String WebControl::getIP() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    } else {
        return WiFi.softAPIP().toString();
    }
}

void WebControl::handleRoot() {
    server.send(200, "text/html", getHTML());
}

void WebControl::handleTrainPage() {
    server.send(200, "text/html", getTrainHTML());
}

void WebControl::handleReminderPage() {
    server.send(200, "text/html", getReminderHTML());
}

void WebControl::handleSaveReminders() {
    if (server.hasArg("items")) {
        reminders.setFromText(server.arg("items"));
        server.sendHeader("Location", "/reminders");
        server.send(303);
    } else {
        server.send(400, "text/plain", "Missing reminders");
    }
}

void WebControl::handleSetMode() {
    if (server.hasArg("id")) {
        int m = server.arg("id").toInt();
        if (m >= 0 && m < MODE_COUNT) {
            currentMode = m;
            if (currentMode == MODE_REMINDERS) {
                reminders.startManualList();
            }
        }
    }
    server.sendHeader("Location", "/");
    server.send(303); 
}

void WebControl::handleSaveWiFi() {
    if (server.hasArg("ssid") && server.hasArg("pass")) {
        String s = server.arg("ssid");
        String p = server.arg("pass");
        
        preferences.putString("ssid", s);
        preferences.putString("pass", p);
        
        clearDisplayFast();
        drawCenteredText("SAVING WIFI...", 30);
        display.display();
        
        server.send(200, "text/html", "<h1>Saved!</h1><p>Bot is restarting...</p>");
        delay(2000);
        ESP.restart(); 
    } else {
        server.send(400, "text/plain", "Missing Data");
    }
}

String WebControl::getReminderHTML() {
    String ptr = R"html(
<!DOCTYPE html>
<html>
<head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<title>Simba Reminders</title>
<style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        background: linear-gradient(135deg, #2c1810 0%, #1a0f0a 100%);
        color: #fff;
        margin: 0;
        min-height: 100vh;
        padding: 0 0 42px 0;
    }
    .header {
        background: linear-gradient(135deg, #3d2817 0%, #2c1810 100%);
        border-bottom: 2px solid #d4a574;
        box-shadow: 0 3px 15px rgba(0,0,0,0.4);
        min-height: 70px;
        padding: 0 18px;
        display: flex;
        align-items: center;
        gap: 12px;
        position: sticky;
        top: 0;
        z-index: 10;
    }
    .back-btn, button {
        background: linear-gradient(135deg, rgba(61,40,23,0.85), rgba(44,24,16,0.85));
        color: #f5deb3;
        border: 2px solid rgba(212,165,116,0.4);
        border-radius: 12px;
        padding: 12px 16px;
        font-weight: bold;
        text-transform: uppercase;
        letter-spacing: 1px;
        text-decoration: none;
        cursor: pointer;
    }
    .title { color: #f5deb3; font-size: 18px; font-weight: bold; letter-spacing: 2px; text-transform: uppercase; }
    .sub { color: #d4a574; font-size: 11px; margin-top: 3px; letter-spacing: 1px; }
    .shell {
        max-width: 720px;
        margin: 0 auto;
        padding: 18px 14px 0;
    }
    .card {
        max-width: 720px;
        margin: 18px auto;
        padding: 18px;
        background: linear-gradient(145deg, #3d2817, #2c1810);
        border: 1px solid rgba(212,165,116,0.25);
        border-radius: 18px;
        box-shadow: 0 8px 25px rgba(0,0,0,0.35), inset 0 1px 0 rgba(245,222,179,0.08);
    }
    h3 {
        color: #f5deb3;
        margin: 0 0 14px 0;
        padding-bottom: 10px;
        border-bottom: 2px solid #d4a574;
        text-transform: uppercase;
        letter-spacing: 2px;
        font-size: 16px;
    }
    .stats {
        display: grid;
        grid-template-columns: repeat(3, 1fr);
        gap: 10px;
        margin-bottom: 16px;
    }
    .stat {
        background: rgba(212,165,116,0.1);
        border: 1px solid rgba(212,165,116,0.22);
        border-radius: 12px;
        padding: 12px 10px;
        text-align: center;
    }
    .stat strong {
        display: block;
        color: #f5deb3;
        font-size: 22px;
        line-height: 1;
    }
    .stat span {
        display: block;
        color: #d4a574;
        font-size: 11px;
        margin-top: 6px;
        text-transform: uppercase;
        letter-spacing: 1px;
    }
    .editor-head {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 12px;
        margin-bottom: 12px;
    }
    .pill {
        color: #2c1810;
        background: #f5deb3;
        border-radius: 999px;
        padding: 6px 11px;
        font-size: 12px;
        font-weight: 900;
        white-space: nowrap;
    }
    textarea {
        width: 100%;
        min-height: 210px;
        resize: vertical;
        border-radius: 12px;
        border: 2px solid rgba(212,165,116,0.35);
        background: rgba(26,15,10,0.55);
        color: #f5deb3;
        padding: 14px;
        font-size: 16px;
        line-height: 1.45;
        outline: none;
        box-shadow: inset 0 2px 12px rgba(0,0,0,0.22);
    }
    textarea:focus {
        border-color: #d4a574;
        box-shadow: 0 0 12px rgba(212,165,116,0.2), inset 0 2px 12px rgba(0,0,0,0.22);
    }
    textarea::placeholder { color: rgba(212,165,116,0.55); }
    .actions {
        display: grid;
        grid-template-columns: 1fr 1fr;
        gap: 12px;
        margin-top: 14px;
    }
    button { transition: transform 0.2s ease, box-shadow 0.2s ease; }
    button:active { transform: scale(0.98); }
    .btn-save {
        width: 100%;
        margin-top: 0;
        background: linear-gradient(135deg, #4682b4, #4169e1);
        color: white;
        border-color: #5f9ea0;
    }
    .btn-show {
        width: 100%;
        margin-top: 0;
        background: linear-gradient(135deg, #6b8e23, #556b2f);
        color: white;
        border-color: #8fb339;
    }
    .hint-row {
        display: flex;
        justify-content: space-between;
        gap: 10px;
        color: #d4a574;
        font-size: 12px;
        margin-top: 10px;
    }
    .list-grid {
        display: grid;
        grid-template-columns: 1fr 1fr;
        gap: 10px;
    }
    .item {
        display: flex;
        gap: 12px;
        align-items: flex-start;
        text-align: left;
        padding: 12px;
        min-height: 66px;
        border-radius: 14px;
        background: linear-gradient(145deg, rgba(61,40,23,0.9), rgba(44,24,16,0.9));
        border: 1px solid rgba(212,165,116,0.25);
        box-shadow: 0 6px 18px rgba(0,0,0,0.25);
    }
    .num {
        color: #2c1810;
        background: #f5deb3;
        border-radius: 50%;
        width: 26px;
        height: 26px;
        display: inline-flex;
        align-items: center;
        justify-content: center;
        font-weight: 900;
        flex-shrink: 0;
        box-shadow: 0 0 0 3px rgba(245,222,179,0.1);
    }
    .item p { color: #f5deb3; margin: 2px 0 0 0; line-height: 1.35; word-break: break-word; }
    .muted { color: #d4a574; font-size: 14px; }
    .empty {
        background: rgba(212,165,116,0.1);
        border: 1px dashed rgba(212,165,116,0.4);
        border-radius: 14px;
        padding: 24px;
        text-align: center;
        color: #d4a574;
    }
    #toast {
        position: fixed;
        left: 50%;
        bottom: 28px;
        transform: translateX(-50%) translateY(90px);
        background: linear-gradient(135deg, #f5deb3, #d4a574);
        color: #2c1810;
        padding: 12px 24px;
        border-radius: 25px;
        font-weight: bold;
        transition: transform 0.25s ease;
        box-shadow: 0 4px 15px rgba(0,0,0,0.3);
    }
    #toast.show { transform: translateX(-50%) translateY(0); }
    @media (max-width: 560px) {
        .shell { padding: 14px 10px 0; }
        .card { margin: 14px auto; padding: 16px; }
        .actions, .list-grid { grid-template-columns: 1fr; }
        .hint-row { flex-direction: column; }
        .stat strong { font-size: 18px; }
    }
</style>
</head>
<body>
    <div class="header">
        <a href="/" class="back-btn">&#8592; Back</a>
        <div>
            <div class="title">&#128276; Reminders</div>
            <div class="sub">OLED list and hourly flashes</div>
        </div>
    </div>

    <main class="shell">
    <div class="card">
        <div class="stats">
            <div class="stat"><strong id="itemCount">)html";

    ptr += String(reminders.getItemCount());

    ptr += R"html(</strong><span>Items</span></div>
            <div class="stat"><strong>1h</strong><span>Auto Flash</span></div>
            <div class="stat"><strong>10s</strong><span>Manual Step</span></div>
        </div>

        <div class="editor-head">
            <h3>Reminder List</h3>
            <span class="pill" id="liveCount">)html";

    ptr += String(reminders.getItemCount());

    ptr += R"html(/8 saved</span>
        </div>
        <form action="/saveReminders" method="POST">
            <textarea id="itemsBox" name="items" maxlength="520" placeholder="One reminder per line">)html";

    ptr += htmlEscape(reminders.getItemsAsText());

    ptr += R"html(</textarea>
            <div class="actions">
                <button class="btn-save" type="submit">&#128190; Save Reminders</button>
                <button class="btn-show" type="button" onclick="showNow()">&#128250; Show On OLED</button>
            </div>
        </form>
        <div class="hint-row">
            <span id="lineHint">Up to 8 reminders</span>
            <span id="charHint">0 characters</span>
        </div>
    </div>

    <div class="card">
        <h3>Saved Preview</h3>
)html";

    if (reminders.getItemCount() == 0) {
        ptr += "<div class='empty'>No reminders saved yet.</div>";
    } else {
        ptr += "<div class='list-grid'>";
        for (uint8_t i = 0; i < reminders.getItemCount(); i++) {
            ptr += "<div class='item'><span class='num'>";
            ptr += String(i + 1);
            ptr += "</span><p>";
            ptr += htmlEscape(reminders.getItem(i));
            ptr += "</p></div>";
        }
        ptr += "</div>";
    }

    ptr += R"html(
    </div>
    </main>

    <div id="toast"></div>
    <script>
        const box = document.getElementById('itemsBox');
        const liveCount = document.getElementById('liveCount');
        const lineHint = document.getElementById('lineHint');
        const charHint = document.getElementById('charHint');
        const itemCount = document.getElementById('itemCount');

        function countLines() {
            return box.value
                .split(/\r?\n/)
                .map(v => v.trim())
                .filter(Boolean)
                .slice(0, 8);
        }

        function updateStats() {
            const lines = countLines();
            liveCount.textContent = lines.length + '/8 ready';
            itemCount.textContent = lines.length;
            lineHint.textContent = lines.length >= 8 ? 'List is full' : (8 - lines.length) + ' slots free';
            charHint.textContent = box.value.length + ' characters';
        }

        function showNow() {
            fetch('/showReminders')
                .then(r => showToast(r.ok ? 'Showing on OLED' : 'Could not show list'))
                .catch(() => showToast('Connection failed'));
        }
        function showToast(message) {
            const toast = document.getElementById('toast');
            toast.textContent = message;
            toast.classList.add('show');
            setTimeout(() => toast.classList.remove('show'), 2200);
        }

        box.addEventListener('input', updateStats);
        updateStats();
    </script>
</body>
</html>
)html";
    return ptr;
}

// =====================================================
// TRAIN SIMBA PAGE
// Inherits the same dark-brown / wheat color scheme.
// All 14 eye states shown as tappable cards.
// =====================================================
String WebControl::getTrainHTML() {
    String ptr = R"html(
<!DOCTYPE html>
<html>
<head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<title>Train Simba</title>
<style>
    * { box-sizing: border-box; margin: 0; padding: 0; }

    body {
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        background: linear-gradient(135deg, #2c1810 0%, #1a0f0a 100%);
        color: #fff;
        min-height: 100vh;
        padding-bottom: 50px;
    }

    /* ---- Header — matches booted splash bar ---- */
    .header {
        background: linear-gradient(135deg, #3d2817 0%, #2c1810 100%);
        border-bottom: 2px solid #d4a574;
        box-shadow: 0 3px 15px rgba(0,0,0,0.4);
        padding: 0 20px;
        height: 70px;
        display: flex;
        align-items: center;
        gap: 14px;
        position: sticky;
        top: 0;
        z-index: 100;
    }

    /* Back button uses exact .btn-mode style */
    .back-btn {
        background: linear-gradient(135deg, rgba(61,40,23,0.85), rgba(44,24,16,0.85));
        color: #f5deb3;
        border: 2px solid rgba(212,165,116,0.4);
        border-radius: 12px;
        padding: 9px 16px;
        font-size: 13px;
        font-weight: bold;
        text-transform: uppercase;
        letter-spacing: 1.5px;
        cursor: pointer;
        text-decoration: none;
        transition: all 0.3s ease;
        flex-shrink: 0;
        position: relative;
        overflow: hidden;
    }
    .back-btn:hover {
        background: linear-gradient(135deg, rgba(75,50,30,0.9), rgba(61,40,23,0.9));
        border-color: #d4a574;
        box-shadow: 0 5px 14px rgba(212,165,116,0.25);
        transform: translateY(-2px);
    }
    .back-btn:active { transform: scale(0.97); }

    .header-text { flex: 1; }
    .header-title {
        font-size: 17px;
        font-weight: bold;
        color: #f5deb3;
        text-transform: uppercase;
        letter-spacing: 2px;
    }
    .header-sub {
        font-size: 11px;
        color: #d4a574;
        letter-spacing: 1px;
        margin-top: 2px;
    }

    /* ---- Active state banner — matches .status-display ---- */
    #active-banner {
        max-width: 450px;
        margin: 20px auto 0 auto;
        padding: 0 20px;
        display: none;
    }
    #active-banner.visible { display: block; }
    .banner-inner {
        background: rgba(212,165,116,0.12);
        border-left: 3px solid #f5deb3;
        border-radius: 0 12px 12px 0;
        padding: 12px 16px;
        display: flex;
        align-items: center;
        gap: 10px;
        font-size: 14px;
        color: #f5deb3;
    }
    .live-dot {
        width: 10px; height: 10px;
        border-radius: 50%;
        background: #8fb339;
        box-shadow: 0 0 8px #8fb339;
        flex-shrink: 0;
        animation: livePulse 1.2s ease-in-out infinite;
    }
    @keyframes livePulse {
        0%, 100% { transform: scale(1); opacity: 1; }
        50%       { transform: scale(1.5); opacity: 0.6; }
    }

    /* ---- Section labels — match h3 style ---- */
    .section-label {
        max-width: 450px;
        margin: 24px auto 10px auto;
        padding: 0 20px 8px 20px;
        color: #f5deb3;
        font-size: 17px;
        font-weight: bold;
        text-transform: uppercase;
        letter-spacing: 2px;
        border-bottom: 2px solid #d4a574;
    }

    /* ---- Card grid ---- */
    .eye-grid {
        display: grid;
        grid-template-columns: 1fr 1fr;
        gap: 12px;
        max-width: 450px;
        margin: 0 auto;
        padding: 0 20px;
    }

    /* ---- Eye card — inherits .card style ---- */
    .eye-card {
        background: linear-gradient(145deg, #3d2817, #2c1810);
        border: 1px solid rgba(212,165,116,0.25);
        border-radius: 18px;
        padding: 18px 12px 14px 12px;
        cursor: pointer;
        text-align: center;
        transition: transform 0.3s ease, box-shadow 0.3s ease;
        user-select: none;
        -webkit-tap-highlight-color: transparent;
        position: relative;
        overflow: hidden;
        box-shadow: 0 8px 25px rgba(0,0,0,0.35),
                    inset 0 1px 0 rgba(245,222,179,0.08);
    }
    /* Ripple effect — matches button::before */
    .eye-card::before {
        content: '';
        position: absolute;
        top: 50%; left: 50%;
        width: 0; height: 0;
        border-radius: 50%;
        background: rgba(255,255,255,0.15);
        transform: translate(-50%,-50%);
        transition: width 0.5s, height 0.5s;
    }
    .eye-card:active::before { width: 300px; height: 300px; }
    .eye-card:active         { transform: scale(0.97); }
    .eye-card:hover {
        transform: translateY(-3px);
        box-shadow: 0 12px 35px rgba(0,0,0,0.4),
                    inset 0 1px 0 rgba(245,222,179,0.12);
    }
    .eye-card.active {
        border-color: #d4a574;
        box-shadow: 0 8px 28px rgba(212,165,116,0.22),
                    inset 0 1px 0 rgba(245,222,179,0.12);
    }
    .eye-card.active::after {
        content: '✓';
        position: absolute;
        top: 8px; right: 11px;
        font-size: 12px;
        font-weight: 900;
        color: #f5deb3;
    }

    .eye-emoji {
        font-size: 32px;
        line-height: 1;
        margin-bottom: 8px;
        display: block;
    }
    .eye-name {
        color: #f5deb3;
        font-size: 13px;
        font-weight: bold;
        text-transform: uppercase;
        letter-spacing: 1px;
    }
    .eye-desc {
        color: #8b7355;
        font-size: 10px;
        margin-top: 4px;
        line-height: 1.3;
    }

    /* ---- Toast — identical to main page ---- */
    #toast-wrap {
        position: fixed;
        bottom: 30px;
        left: 0; right: 0;
        display: flex;
        justify-content: center;
        pointer-events: none;
        z-index: 1000;
    }
    #toast {
        background: linear-gradient(135deg, #f5deb3, #d4a574);
        color: #2c1810;
        padding: 12px 24px;
        border-radius: 25px;
        font-weight: bold;
        font-size: 14px;
        letter-spacing: 0.5px;
        box-shadow: 0 4px 15px rgba(0,0,0,0.3);
        transform: translateY(80px);
        transition: transform 0.3s cubic-bezier(0.34,1.56,0.64,1);
        white-space: nowrap;
    }
    #toast.show { transform: translateY(0); }

    /* ---- Footer ---- */
    .footer {
        color: #8b7355;
        text-align: center;
        font-size: 14px;
        letter-spacing: 1px;
        margin-top: 40px;
        margin-bottom: 10px;
    }
    .footer-emoji {
        display: inline-block;
        animation: bounce 2.5s ease-in-out infinite;
        margin-left: 5px;
    }
    @keyframes bounce {
        0%, 100% { transform: translateY(0); }
        50%       { transform: translateY(-6px); }
    }

    @media (max-width: 500px) {
        .eye-grid { gap: 10px; padding: 0 10px; }
        .section-label { padding: 0 10px 8px 10px; }
        #active-banner { padding: 0 10px; }
    }
</style>
</head>
<body>

<!-- Header -->
<div class="header">
    <a href="/" class="back-btn">&#8592; Back</a>
    <div class="header-text">
        <div class="header-title">&#127925; Train Simba</div>
        <div class="header-sub">Tap to display on OLED</div>
    </div>
</div>

<!-- Active state banner -->
<div id="active-banner">
    <div class="banner-inner">
        <div class="live-dot"></div>
        <span>Showing on OLED: <strong id="active-name">&#8212;</strong></span>
    </div>
</div>

<!-- MOODS -->
<div class="section-label">&#128149; Moods</div>
<div class="eye-grid">
    <div class="eye-card" id="card-0"  onclick="setEye(0,  'Neutral',   this)">
        <span class="eye-emoji">&#128528;</span>
        <div class="eye-name">Neutral</div>
        <div class="eye-desc">Calm default look</div>
    </div>
    <div class="eye-card" id="card-3"  onclick="setEye(3,  'Happy',     this)">
        <span class="eye-emoji">&#128513;</span>
        <div class="eye-name">Happy</div>
        <div class="eye-desc">Joyful bouncing eyes</div>
    </div>
    <div class="eye-card" id="card-10" onclick="setEye(10, 'Sad',       this)">
        <span class="eye-emoji">&#128546;</span>
        <div class="eye-name">Sad</div>
        <div class="eye-desc">Droopy downcast gaze</div>
    </div>
    <div class="eye-card" id="card-11" onclick="setEye(11, 'Angry',     this)">
        <span class="eye-emoji">&#128545;</span>
        <div class="eye-name">Angry</div>
        <div class="eye-desc">Tense vibrating glare</div>
    </div>
    <div class="eye-card" id="card-9"  onclick="setEye(9,  'Surprised', this)">
        <span class="eye-emoji">&#128561;</span>
        <div class="eye-name">Surprised</div>
        <div class="eye-desc">Wide-open shock</div>
    </div>
    <div class="eye-card" id="card-8"  onclick="setEye(8,  'Offended',  this)">
        <span class="eye-emoji">&#128064;</span>
        <div class="eye-name">Offended</div>
        <div class="eye-desc">Grumpy side-eye look</div>
    </div>
    <div class="eye-card" id="card-12" onclick="setEye(12, 'Curious',   this)">
        <span class="eye-emoji">&#129300;</span>
        <div class="eye-name">Curious</div>
        <div class="eye-desc">One eyebrow raised</div>
    </div>
    <div class="eye-card" id="card-4"  onclick="setEye(4,  'Squint',    this)">
        <span class="eye-emoji">&#129488;</span>
        <div class="eye-name">Squint</div>
        <div class="eye-desc">Sleepy relaxed lids</div>
    </div>
</div>

<!-- ACTIONS -->
<div class="section-label">&#127919; Actions</div>
<div class="eye-grid">
    <div class="eye-card" id="card-5"  onclick="setEye(5,  'Scan',       this)">
        <span class="eye-emoji">&#128270;</span>
        <div class="eye-name">Scan</div>
        <div class="eye-desc">Darting alert eyes</div>
    </div>
    <div class="eye-card" id="card-6"  onclick="setEye(6,  'Wink Left',  this)">
        <span class="eye-emoji">&#128521;</span>
        <div class="eye-name">Wink Left</div>
        <div class="eye-desc">Left eye wink</div>
    </div>
    <div class="eye-card" id="card-7"  onclick="setEye(7,  'Wink Right', this)">
        <span class="eye-emoji">&#128521;</span>
        <div class="eye-name">Wink Right</div>
        <div class="eye-desc">Right eye wink</div>
    </div>
    <div class="eye-card" id="card-13" onclick="setEye(13, 'Petting',    this)">
        <span class="eye-emoji">&#128062;</span>
        <div class="eye-name">Petting</div>
        <div class="eye-desc">Happy tongue-out bliss</div>
    </div>
</div>

<!-- REST -->
<div class="section-label">&#128564; Rest</div>
<div class="eye-grid">
    <div class="eye-card" id="card-1" onclick="setEye(1, 'Sleep',   this)">
        <span class="eye-emoji">&#128564;</span>
        <div class="eye-name">Sleep</div>
        <div class="eye-desc">Closed eyes, breathing</div>
    </div>
    <div class="eye-card" id="card-2" onclick="setEye(2, 'Wake Up', this)">
        <span class="eye-emoji">&#127774;</span>
        <div class="eye-name">Wake Up</div>
        <div class="eye-desc">Groggy multi-stage open</div>
    </div>
</div>

<p class="footer">Who&#39;s a good buddy? Simba is! <span class="footer-emoji">&#129454;</span></p>

<!-- Toast -->
<div id="toast-wrap"><div id="toast"></div></div>

<script>
    let activeCard = null;

    function setEye(id, name, cardEl) {
        if (activeCard) activeCard.classList.remove('active');
        cardEl.classList.add('active');
        activeCard = cardEl;

        document.getElementById('active-name').textContent = name;
        document.getElementById('active-banner').classList.add('visible');

        fetch('/setEye?id=' + id)
            .then(r => showToast(r.ok ? '\uD83D\uDC15 Simba shows: ' + name : '\u26A0 Error'))
            .catch(() => showToast('\u2717 Connection failed'));
    }

    function showToast(message) {
        const t = document.getElementById('toast');
        t.textContent = message;
        t.classList.add('show');
        setTimeout(() => t.classList.remove('show'), 2200);
    }
</script>
</body>
</html>
)html";
    return ptr;
}

String WebControl::getHTML() {
    // GOLDEN RETRIEVER THEME: Warm, playful, loyal companion
    String ptr = R"html(
<!DOCTYPE html>
<html>
<head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<title>Simba Control</title>
<style>
    * { box-sizing: border-box; }
    
    body { 
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
        text-align: center; 
        background: linear-gradient(135deg, #2c1810 0%, #1a0f0a 100%);
        color: #fff; 
        margin: 0; 
        padding: 0; 
        overflow-x: hidden;
        position: relative;
    }
    
    /* Paw print background pattern */
    body::before {
        content: '';
        position: fixed;
        top: 0; left: 0;
        width: 100%; height: 100%;
        background-image: 
            radial-gradient(circle at 20% 30%, rgba(212, 165, 116, 0.02) 0%, transparent 50%),
            radial-gradient(circle at 80% 70%, rgba(245, 222, 179, 0.02) 0%, transparent 50%);
        pointer-events: none;
        z-index: 0;
    }
    
    /* --- BOOT ANIMATION --- */
    #splash-screen {
        position: fixed; top: 0; left: 0; width: 100%; height: 100%;
        background: linear-gradient(135deg, #3d2817 0%, #1a0f0a 100%);
        z-index: 999;
        display: flex; 
        flex-direction: column;
        align-items: center; 
        justify-content: center;
        transition: all 1s cubic-bezier(0.68, -0.55, 0.265, 1.55);
    }
    
    .logo-container {
        display: flex; 
        align-items: center; 
        justify-content: center;
        flex-direction: column;
        animation: fadeInBounce 1.5s ease-out;
        position: relative;
    }
    
    @keyframes fadeInBounce {
        0% { opacity: 0; transform: scale(0.3) translateY(-100px) rotate(-15deg); }
        50% { transform: scale(1.08) translateY(8px) rotate(3deg); }
        70% { transform: scale(0.96) translateY(-3px) rotate(-2deg); }
        100% { opacity: 1; transform: scale(1) translateY(0) rotate(0deg); }
    }
    
    .logo-svg { 
        width: 160px; height: 160px; 
        transition: all 0.8s cubic-bezier(0.68, -0.55, 0.265, 1.55);
        filter: drop-shadow(0 8px 20px rgba(245, 222, 179, 0.25));
    }
    
    /* Tail wag animation - more playful */
    @keyframes tailWag {
        0%, 100% { transform: rotate(0deg); }
        25% { transform: rotate(-25deg); }
        75% { transform: rotate(25deg); }
    }
    
    .tail {
        transform-origin: 15% 20%;
        animation: tailWag 0.7s ease-in-out infinite;
    }

    /* Ear wiggle */
    @keyframes earWiggle {
        0%, 100% { transform: translateY(0) rotate(0deg); }
        50% { transform: translateY(-2px) rotate(-3deg); }
    }
    
    .ear-left, .ear-right {
        animation: earWiggle 1.8s ease-in-out infinite;
    }
    .ear-right { animation-delay: 0.5s; }
    
    /* Paw prints appearing */
    @keyframes pawFadeIn {
        0% { opacity: 0; transform: scale(0.3) rotate(-20deg); }
        100% { opacity: 0.8; transform: scale(1) rotate(0deg); }
    }
    
    .paw-print {
        position: absolute;
        font-size: 35px;
        opacity: 0;
        animation: pawFadeIn 0.5s ease-out forwards;
    }
    .paw-print:nth-child(1) { top: -55px; left: -65px; animation-delay: 0.2s; }
    .paw-print:nth-child(2) { top: -55px; right: -65px; animation-delay: 0.4s; }
    .paw-print:nth-child(3) { bottom: -55px; left: -65px; animation-delay: 0.6s; }
    .paw-print:nth-child(4) { bottom: -55px; right: -65px; animation-delay: 0.8s; }
    
    .app-title {
        font-size: 0px; opacity: 0; margin-top: 20px; 
        color: #f5deb3; font-weight: bold; letter-spacing: 5px;
        text-transform: uppercase; 
        transition: all 0.8s ease 0.8s;
        text-shadow: 0 2px 8px rgba(245, 222, 179, 0.3);
    }
    
    .loading-text {
        margin-top: 30px;
        color: #d4a574;
        font-size: 13px;
        letter-spacing: 2px;
        opacity: 0;
        animation: fadeInOut 2s ease-in-out infinite;
    }
    
    @keyframes fadeInOut {
        0%, 100% { opacity: 0.2; }
        50% { opacity: 0.8; }
    }

    /* Booted State - Move to top left */
    body.booted #splash-screen {
        height: 70px; 
        background: linear-gradient(135deg, #3d2817 0%, #2c1810 100%);
        box-shadow: 0 3px 15px rgba(0,0,0,0.4);
        justify-content: flex-start;
        flex-direction: row;
        padding-left: 20px;
        border-bottom: 2px solid #d4a574;
    }
    
    body.booted .logo-container { 
        flex-direction: row;
        animation: none;
    }
    
    body.booted .paw-print {
        display: none;
    }
    
    body.booted .loading-text {
        display: none;
    }
    
    body.booted .logo-svg { 
        width: 50px; height: 50px;
        filter: drop-shadow(0 2px 6px rgba(245, 222, 179, 0.2));
    }
    
    body.booted .app-title { 
        font-size: 24px; opacity: 1; 
        margin-top: 0; margin-left: 12px; 
    }
    
    /* Stop animations when booted */
    body.booted .tail,
    body.booted .ear-left,
    body.booted .ear-right {
        animation: none;
    }
    
    #main-content { 
        opacity: 0; 
        transform: translateY(60px); 
        transition: all 1s ease 1s; 
        padding-top: 95px; 
        padding-bottom: 50px;
        position: relative;
        z-index: 1;
    }
    body.booted #main-content { opacity: 1; transform: translateY(0); }

    /* --- CARDS & BUTTONS --- */
    .card { 
        background: linear-gradient(145deg, #3d2817, #2c1810);
        padding: 25px; 
        margin: 20px auto; 
        border-radius: 18px; 
        max-width: 450px; 
        box-shadow: 
            0 8px 25px rgba(0,0,0,0.35),
            inset 0 1px 0 rgba(245, 222, 179, 0.08);
        border: 1px solid rgba(212, 165, 116, 0.25);
        position: relative;
        overflow: hidden;
        transition: transform 0.3s ease, box-shadow 0.3s ease;
    }
    
    .card::before {
        content: '';
        position: absolute;
        top: 0; right: 0;
        width: 80px; height: 80px;
        background: radial-gradient(circle, rgba(245, 222, 179, 0.04) 0%, transparent 70%);
        pointer-events: none;
    }
    
    .card:hover {
        transform: translateY(-3px);
        box-shadow: 
            0 12px 35px rgba(0,0,0,0.4),
            inset 0 1px 0 rgba(245, 222, 179, 0.12);
    }
    
    h3 { 
        color: #f5deb3; 
        margin-top: 0; 
        margin-bottom: 18px;
        border-bottom: 2px solid #d4a574; 
        padding-bottom: 10px; 
        text-transform: uppercase; 
        letter-spacing: 2px;
        font-size: 17px;
        position: relative;
    }
    
    .icon {
        margin-right: 6px;
    }
    
    .status-display {
        background: rgba(212, 165, 116, 0.12);
        padding: 15px;
        border-radius: 12px;
        border-left: 3px solid #f5deb3;
        margin-top: 10px;
    }
    
    .mode-name {
        color: #f5deb3;
        font-weight: bold;
        font-size: 1.3em;
        text-shadow: 0 1px 6px rgba(245, 222, 179, 0.3);
    }
    
    input { 
        padding: 14px; 
        width: 85%; 
        margin: 10px 0; 
        border-radius: 12px; 
        border: 2px solid rgba(212, 165, 116, 0.3); 
        background: rgba(61, 40, 23, 0.5); 
        color: #f5deb3; 
        text-align: center;
        font-size: 16px;
        transition: all 0.3s ease;
    }
    
    input:focus {
        outline: none;
        border-color: #d4a574;
        box-shadow: 0 0 12px rgba(212, 165, 116, 0.2);
        background: rgba(61, 40, 23, 0.7);
    }
    
    input::placeholder {
        color: rgba(212, 165, 116, 0.5);
    }
    
    button { 
        padding: 14px; 
        width: 100%; 
        margin-top: 12px; 
        border: none; 
        border-radius: 12px; 
        font-weight: bold; 
        cursor: pointer; 
        transition: all 0.3s ease;
        text-transform: uppercase; 
        letter-spacing: 1.5px;
        font-size: 14px;
        position: relative;
        overflow: hidden;
    }
    
    button::before {
        content: '';
        position: absolute;
        top: 50%;
        left: 50%;
        width: 0;
        height: 0;
        border-radius: 50%;
        background: rgba(255, 255, 255, 0.15);
        transform: translate(-50%, -50%);
        transition: width 0.5s, height 0.5s;
    }
    
    button:active::before {
        width: 300px;
        height: 300px;
    }
    
    button:active { 
        transform: scale(0.97); 
    }
    
    /* Pet Button - Golden highlight */
    .btn-pet {
        background: linear-gradient(135deg, #f5deb3, #d4a574);
        color: #2c1810;
        box-shadow: 
            0 5px 18px rgba(245, 222, 179, 0.3),
            inset 0 1px 0 rgba(255, 255, 255, 0.25);
        border: 2px solid #f5deb3;
        font-size: 16px;
        font-weight: 900;
    }
    
    .btn-pet:hover { 
        background: linear-gradient(135deg, #fff8dc, #f5deb3);
        box-shadow: 
            0 7px 22px rgba(245, 222, 179, 0.4),
            inset 0 1px 0 rgba(255, 255, 255, 0.3);
        transform: translateY(-2px);
    }

    /* Mode Buttons - Warm brown */
    .btn-mode { 
        background: linear-gradient(135deg, rgba(61, 40, 23, 0.85), rgba(44, 24, 16, 0.85));
        color: #f5deb3; 
        border: 2px solid rgba(212, 165, 116, 0.4);
        box-shadow: 0 3px 10px rgba(0, 0, 0, 0.25);
    }
    
    .btn-mode:hover { 
        background: linear-gradient(135deg, rgba(75, 50, 30, 0.9), rgba(61, 40, 23, 0.9));
        border-color: #d4a574;
        box-shadow: 0 5px 14px rgba(212, 165, 116, 0.25);
        transform: translateY(-2px);
    }
    
    /* Start Timer Button */
    .btn-start {
        background: linear-gradient(135deg, #6b8e23, #556b2f);
        color: white;
        box-shadow: 0 5px 18px rgba(107, 142, 35, 0.3);
        border: 2px solid #8fb339;
    }
    
    .btn-start:hover {
        background: linear-gradient(135deg, #7ba428, #6b8e23);
        box-shadow: 0 7px 22px rgba(107, 142, 35, 0.4);
    }
    
    /* Stop Button - Red */
    .btn-stop { 
        background: linear-gradient(135deg, #cd5c5c, #a52a2a);
        color: white;
        box-shadow: 0 5px 18px rgba(205, 92, 92, 0.3);
        border: 2px solid #dc143c;
    }
    
    .btn-stop:hover {
        background: linear-gradient(135deg, #dc6b6b, #cd5c5c);
        box-shadow: 0 7px 22px rgba(205, 92, 92, 0.4);
    }
    
    /* Save WiFi Button */
    .btn-save {
        background: linear-gradient(135deg, #4682b4, #4169e1);
        color: white;
        box-shadow: 0 5px 18px rgba(70, 130, 180, 0.3);
        border: 2px solid #5f9ea0;
    }
    
    .btn-save:hover {
        background: linear-gradient(135deg, #5a9bd4, #4682b4);
        box-shadow: 0 7px 22px rgba(70, 130, 180, 0.4);
    }
    
    .mode-grid {
        display: grid; 
        grid-template-columns: 1fr 1fr; 
        gap: 12px;
    }
    
    .pomo-controls {
        display: flex; 
        gap: 12px; 
        justify-content: center; 
        align-items: center; 
        margin-bottom: 15px;
        background: rgba(61, 40, 23, 0.35);
        padding: 15px;
        border-radius: 12px;
    }
    
    .pomo-input { 
        width: 70px; 
        text-align: center; 
        font-size: 22px;
        font-weight: bold;
        background: rgba(245, 222, 179, 0.08);
        border: 2px solid #d4a574;
    }
    
    .pomo-label {
        color: #d4a574;
        font-size: 18px;
        font-weight: bold;
    }
    
    /* Footer */
    .footer { 
        color: #8b7355; 
        margin-top: 40px; 
        margin-bottom: 30px;
        font-size: 14px;
        letter-spacing: 1px;
    }
    
    .footer-emoji {
        display: inline-block;
        animation: bounce 2.5s ease-in-out infinite;
        margin-left: 5px;
    }
    
    @keyframes bounce {
        0%, 100% { transform: translateY(0); }
        50% { transform: translateY(-6px); }
    }
    
    /* Emoji animations in buttons */
    .btn-pet .emoji { 
        display: inline-block;
        transition: transform 0.3s ease;
    }
    .btn-pet:hover .emoji { 
        animation: wiggle 0.6s ease-in-out;
    }
    
    @keyframes wiggle {
        0%, 100% { transform: rotate(0deg); }
        25% { transform: rotate(-12deg); }
        75% { transform: rotate(12deg); }
    }
    
    /* Responsive */
    @media (max-width: 500px) {
        .card { margin: 15px 10px; padding: 20px; }
        .logo-svg { width: 130px; height: 130px; }
        body.booted .logo-svg { width: 45px; height: 45px; }
        body.booted .app-title { font-size: 20px; }
    }
</style>
</head>
<body>

    <div id="splash-screen">
        <div class="logo-container">
            <div class="paw-print">&#128062;</div>
            <div class="paw-print">&#128062;</div>
            <div class="paw-print">&#128062;</div>
            <div class="paw-print">&#128062;</div>
            
            <!-- Redesigned Clean Golden Retriever SVG -->
            <svg class="logo-svg" viewBox="0 0 120 120" xmlns="http://www.w3.org/2000/svg">
                <!-- Tail (cleaner, fluffier) -->
                <path class="tail" d="M85,68 Q92,72 94,82 Q95,86 93,90" 
                      fill="none" stroke="#d4a574" stroke-width="8" stroke-linecap="round"/>
                <path class="tail" d="M86,69 Q91,73 93,81" 
                      fill="none" stroke="#f5deb3" stroke-width="4" stroke-linecap="round"/>
                
                <!-- Body (rounder, friendlier) -->
                <ellipse cx="62" cy="68" rx="26" ry="21" fill="#f5deb3"/>
                <ellipse cx="62" cy="68" rx="22" ry="18" fill="#faf0e6"/>
                
                <!-- Head (bigger, rounder) -->
                <circle cx="50" cy="48" r="24" fill="#f5deb3"/>
                <circle cx="50" cy="48" r="20" fill="#faf0e6"/>
                
                <!-- Ears (cleaner floppy style) -->
                <ellipse class="ear-left" cx="34" cy="42" rx="10" ry="18" 
                         fill="#d4a574" transform="rotate(-35 34 42)"/>
                <ellipse class="ear-left" cx="35" cy="43" rx="7" ry="14" 
                         fill="#e6c896" transform="rotate(-35 35 43)"/>
                
                <ellipse class="ear-right" cx="66" cy="42" rx="10" ry="18" 
                         fill="#d4a574" transform="rotate(35 66 42)"/>
                <ellipse class="ear-right" cx="65" cy="43" rx="7" ry="14" 
                         fill="#e6c896" transform="rotate(35 65 43)"/>
                
                <!-- Snout (rounder) -->
                <ellipse cx="50" cy="56" rx="12" ry="10" fill="#faf0e6"/>
                <ellipse cx="50" cy="56" rx="10" ry="8" fill="#fff"/>
                
                <!-- Nose (cleaner, shinier) -->
                <ellipse cx="50" cy="58" rx="5.5" ry="4.5" fill="#3d2f24"/>
                <ellipse cx="49" cy="57" rx="2.5" ry="2" fill="#5a4a3a" opacity="0.7"/>
                <circle cx="48" cy="57" r="1.2" fill="#8a7a6a" opacity="0.5"/>
                
                <!-- Eyes (bigger, more expressive) -->
                <ellipse cx="41" cy="45" rx="4" ry="4.5" fill="#3d2f24"/>
                <ellipse cx="59" cy="45" rx="4" ry="4.5" fill="#3d2f24"/>
                
                <!-- Eye shine (bigger sparkle) -->
                <circle cx="42" cy="44" r="2" fill="#fff" opacity="0.9"/>
                <circle cx="60" cy="44" r="2" fill="#fff" opacity="0.9"/>
                <circle cx="43" cy="45" r="0.8" fill="#fff" opacity="0.7"/>
                <circle cx="61" cy="45" r="0.8" fill="#fff" opacity="0.7"/>
                
                <!-- Eyebrows (more defined) -->
                <path d="M37,41 Q41,38.5 45,41" fill="none" stroke="#c99566" 
                      stroke-width="1.8" stroke-linecap="round"/>
                <path d="M55,41 Q59,38.5 63,41" fill="none" stroke="#c99566" 
                      stroke-width="1.8" stroke-linecap="round"/>
                
                <!-- Mouth (happier smile) -->
                <path d="M50,61 Q44,64 41,63" fill="none" stroke="#3d2f24" 
                      stroke-width="2.2" stroke-linecap="round"/>
                <path d="M50,61 Q56,64 59,63" fill="none" stroke="#3d2f24" 
                      stroke-width="2.2" stroke-linecap="round"/>
                <line x1="50" y1="58" x2="50" y2="62" stroke="#3d2f24" 
                      stroke-width="2" stroke-linecap="round"/>
                
                <!-- Tongue (cuter) -->
                <ellipse cx="50" cy="65" rx="4.5" ry="3.5" fill="#ff9999"/>
                <ellipse cx="50" cy="64" rx="3.5" ry="2.5" fill="#ffb3b3"/>
                <line x1="50" y1="63" x2="50" y2="66" stroke="#ff8888" stroke-width="0.5"/>
                
                <!-- Front legs (cleaner) -->
                <rect x="42" y="80" width="7" height="18" rx="3.5" fill="#f5deb3"/>
                <rect x="43" y="81" width="5" height="17" rx="2.5" fill="#faf0e6"/>
                
                <rect x="56" y="80" width="7" height="18" rx="3.5" fill="#f5deb3"/>
                <rect x="57" y="81" width="5" height="17" rx="2.5" fill="#faf0e6"/>
                
                <!-- Paws (detailed toe beans) -->
                <ellipse cx="45.5" cy="97" rx="4" ry="3" fill="#d4a574"/>
                <ellipse cx="59.5" cy="97" rx="4" ry="3" fill="#d4a574"/>
                
                <!-- Toe beans detail -->
                <circle cx="44" cy="97" r="1.2" fill="#c99566"/>
                <circle cx="47" cy="97" r="1.2" fill="#c99566"/>
                <circle cx="45.5" cy="95" r="1.4" fill="#c99566"/>
                
                <circle cx="58" cy="97" r="1.2" fill="#c99566"/>
                <circle cx="61" cy="97" r="1.2" fill="#c99566"/>
                <circle cx="59.5" cy="95" r="1.4" fill="#c99566"/>
                
                <!-- Collar (cleaner design) -->
                <ellipse cx="50" cy="65" rx="14" ry="4" fill="#cd5c5c"/>
                <ellipse cx="50" cy="65" rx="12" ry="3" fill="#dc143c"/>
                
                <!-- Tag (shiny) -->
                <circle cx="57" cy="65" r="3" fill="#ffd700"/>
                <circle cx="57" cy="65" r="2" fill="#ffed4e"/>
                <circle cx="56.5" cy="64.5" r="0.8" fill="#fff" opacity="0.8"/>
            </svg>
            
            <div class="app-title">Simba</div>
        </div>
        <div class="loading-text">&#127830; Waking up...</div>
    </div>

    <div id="main-content">
        <div class='card'>
            <h3><span class="icon">&#128202;</span>Status Dashboard</h3>
            <div class="status-display">
                <p style="margin: 5px 0; color: #d4a574; font-size: 14px;">Current Activity:</p>
                <p style="margin: 0;"><span class="mode-name">)html";
    
    // Inject Mode Number/Name
    String modeName = "Unknown";
    switch(currentMode) {
        case 0: modeName = "&#128064; Watching"; break;
        case 1: modeName = "&#9200; Time Check"; break;
        case 2: modeName = "&#127919; Focus Time"; break;
        case 3: modeName = "&#9925; Weather"; break;
        case 4: modeName = "&#10024; Animation"; break;
        case 5: modeName = "&#128276; Reminders"; break;
    }
    ptr += modeName;
    
    ptr += R"html(</span></p>
            </div>
        </div>

        <div class='card'>
            <h3><span class="icon">&#128150;</span>Play & Interact</h3>
            <p style="color: #d4a574; font-size: 14px; margin-bottom: 15px;">Give Simba some love!</p>
            <button class="btn-pet" onclick="send('/pet')">
                <span class="emoji">&#128062;</span> Pet Simba <span class="emoji">&#128062;</span>
            </button>
        </div>

        <div class='card'>
            <h3><span class="icon">&#127925;</span>Train Simba</h3>
            <p style="color: #d4a574; font-size: 14px; margin-bottom: 15px;">Teach Simba new expressions &mdash; pick an eye animation to display on the OLED!</p>
            <button class="btn-mode" onclick="window.location.href='/train'" style="border-color: #d4a574;">
                &#128064; Open Eye Animations &rarr;
            </button>
        </div>

        <div class='card'>
            <h3><span class="icon">&#127918;</span>Activity Selection</h3>
            <p style="color: #d4a574; font-size: 14px; margin-bottom: 15px;">Choose what Simba should do:</p>
            <div class="mode-grid">
                <button class='btn-mode' onclick="send('/mode?id=0')">&#128064; Eyes</button>
                <button class='btn-mode' onclick="send('/mode?id=1')">&#9200; Clock</button>
                <button class='btn-mode' onclick="send('/mode?id=3')">&#9925; Weather</button>
                <button class='btn-mode' onclick="send('/mode?id=4')">&#10024; Animate</button>
                <button class='btn-mode' onclick="send('/mode?id=5')">&#128276; Reminders</button>
            </div>
        </div>

        <div class='card'>
            <h3><span class="icon">&#128276;</span>Reminders</h3>
            <p style="color: #d4a574; font-size: 14px; margin-bottom: 15px;">Save work or event notes for Simba to flash on the OLED.</p>
            <button class="btn-mode" onclick="window.location.href='/reminders'" style="border-color: #d4a574;">
                &#9997; Set Reminder List &rarr;
            </button>
            <button class="btn-start" onclick="send('/showReminders')">&#128250; SHOW LIST ON OLED</button>
        </div>

        <div class='card'>
            <h3><span class="icon">&#9201;</span>Focus Timer</h3>
            <p style="color: #d4a574; font-size: 14px; margin-bottom: 15px;">Set a timer and stay focused with Simba!</p>
            <div class="pomo-controls">
                <input id="pomoMin" type='number' value='25' min='1' max='120' class='pomo-input'>
                <span class="pomo-label">minutes</span>
            </div>
            <button class="btn-start" onclick="startPomo()">&#127919; START TIMER</button>
            <button class="btn-stop" onclick="send('/stopPomo')">&#9209; STOP TIMER</button>
        </div>

        <div class='card'>
            <h3><span class="icon">&#128225;</span>WiFi Setup</h3>
            <p style="color: #d4a574; font-size: 14px; margin-bottom: 15px;">Connect Simba to the internet:</p>
            <form action='/save' method='POST'>
                <input type='text' name='ssid' placeholder='&#127968; WiFi Network Name' required>
                <input type='password' name='pass' placeholder='&#128274; WiFi Password' required>
                <button class="btn-save" type='submit'>&#128190; SAVE & CONNECT</button>
            </form>
        </div>
        
        <p class="footer">
            Who's a good buddy? Simba is! 
            <span class="footer-emoji">&#129454;</span>
        </p>
    </div>

    <script>
        // Boot animation sequence
        window.addEventListener('load', () => {
            setTimeout(() => {
                document.body.classList.add('booted');
            }, 3000);
        });

        function send(url) {
            fetch(url)
                .then(response => {
                    if (response.ok) {
                        showToast('\u2713 Command sent!');
                    }
                })
                .catch(e => {
                    console.log('Error:', e);
                    showToast('\u2717 Connection error');
                });
        }

        function startPomo() {
            let min = document.getElementById('pomoMin').value;
            if (min < 1 || min > 120) {
                showToast('\u26A0 Please enter 1-120 minutes');
                return;
            }
            send('/startPomo?min=' + min);
            showToast(' Timer started: ' + min + ' min');
        }
        
        // Simple toast notification
        function showToast(message) {
            const toast = document.createElement('div');
            toast.textContent = message;
            toast.style.cssText = `
                position: fixed;
                bottom: 30px;
                left: 50%;
                transform: translateX(-50%);
                background: linear-gradient(135deg, #f5deb3, #d4a574);
                color: #2c1810;
                padding: 12px 24px;
                border-radius: 25px;
                font-weight: bold;
                box-shadow: 0 4px 15px rgba(0,0,0,0.3);
                z-index: 1000;
                animation: slideUp 0.3s ease-out;
            `;
            document.body.appendChild(toast);
            setTimeout(() => {
                toast.style.animation = 'slideDown 0.3s ease-out';
                setTimeout(() => toast.remove(), 300);
            }, 2000);
        }
        
        // Add CSS animations for toast
        const style = document.createElement('style');
        style.textContent = `
            @keyframes slideUp {
                from { transform: translate(-50%, 100px); opacity: 0; }
                to { transform: translate(-50%, 0); opacity: 1; }
            }
            @keyframes slideDown {
                from { transform: translate(-50%, 0); opacity: 1; }
                to { transform: translate(-50%, 100px); opacity: 0; }
            }
        `;
        document.head.appendChild(style);
    </script>

</body>
</html>
)html";
    return ptr;
}
