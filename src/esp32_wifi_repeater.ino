#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <Update.h>
#include <ESPmDNS.h>

const char* ap_ssid = "GMpro87dev-EXT";
const char* ap_password = "12345678";
const char* device_name = "Android-GMpro87dev";
const char* version = "1.0.0";

String sta_ssid = "";
String sta_pass = "";
String op_mode = "repeater";

bool ap_active = false;
bool sta_connected = false;
IPAddress local_ip;

Preferences prefs;
WebServer server(80);
DNSServer dnsServer;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== GMpro87dev WiFi ===");
  
  prefs.begin("wifi", false);
  sta_ssid = prefs.getString("sta_ssid", "");
  sta_pass = prefs.getString("sta_password", "");
  op_mode = prefs.getString("mode", "repeater");
  prefs.end();
  
  WiFi.setHostname(device_name);
  
  if(op_mode == "ap" || op_mode == "repeater" || op_mode == "wisp") {
    WiFi.softAP(ap_ssid, ap_password);
    ap_active = true;
    Serial.println("AP: " + String(ap_ssid));
  }
  
  if((op_mode == "repeater" || op_mode == "wisp") && sta_ssid != "") {
    WiFi.begin(sta_ssid.c_str(), sta_pass.c_str());
    Serial.println("Connect to: " + sta_ssid);
  }
  
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/setmode", HTTP_POST, handleSetMode);
  server.on("/connect", HTTP_POST, handleConnect);
  server.on("/reboot", HTTP_POST, handleReboot);
  server.on("/reset", HTTP_POST, handleReset);
  server.onNotFound(handleNotFound);
  
  server.begin();
  dnsServer.start(53, "*", WiFi.softAPIP());
  Serial.println("Web: http://" + WiFi.softAPIP().toString());
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  
  if(WiFi.status() == WL_CONNECTED) {
    if(!sta_connected) {
      sta_connected = true;
      local_ip = WiFi.localIP();
      Serial.println("STA Connected: " + local_ip.toString());
    }
  } else {
    sta_connected = false;
  }
  delay(100);
}

void handleRoot() {
  String html = R"rawl(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>GMpro87dev WiFi</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial;background:#0d1117;color:#e6edf3}
.header{text-align:center;padding:20px;background:linear-gradient(135deg,#1a1f2e,#161b22);border-radius:12px;margin:10px}
.header h1{color:#00f5ff;font-size:24px;margin-bottom:5px}
.tab-bar{display:flex;gap:5px;padding:10px;background:#161b22;margin:10px;border-radius:12px}
.tab{padding:10px 15px;background:#21262d;color:#8b949e;border:none;border-radius:8px;cursor:pointer}
.tab.active{background:#00f5ff;color:#000}
.content{display:none;padding:15px;background:#161b22;margin:10px;border-radius:12px}
.content.active{display:block}
select,input{padding:10px;width:100%;margin:5px 0;background:#0d1117;color:#e6edf3;border:1px solid #30363d;border-radius:8px}
.btn{padding:10px 20px;background:#00f5ff;color:#000;border:none;border-radius:8px;cursor:pointer;margin:5px 0}
.status{display:grid;grid-template-columns:repeat(2,1fr);gap:10px;padding:15px;background:#161b22;margin:10px;border-radius:12px}
.status-item{text-align:center}
.status-item .val{font-size:18px;font-weight:bold;color:#00f5ff}
</style>
</head>
<body>
<div class="header">
<h1>GMpro87dev WiFi</h1>
<div style="color:#8b949e;font-size:12px">v)rawl" + String(version) + R"rawl( • ESP32</div>
</div>

<div class="tab-bar">
<button class="tab active" onclick="showTab('dash')">Dashboard</button>
<button class="tab" onclick="showTab('conn')">Connect</button>
<button class="tab" onclick="showTab('set')">Settings</button>
</div>

<div id="dash" class="content active">
<div class="status">
<div class="status-item"><div class="label">Mode</div><div class="val" id="mVal">)rawl" + String(op_mode) + R"rawl(</div></div>
<div class="status-item"><div class="label">STA</div><div class="val" id="sVal">)rawl" + String(sta_connected ? "OK" : "Off") + R"rawl(</div></div>
<div class="status-item"><div class="label">AP</div><div class="val" id="aVal">)rawl" + String(ap_active ? "ON" : "Off") + R"rawl(</div></div>
<div class="status-item"><div class="label">IP</div><div class="val" id="iVal">)rawl" + (sta_connected ? local_ip.toString() : WiFi.softAPIP().toString()) + R"rawl(</div></div>
</div>
<button class="btn" onclick="reboot()">Reboot</button>
<button class="btn" style="background:#f85149" onclick="resetCfg()">Reset</button>
</div>

<div id="conn" class="content">
<h3>Connect to WiFi</h3>
<select id="wifiList"></select>
<input id="wifiPass" type="password" placeholder="Password">
<button class="btn" onclick="doConnect()">Connect</button>
<div id="msg"></div>
</div>

<div id="set" class="content">
<h3>Settings</h3>
<label>Mode:</label>
<select id="modeSel">
<option value="repeater">Repeater</option>
<option value="wisp">WISP</option>
<option value="ap">AP Only</option>
</select>
<label>AP SSID:</label>
<input id="apSsid" value=")rawl" + String(ap_ssid) + R"rawl(">
<label>AP Password:</label>
<input id="apPass" value=")rawl" + String(ap_password) + R"rawl(">
<button class="btn" onclick="saveSet()">Save</button>
</div>

<script>
function showTab(id) {
  document.querySelectorAll('.content').forEach(e => e.classList.remove('active'));
  document.querySelectorAll('.tab').forEach(e => e.classList.remove('active'));
  document.getElementById(id).classList.add('active');
  event.target.classList.add('active');
  if(id == 'conn') scanWiFi();
}

function scanWiFi() {
  fetch('/scan').then(r=>r.json()).then(d=>{
    let s = '<option>Select WiFi...</option>';
    d.forEach(w => s += '<option>' + w.ssid + '</option>');
    document.getElementById('wifiList').innerHTML = s;
  });
}

function doConnect() {
  const ssid = document.getElementById('wifiList').value;
  const pass = document.getElementById('wifiPass').value;
  fetch('/connect', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:'ssid='+ssid+'&pass='+pass})
    .then(r=>r.text()).then(t=>{document.getElementById('msg').innerText = t; setTimeout(()=>location.reload(),2000);});
}

function saveSet() {
  const mode = document.getElementById('modeSel').value;
  fetch('/setmode', {method:'POST', body:'mode='+mode});
  setTimeout(()=>location.reload(),1000);
}

function reboot() { if(confirm('Reboot?')) fetch('/reboot',{method:'POST'}); }
function resetCfg() { if(confirm('Reset all?')) fetch('/reset',{method:'POST'}); }

showTab('dash');
</script>
</body>
</html>
)rawl";
  server.send(200, "text/html", html);
}

void handleStatus() {
  String json = "{\"mode\":\"" + op_mode + "\",\"sta\":" + (sta_connected?"true":"false") + ",\"ap\":" + (ap_active?"true":"false") + ",\"ip\":\"" + (sta_connected?local_ip.toString():WiFi.softAPIP().toString()) + "\"}";
  server.send(200, "application/json", json);
}

void handleSetMode() {
  if(server.hasArg("mode")) {
    op_mode = server.arg("mode");
    prefs.begin("wifi", false);
    prefs.putString("mode", op_mode);
    prefs.end();
    server.send(200, "text/plain", "OK");
  }
}

void handleConnect() {
  if(server.hasArg("ssid") && server.hasArg("pass")) {
    sta_ssid = server.arg("ssid");
    sta_pass = server.arg("pass");
    prefs.begin("wifi", false);
    prefs.putString("sta_ssid", sta_ssid);
    prefs.putString("sta_password", sta_pass);
    prefs.end();
    WiFi.begin(sta_ssid.c_str(), sta_pass.c_str());
    server.send(200, "text/plain", "Connecting...");
  }
}

void handleReboot() {
  server.send(200, "text/plain", "Rebooting...");
  delay(500);
  ESP.restart();
}

void handleReset() {
  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();
  server.send(200, "text/plain", "Reset!");
  delay(500);
  ESP.restart();
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}
