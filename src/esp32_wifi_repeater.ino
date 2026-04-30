#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <Update.h>
#include <ESPmDNS.h>

void startAP();

#define MAX_CLIENTS 8

const char *hostname = "esp32-repeater";
const char *version = "1.0.0";

String sta_ssid = "";
String sta_password = "";
String ap_ssid = "GMpro87dev-EXT";
String ap_password = "12345678";
String device_name = "Android-GMpro87dev";
String op_mode = "repeater";

bool ap_active = false;
bool sta_connected = false;
IPAddress local_ip;
IPAddress gateway_ip;
IPAddress subnet_mask;

Preferences prefs;
WebServer webServer(80);
DNSServer dnsServer;

unsigned long last_sta_check = 0;
unsigned long reconnect_time = 0;
int reconnect_delay = 8000;

struct client_info {
  String mac;
  String ip;
  unsigned long connect_time;
};

client_info clients[MAX_CLIENTS];
int client_count = 0;

const char INDEX_HTML[] PROGMEM = R"rawl(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>GMpro87dev WiFi Repeater</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
:root { --primary: #00f5ff; --primary-dark: #00c8d4; --bg-dark: #0d1117; --bg-card: #161b22; --border: #30363d; --text: #e6edf3; --text-muted: #8b949e; --success: #3fb950; --danger: #f85149; --warning: #d29922; }
body { font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif; background: var(--bg-dark); min-height: 100vh; color: var(--text); line-height: 1.6; }
.container { max-width: 800px; margin: 0 auto; padding: 20px; }
.header { text-align: center; padding: 30px 20px; background: linear-gradient(135deg, #1a1f2e 0%, #161b22 100%); border-radius: 16px; margin-bottom: 20px; border: 1px solid var(--border); }
.header h1 { font-size: 32px; font-weight: 700; margin-bottom: 8px; background: linear-gradient(90deg, #00f5ff, #00c8d4); -webkit-background-clip: text; -webkit-text-fill-color: transparent; background-clip: text; letter-spacing: -0.5px; }
.header .version { color: var(--text-muted); font-size: 13px; font-weight: 500; }
.header .brand-tag { display: inline-block; background: rgba(0,245,255,0.1); border: 1px solid rgba(0,245,255,0.3); padding: 4px 12px; border-radius: 20px; font-size: 11px; color: var(--primary); margin-top: 10px; letter-spacing: 1px; text-transform: uppercase; }
.status-bar { display: grid; grid-template-columns: repeat(4, 1fr); gap: 12px; padding: 20px; background: var(--bg-card); border-radius: 16px; margin-bottom: 20px; border: 1px solid var(--border); }
.status-item { text-align: center; padding: 15px; background: rgba(255,255,255,0.02); border-radius: 12px; transition: all 0.3s; }
.status-item:hover { background: rgba(255,255,255,0.05); }
.status-item .label { font-size: 11px; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.5px; margin-bottom: 8px; }
.status-item .value { font-size: 18px; font-weight: 700; color: var(--primary); }
.status-item .value.connected { color: var(--success); }
.status-item .value.disconnected { color: var(--danger); }
.card { background: var(--bg-card); border-radius: 16px; padding: 24px; margin-bottom: 20px; border: 1px solid var(--border); }
.card h2 { font-size: 16px; font-weight: 600; margin-bottom: 20px; padding-bottom: 12px; border-bottom: 1px solid var(--border); color: var(--text); }
.form-group { margin-bottom: 18px; }
.form-group label { display: block; margin-bottom: 8px; color: var(--text-muted); font-size: 13px; font-weight: 500; }
.form-group input, .form-group select { width: 100%; padding: 14px 16px; border: 1px solid var(--border); border-radius: 10px; background: var(--bg-dark); color: var(--text); font-size: 14px; transition: all 0.2s; }
.form-group input:focus, .form-group select:focus { outline: none; border-color: var(--primary); box-shadow: 0 0 0 3px rgba(0,245,255,0.1); }
.form-group input::placeholder { color: var(--text-muted); }
.btn { padding: 14px 28px; border: none; border-radius: 10px; font-size: 14px; font-weight: 600; cursor: pointer; transition: all 0.3s; display: inline-flex; align-items: center; justify-content: center; gap: 8px; }
.btn-primary { background: linear-gradient(135deg, var(--primary), var(--primary-dark)); color: #000; }
.btn-primary:hover { transform: translateY(-2px); box-shadow: 0 8px 24px rgba(0,245,255,0.3); }
.btn-danger { background: linear-gradient(135deg, var(--danger), #d63a3a); color: #fff; }
.btn-warning { background: linear-gradient(135deg, var(--warning), #b3841a); color: #000; }
.btn-group { display: flex; gap: 12px; flex-wrap: wrap; }
.btn-group .btn { flex: 1; min-width: 120px; }
.network-list { max-height: 350px; overflow-y: auto; margin-top: 16px; }
.network-list::-webkit-scrollbar { width: 6px; }
.network-list::-webkit-scrollbar-track { background: var(--bg-dark); border-radius: 3px; }
.network-list::-webkit-scrollbar-thumb { background: var(--border); border-radius: 3px; }
.network-item { padding: 16px; background: rgba(255,255,255,0.03); border-radius: 12px; margin-bottom: 10px; cursor: pointer; transition: all 0.3s; border: 1px solid transparent; }
.network-item:hover { background: rgba(0,245,255,0.08); border-color: var(--primary); transform: translateX(4px); }
.network-item .ssid { font-weight: 600; font-size: 15px; margin-bottom: 6px; }
.network-item .info { display: flex; justify-content: space-between; font-size: 12px; color: var(--text-muted); }
.signal { color: var(--success); }
.locked { color: var(--warning); }
.client-list { margin-top: 12px; }
.client-item { padding: 14px; background: rgba(255,255,255,0.03); border-radius: 10px; margin-bottom: 8px; display: flex; justify-content: space-between; align-items: center; }
.client-item .mac { color: var(--text-muted); font-size: 12px; }
.client-item .ip { color: var(--primary); font-weight: 600; }
.tabs { display: flex; margin-bottom: 20px; background: var(--bg-card); border-radius: 12px; padding: 4px; border: 1px solid var(--border); }
.tab { flex: 1; padding: 14px 20px; background: transparent; border: none; cursor: pointer; color: var(--text-muted); font-size: 14px; font-weight: 600; border-radius: 10px; transition: all 0.3s; }
.tab.active { background: var(--primary); color: #000; }
.tab:not(.active):hover { background: rgba(255,255,255,0.05); color: var(--text); }
.tab-content { display: none; }
.tab-content.active { display: block; }
.loading { text-align: center; padding: 40px; }
.spinner { width: 40px; height: 40px; border: 3px solid var(--border); border-top-color: var(--primary); border-radius: 50%; animation: spin 1s linear infinite; margin: 0 auto; }
@keyframes spin { to { transform: rotate(360deg); } }
.footer { text-align: center; padding: 30px; color: var(--text-muted); font-size: 12px; }
.footer a { color: var(--primary); text-decoration: none; }
@media (max-width: 600px) {
  .status-bar { grid-template-columns: repeat(2, 1fr); }
  .btn-group { flex-direction: column; }
  .tabs { flex-direction: column; }
}
</style>
</head>
<body>
<div class="container">
<div class="header">
<h1>GMpro87dev</h1>
<div class="version">WiFi Repeater <span id="fwVersion">v1.0.0</span></div>
<div class="brand-tag">Powered by ESP32</div>
</div>

<div class="status-bar">
<div class="status-item">
<div class="label">Mode</div>
<div class="value" id="statusMode">Repeater</div>
</div>
<div class="status-item">
<div class="label">Client Status</div>
<div class="value" id="statusSta">Disconnecting</div>
</div>
<div class="status-item">
<div class="label">AP Status</div>
<div class="value" id="statusAp">Active</div>
</div>
<div class="status-item">
<div class="label">Clients</div>
<div class="value" id="statusClients">0</div>
</div>
</div>

<div class="tabs">
<button class="tab active" onclick="showTab('dashboard', this)">Dashboard</button>
<button class="tab" onclick="showTab('connect', this)">Connect</button>
<button class="tab" onclick="showTab('clients', this)">Clients</button>
<button class="tab" onclick="showTab('settings', this)">Settings</button>
</div>

<div id="dashboard" class="tab-content active">
<div class="card">
<h2>⚡ System Status</h2>
<div id="statusInfo">
<div class="form-group">
<label>Device Name:</label>
<div id="currDeviceName">-</div>
</div>
<div class="form-group">
<label>Upstream SSID:</label>
<div id="currStaSsid">-</div>
</div>
<div class="form-group">
<label>Client IP:</label>
<div id="currStaIp">-</div>
</div>
<div class="form-group">
<label>Local IP:</label>
<div id="currApIp">-</div>
</div>
</div>
</div>
</div>

<div id="connect" class="tab-content">
<div class="card">
<h2>📶 Scan Networks</h2>
<button class="btn btn-primary" onclick="scanNetworks()">Scan WiFi</button>
<div class="loading" id="scanLoading" style="display:none">
<div class="spinner"></div>
<p>Scanning...</p>
</div>
<div class="network-list" id="networkList"></div>
</div>

<div class="card">
<h2>🔗 Connect to Network</h2>
<form id="connectForm">
<div class="form-group">
<label>Select Network:</label>
<select id="ssidSelect" onchange="updateSsid()">
<option value="">-- Select Network --</option>
</select>
</div>
<div class="form-group">
<label>SSID:</label>
<input type="text" id="ssidInput" placeholder="Network SSID">
</div>
<div class="form-group">
<label>Password:</label>
<input type="password" id="passwordInput" placeholder="Network Password">
</div>
<div class="form-group">
<label>Your Device Name (for router display):</label>
<input type="text" id="deviceNameInput" placeholder="e.g., Xiaomi-Redmi-Note10">
</div>
<div class="btn-group">
<button type="button" class="btn btn-primary" onclick="connectToNetwork()">Connect</button>
</div>
</form>
</div>
</div>

<div id="clients" class="tab-content">
<div class="card">
<h2>👥 Connected Clients</h2>
<div class="client-list" id="clientList">
<p style="color:#666">No clients connected</p>
</div>
</div>
</div>

<div id="settings" class="tab-content">
<div class="card">
<h2>⚙️ AP Settings</h2>
<form id="apForm">
<div class="form-group">
<label>AP SSID (Your Network Name):</label>
<input type="text" id="apSsidInput" placeholder="ESP32-Repeater">
</div>
<div class="form-group">
<label>AP Password (min 8 chars):</label>
<input type="password" id="apPasswordInput" placeholder="12345678">
</div>
<div class="form-group">
<label>Device Name:</label>
<input type="text" id="deviceNameInput2" placeholder="ESP32-Repeater">
</div>
<div class="btn-group">
<button type="button" class="btn btn-primary" onclick="saveApSettings()">Save AP Settings</button>
</div>
</form>
</div>

<div class="card">
<h2>🔄 Mode Selection</h2>
<div class="form-group">
<label>Operation Mode:</label>
<select id="modeSelect" onchange="setMode()">
<option value="repeater">WiFi Repeater</option>
<option value="wisp">WISP (Client + AP)</option>
<option value="ap">AP Only</option>
</select>
</div>
</div>

<div class="card">
<h2>🔧 System</h2>
<div class="btn-group">
<button class="btn btn-warning" onclick="reboot()">Reboot</button>
<button class="btn btn-danger" onclick="resetConfig()">Factory Reset</button>
</div>
</div>
</div>

<div class="footer">
GMpro87dev WiFi Repeater • <a href="#">v1.0.0</a> • ESP32
</div>
</div>

<script>
var currentStatus = {};

async function getStatus() {
  try {
    const res = await fetch('/status');
    currentStatus = await res.json();
    updateStatus();
  } catch(e) {
    console.error(e);
  }
}

function updateStatus() {
  document.getElementById('deviceName').textContent = currentStatus.device_name || 'ESP32-Repeater';
  document.getElementById('currDeviceName').textContent = currentStatus.device_name || '-';
  document.getElementById('opMode').textContent = currentStatus.mode || 'repeater';
  document.getElementById('statusMode').textContent = currentStatus.mode || 'repeater';
  document.getElementById('currStaSsid').textContent = currentStatus.sta_ssid || '-';
  document.getElementById('currStaIp').textContent = currentStatus.sta_ip || '-';
  document.getElementById('currApIp').textContent = currentStatus.ap_ip || '-';
  
  document.getElementById('statusSta').textContent = currentStatus.sta_connected ? 'Connected' : 'Disconnected';
  document.getElementById('statusSta').className = 'value ' + (currentStatus.sta_connected ? 'connected' : 'disconnected');
  document.getElementById('statusAp').textContent = currentStatus.ap_enabled ? 'Active' : 'Disabled';
  document.getElementById('statusAp').className = 'value ' + (currentStatus.ap_enabled ? 'connected' : 'disconnected');
  document.getElementById('statusClients').textContent = currentStatus.stations ? currentStatus.stations.length : '0';
  
  if (currentStatus.stations && currentStatus.stations.length > 0) {
    let html = '';
    currentStatus.stations.forEach(sta => {
      html += `<div class="client-item"><div class="ip">${sta.ip}</div><div class="mac">${sta.mac}</div></div>`;
    });
    document.getElementById('clientList').innerHTML = html;
  }
  
  document.getElementById('apSsidInput').value = currentStatus.ap_ssid || '';
  document.getElementById('apPasswordInput').value = '';
  document.getElementById('deviceNameInput2').value = currentStatus.device_name || '';
  document.getElementById('modeSelect').value = currentStatus.mode || 'repeater';
}

async function scanNetworks() {
  document.getElementById('scanLoading').style.display = 'block';
  document.getElementById('networkList').innerHTML = '';
  
  try {
    const res = await fetch('/scan');
    const networks = await res.json();
    
    let html = '';
    networks.forEach(net => {
      html += `<div class="network-item" onclick="selectNetwork('${net.ssid}')">
        <div class="ssid">${net.ssid}</div>
        <div class="info">
          <span class="signal">Signal: ${net.rssi} dBm</span>
          <span class="${net.secure ? 'locked' : 'open'}">${net.secure ? '🔒 Secured' : '🔓 Open'}</span>
        </div>
      </div>`;
    });
    document.getElementById('networkList').innerHTML = html || '<p>No networks found</p>';
    
    let selectHtml = '<option value="">-- Select Network --</option>';
    networks.forEach(net => {
      selectHtml += `<option value="${net.ssid}">${net.ssid} (${net.rssi} dBm)</option>`;
    });
    document.getElementById('ssidSelect').innerHTML = selectHtml;
  } catch(e) {
    console.error(e);
  }
  
  document.getElementById('scanLoading').style.display = 'none';
}

function selectNetwork(ssid) {
  document.getElementById('ssidInput').value = ssid;
  document.getElementById('ssidSelect').value = ssid;
}

function updateSsid() {
  document.getElementById('ssidInput').value = document.getElementById('ssidSelect').value;
}

async function connectToNetwork() {
  const ssid = document.getElementById('ssidInput').value;
  const password = document.getElementById('passwordInput').value;
  const devName = document.getElementById('deviceNameInput').value;
  
  if (!ssid) {
    alert('Please select or enter a network SSID');
    return;
  }
  
  try {
    const res = await fetch('/connect', {
      method: 'POST',
      headers: {'Content-Type': 'application/x-www-form-urlencoded'},
      body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}&device_name=${encodeURIComponent(devName)}`
    });
    alert('Connecting... Reboot required');
  } catch(e) {
    console.error(e);
  }
}

async function saveApSettings() {
  const apSsid = document.getElementById('apSsidInput').value;
  const apPassword = document.getElementById('apPasswordInput').value;
  const devName = document.getElementById('deviceNameInput2').value;
  
  if (!apSsid) {
    alert('Please enter AP SSID');
    return;
  }
  
  try {
    const res = await fetch('/apsettings', {
      method: 'POST',
      headers: {'Content-Type': 'application/x-www-form-urlencoded'},
      body: `ssid=${encodeURIComponent(apSsid)}&password=${encodeURIComponent(apPassword)}&device_name=${encodeURIComponent(devName)}`
    });
    alert('Settings saved');
  } catch(e) {
    console.error(e);
  }
}

async function setMode() {
  const mode = document.getElementById('modeSelect').value;
  try {
    await fetch('/setmode', {
      method: 'POST',
      headers: {'Content-Type': 'application/x-www-form-urlencoded'},
      body: `mode=${mode}`
    });
    alert('Mode changed - please reboot');
  } catch(e) {
    console.error(e);
  }
}

async function reboot() {
  if (confirm('Reboot device?')) {
    await fetch('/reboot', {method: 'POST'});
  }
}

async function resetConfig() {
  if (confirm('Factory reset? All settings will be lost!')) {
    await fetch('/reset', {method: 'POST'});
  }
}

function showTab(tabId, btn) {
  document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));
  document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
  document.getElementById(tabId).classList.add('active');
  if(btn) btn.classList.add('active');
}

setInterval(getStatus, 5000);
getStatus();
</script>
</body>
</html>
)rawl";

void handleRoot() {
  webServer.send(200, "text/html", INDEX_HTML);
}

void handleScan() {
  String json = "[";
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
    json += "\"secure\":" + String((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? 0 : 1) + ",";
    json += "\"channel\":" + String(WiFi.channel(i)) + "}";
  }
  json += "]";
  webServer.send(200, "application/json", json);
}

void handleStatus() {
  String json = "{";
  json += "\"mode\":\"" + op_mode + "\",";
  json += "\"ap_enabled\":" + String(ap_active) + ",";
  json += "\"sta_connected\":" + String(sta_connected) + ",";
  json += "\"device_name\":\"" + device_name + "\",";
  json += "\"ap_ssid\":\"" + ap_ssid + "\",";
  json += "\"sta_ssid\":\"" + sta_ssid + "\",";
  json += "\"sta_ip\":\"" + (sta_connected ? local_ip.toString() : "0.0.0.0") + "\",";
  json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\",";
  json += "\"stations\":[";
  
  for (int i = 0; i < client_count; i++) {
    if (i > 0) json += ",";
    json += "{\"ip\":\"" + clients[i].ip + "\",";
    json += "\"mac\":\"" + clients[i].mac + "\"}";
  }
  json += "]}";
  
  webServer.send(200, "application/json", json);
}

void handleConnect() {
  if (webServer.hasArg("ssid") && webServer.hasArg("password")) {
    sta_ssid = webServer.arg("ssid");
    sta_password = webServer.arg("password");
    
    if (webServer.hasArg("device_name") && webServer.arg("device_name").length() > 0) {
      device_name = webServer.arg("device_name");
    } else {
      device_name = sta_ssid + "_EXT";
    }
    
    if (webServer.hasArg("ap_ssid") && webServer.arg("ap_ssid").length() > 0) {
      ap_ssid = webServer.arg("ap_ssid");
    } else {
      ap_ssid = device_name;
    }
    
    if (webServer.hasArg("ap_password") && webServer.arg("ap_password").length() >= 8) {
      ap_password = webServer.arg("ap_password");
    } else if (webServer.hasArg("ap_password")) {
      ap_password = "12345678";
    }
    
    prefs.begin("wifi-repeater", false);
    prefs.putString("sta_ssid", sta_ssid);
    prefs.putString("sta_password", sta_password);
    prefs.putString("ap_ssid", ap_ssid);
    prefs.putString("ap_password", ap_password);
    prefs.putString("device_name", device_name);
    prefs.end();
    
    delay(100);
    ESP.restart();
  }
  webServer.send(200, "text/plain", "OK");
}

void handleApSettings() {
  if (webServer.hasArg("ssid")) {
    ap_ssid = webServer.arg("ssid");
    if (webServer.hasArg("password") && webServer.arg("password").length() >= 8) {
      ap_password = webServer.arg("password");
    }
    if (webServer.hasArg("device_name")) {
      device_name = webServer.arg("device_name");
    }
    
    prefs.begin("wifi-repeater", false);
    prefs.putString("ap_ssid", ap_ssid);
    prefs.putString("ap_password", ap_password);
    prefs.putString("device_name", device_name);
    prefs.end();
    
    WiFi.softAPdisconnect(true);
    delay(100);
    startAP();
  }
  webServer.send(200, "text/plain", "OK");
}

void handleSetMode() {
  if (webServer.hasArg("mode")) {
    op_mode = webServer.arg("mode");
    prefs.begin("wifi-repeater", false);
    prefs.putString("mode", op_mode);
    prefs.end();
    delay(100);
    ESP.restart();
  }
  webServer.send(200, "text/plain", "OK");
}

void handleReboot() {
  webServer.send(200, "text/plain", "Rebooting...");
  delay(500);
  ESP.restart();
}

void handleReset() {
  prefs.begin("wifi-repeater", false);
  prefs.clear();
  prefs.end();
  webServer.send(200, "text/plain", "Resetting...");
  delay(500);
  ESP.restart();
}

void handleNotFound() {
  String msg = "File Not Found\n\nURI: " + webServer.uri() + "\n";
  webServer.send(404, "text/plain", msg);
}

void startAP() {
  WiFi.softAPdisconnect(true);
  delay(100);
  
  WiFi.softAP(ap_ssid.c_str(), ap_password.length() >= 8 ? ap_password.c_str() : NULL);
  ap_active = true;
  
  Serial.print("AP started: ");
  Serial.println(ap_ssid);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

bool connectSTA() {
  if (sta_ssid.length() == 0) return false;
  
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(device_name.c_str());
  WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    sta_connected = true;
    local_ip = WiFi.localIP();
    gateway_ip = WiFi.gatewayIP();
    subnet_mask = WiFi.subnetMask();
    
    Serial.println("STA Connected!");
    Serial.print("IP: ");
    Serial.println(local_ip);
    
    return true;
  }
  
  sta_connected = false;
  return false;
}

void checkConnections() {
  if (op_mode == "repeater" || op_mode == "wisp") {
    if (WiFi.status() == WL_CONNECTED) {
      sta_connected = true;
      local_ip = WiFi.localIP();
    } else {
      if (millis() - reconnect_time > reconnect_delay) {
        reconnect_time = millis();
        Serial.println("Reconnecting...");
        connectSTA();
      }
    }
  }
  
  if (ap_active) {
    client_count = 0;
    int staCount = WiFi.softAPgetStationNum();
    for (int i = 0; i < staCount && i < MAX_CLIENTS; i++) {
      client_count++;
    }
  }
}

void initWiFi() {
  prefs.begin("wifi-repeater", true);
  sta_ssid = prefs.getString("sta_ssid", "");
  sta_password = prefs.getString("sta_password", "");
  ap_ssid = prefs.getString("ap_ssid", "GMpro87dev-EXT");
  ap_password = prefs.getString("ap_password", "12345678");
  device_name = prefs.getString("device_name", "Android-GMpro87dev");
  op_mode = prefs.getString("mode", "repeater");
  prefs.end();
  
  WiFi.setHostname(device_name.c_str());
  WiFi.mode(WIFI_AP_STA);
  
  bool sta_ok = false;
  if (op_mode == "repeater" || op_mode == "wisp") {
    if (sta_ssid.length() > 0) {
      sta_ok = connectSTA();
    }
  }
  
  if (op_mode == "repeater" || op_mode == "wisp" || op_mode == "ap") {
    startAP();
  }
  
  dnsServer.start(53, "*", WiFi.softAPIP());
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== ESP32 WiFi Repeater/WISP ===");
  
  initWiFi();
  
  webServer.on("/", handleRoot);
  webServer.on("/scan", handleScan);
  webServer.on("/status", handleStatus);
  webServer.on("/connect", HTTP_POST, handleConnect);
  webServer.on("/apsettings", HTTP_POST, handleApSettings);
  webServer.on("/setmode", HTTP_POST, handleSetMode);
  webServer.on("/reboot", HTTP_POST, handleReboot);
  webServer.on("/reset", HTTP_POST, handleReset);
  webServer.onNotFound(handleNotFound);
  
  webServer.begin();
  
  Serial.println("HTTP Server started");
  Serial.print("Connect to: http://");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
  checkConnections();
  delay(10);
}