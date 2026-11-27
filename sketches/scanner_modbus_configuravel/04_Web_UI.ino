extern ESP8266WebServer server;
extern SerialSettings serialCfg;

extern uint8_t slaveScanIdStart;
extern uint8_t slaveScanIdEnd;
extern uint8_t slaveScanCurrent;
extern uint8_t slaveScanFn;
extern uint16_t slaveScanAddr;

extern uint8_t regScanSlave;
extern uint8_t regScanFn;
extern uint16_t regScanStart;
extern uint16_t regScanEnd;
extern uint16_t regScanStep;
extern uint16_t regScanLen;
extern uint16_t regScanCurrent;

extern bool regScanning;
extern bool regScanFinished;
extern uint32_t okCount;
extern uint32_t failCount;

extern uint8_t readSlave;
extern uint8_t readFn;
extern uint16_t readAddr;
extern uint16_t readLen;
extern bool readFinished;
extern bool readSuccess;
extern uint8_t readLastRc;

extern ScanItem found[MAX_SCAN_ITEMS];
extern uint16_t foundCount;

extern SlaveHit slaveHits[MAX_SLAVE_HITS];
extern uint16_t slaveHitCount;

extern String logLines[MAX_LOG_LINES];
extern uint16_t logCount;
extern uint16_t logWriteIdx;

extern OpMode opMode;

String htmlEscape(const String& s);
String jsonEscape(const String& s);
float regsToFloat(uint16_t hi, uint16_t lo);
void addLog(const String& level, const String& msg);
void stopAllOperations();
void applySerialSettings();
void resetRegisterResults();
void resetSlaveResults();

static String buildLogsJson() {
  String json = "[";
  uint16_t start = (logCount < MAX_LOG_LINES) ? 0 : logWriteIdx;
  for (uint16_t i = 0; i < logCount; i++) {
    uint16_t idx = (start + i) % MAX_LOG_LINES;
    if (i) json += ",";
    json += "\"" + jsonEscape(logLines[idx]) + "\"";
  }
  json += "]";
  return json;
}

void handleRoot() {
  String page;
  page.reserve(12000);

  page += "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  page += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  page += "<title>Scanner Modbus Completo</title>";
  page += "<style>";
  page += "body{font-family:Arial,sans-serif;margin:20px;background:#f5f5f5;color:#222}";
  page += "h1{font-size:22px;margin-bottom:8px}";
  page += ".box{background:#fff;padding:14px;border-radius:10px;margin-bottom:14px;box-shadow:0 1px 4px rgba(0,0,0,.08)}";
  page += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:10px}";
  page += "label{display:block;font-size:13px;margin-bottom:4px}";
  page += "input,select{width:100%;padding:7px;margin:2px 0 6px 0}";
  page += "button{padding:8px 12px;margin:4px 4px 4px 0}";
  page += "table{border-collapse:collapse;width:100%;background:#fff}";
  page += "th,td{border:1px solid #ccc;padding:6px;text-align:left;font-size:14px}";
  page += "th{background:#eee;position:sticky;top:0}";
  page += ".mono{font-family:Consolas,monospace}";
  page += ".log{background:#111;color:#0f0;padding:10px;border-radius:8px;max-height:220px;overflow:auto;white-space:pre-wrap}";
  page += "</style></head><body>";

  page += "<h1>Scanner Modbus Completo</h1>";

  page += "<div class='box'>";
  page += "<div><b>Wi-Fi AP:</b> " + htmlEscape(String(AP_SSID)) + "</div>";
  page += "<div><b>IP:</b> " + WiFi.softAPIP().toString() + "</div>";
  page += "<div><b>Modo atual:</b> <span id='mode'>-</span></div>";
  page += "</div>";

  page += "<div class='box'>";
  page += "<h3>1) Configuração da Porta Modbus</h3>";
  page += "<div class='grid'>";
  page += "<div><label>Baud</label><select id='baud'>";
  const uint32_t baudOpts[] = {1200,2400,4800,9600,19200,38400,57600,115200};
  for (uint8_t i = 0; i < 8; i++) {
    page += "<option value='" + String(baudOpts[i]) + "'";
    if (serialCfg.baud == baudOpts[i]) page += " selected";
    page += ">" + String(baudOpts[i]) + "</option>";
  }
  page += "</select></div>";

  page += "<div><label>Paridade</label><select id='parity'>";
  page += "<option value='N'" + String(serialCfg.parity=='N' ? " selected" : "") + ">None</option>";
  page += "<option value='E'" + String(serialCfg.parity=='E' ? " selected" : "") + ">Even</option>";
  page += "<option value='O'" + String(serialCfg.parity=='O' ? " selected" : "") + ">Odd</option>";
  page += "</select></div>";

  page += "<div><label>Stop Bits</label><select id='stopBits'>";
  page += "<option value='1'" + String(serialCfg.stopBits==1 ? " selected" : "") + ">1</option>";
  page += "<option value='2'" + String(serialCfg.stopBits==2 ? " selected" : "") + ">2</option>";
  page += "</select></div>";
  page += "</div>";
  page += "<button onclick='applySerial()'>Aplicar porta RTU</button>";
  page += "</div>";

  page += "<div class='box'>";
  page += "<h3>2) Procurar Slaves</h3>";
  page += "<div class='grid'>";
  page += "<div><label>ID inicial</label><input id='slaveStart' type='number' min='1' max='247' value='1'></div>";
  page += "<div><label>ID final</label><input id='slaveEnd' type='number' min='1' max='247' value='247'></div>";
  page += "<div><label>Função teste</label><select id='slaveFn'>";
  page += "<option value='1'>01 - Coil</option>";
  page += "<option value='2'>02 - Input Status</option>";
  page += "<option value='3' selected>03 - Holding Register</option>";
  page += "<option value='4'>04 - Input Register</option>";
  page += "</select></div>";
  page += "<div><label>Endereço teste</label><input id='slaveAddr' type='number' min='0' max='65535' value='0'></div>";
  page += "</div>";
  page += "<button onclick='startSlaveScan()'>Iniciar varredura de slaves</button>";
  page += "</div>";

  page += "<div class='box'>";
  page += "<h3>3) Procurar Registradores</h3>";
  page += "<div class='grid'>";
  page += "<div><label>Slave ID</label><input id='regSlave' type='number' min='1' max='247' value='1'></div>";
  page += "<div><label>Função</label><select id='regFn'>";
  page += "<option value='1'>01 - Coil</option>";
  page += "<option value='2'>02 - Input Status</option>";
  page += "<option value='3' selected>03 - Holding Register</option>";
  page += "<option value='4'>04 - Input Register</option>";
  page += "</select></div>";
  page += "<div><label>Início</label><input id='regStart' type='number' value='0'></div>";
  page += "<div><label>Fim</label><input id='regEnd' type='number' value='1198'></div>";
  page += "<div><label>Passo</label><input id='regStep' type='number' value='2'></div>";
  page += "<div><label>Len</label><input id='regLen' type='number' value='2' min='1' max='4'></div>";
  page += "</div>";
  page += "<button onclick='startRegisterScan()'>Iniciar scan de registradores</button>";
  page += "</div>";

  page += "<div class='box'>";
  page += "<h3>4) Leitura Pontual</h3>";
  page += "<div class='grid'>";
  page += "<div><label>Slave ID</label><input id='readSlave' type='number' min='1' max='247' value='1'></div>";
  page += "<div><label>Função</label><select id='readFn'>";
  page += "<option value='1'>01 - Coil</option>";
  page += "<option value='2'>02 - Input Status</option>";
  page += "<option value='3' selected>03 - Holding Register</option>";
  page += "<option value='4'>04 - Input Register</option>";
  page += "</select></div>";
  page += "<div><label>Endereço</label><input id='readAddr' type='number' value='0'></div>";
  page += "<div><label>Len</label><input id='readLen' type='number' value='1' min='1' max='4'></div>";
  page += "</div>";
  page += "<button onclick='readOnce()'>Ler</button>";
  page += "<button onclick='stopAll()'>Parar</button>";
  page += "</div>";

  page += "<div class='box mono' id='status'>Carregando...</div>";

  page += "<div class='box'>";
  page += "<h3>Slaves encontrados</h3>";
  page += "<table><thead><tr><th>ID</th><th>Success</th><th>RC</th></tr></thead><tbody id='slaveRows'></tbody></table>";
  page += "</div>";

  page += "<div class='box'>";
  page += "<h3>Registradores encontrados / leituras</h3>";
  page += "<table>";
  page += "<thead><tr><th>Slave</th><th>Fn</th><th>Addr</th><th>Len</th><th>V0</th><th>V1</th><th>V2</th><th>V3</th><th>Float HI,LO</th><th>Float LO,HI</th></tr></thead>";
  page += "<tbody id='regRows'></tbody></table>";
  page += "</div>";

  page += "<div class='box'>";
  page += "<h3>Erros / Logs</h3>";
  page += "<div class='log mono' id='logs'></div>";
  page += "</div>";

  page += "<script>";
  page += "function fnName(v){return {'1':'01','2':'02','3':'03','4':'04'}[String(v)]||v;}";
  page += "async function getStatus(){";
  page += " const r = await fetch('/status');";
  page += " const j = await r.json();";
  page += " document.getElementById('mode').textContent = j.modeText;";
  page += " document.getElementById('status').innerHTML = "
          "`scanning=${j.regScanning} | finished=${j.regScanFinished} | atual=${j.regCurrent} | ok=${j.okCount} | fail=${j.failCount} | foundRegs=${j.foundCount} | foundSlaves=${j.slaveHitCount} | readFinished=${j.readFinished} | readSuccess=${j.readSuccess} | readRc=${j.readRc}`;";

  page += " let s='';";
  page += " for (const it of j.slaves){";
  page += "   s += `<tr><td>${it.id}</td><td>${it.success}</td><td>${it.rc}</td></tr>`;";
  page += " }";
  page += " document.getElementById('slaveRows').innerHTML = s;";

  page += " let html='';";
  page += " for (const it of j.items) {";
  page += "   html += `<tr>` +";
  page += "     `<td>${it.slave}</td>` +";
  page += "     `<td>${it.fn}</td>` +";
  page += "     `<td>${it.addr}</td>` +";
  page += "     `<td>${it.len}</td>` +";
  page += "     `<td>${it.v0}</td>` +";
  page += "     `<td>${it.v1}</td>` +";
  page += "     `<td>${it.v2}</td>` +";
  page += "     `<td>${it.v3}</td>` +";
  page += "     `<td>${it.f_hilo}</td>` +";
  page += "     `<td>${it.f_lohi}</td>` +";
  page += "     `</tr>`;";
  page += " }";
  page += " document.getElementById('regRows').innerHTML = html;";

  page += " document.getElementById('logs').textContent = j.logs.join('\\n');";
  page += "}";

  page += "async function applySerial(){";
  page += " const baud=document.getElementById('baud').value;";
  page += " const parity=document.getElementById('parity').value;";
  page += " const stopBits=document.getElementById('stopBits').value;";
  page += " await fetch(`/applySerial?baud=${baud}&parity=${parity}&stopBits=${stopBits}`);";
  page += " setTimeout(getStatus, 300);";
  page += "}";

  page += "async function startSlaveScan(){";
  page += " const idStart=document.getElementById('slaveStart').value;";
  page += " const idEnd=document.getElementById('slaveEnd').value;";
  page += " const fn=document.getElementById('slaveFn').value;";
  page += " const addr=document.getElementById('slaveAddr').value;";
  page += " await fetch(`/startSlaveScan?idStart=${idStart}&idEnd=${idEnd}&fn=${fn}&addr=${addr}`);";
  page += "}";

  page += "async function startRegisterScan(){";
  page += " const slave=document.getElementById('regSlave').value;";
  page += " const fn=document.getElementById('regFn').value;";
  page += " const start=document.getElementById('regStart').value;";
  page += " const end=document.getElementById('regEnd').value;";
  page += " const step=document.getElementById('regStep').value;";
  page += " const len=document.getElementById('regLen').value;";
  page += " await fetch(`/startRegisterScan?slave=${slave}&fn=${fn}&start=${start}&end=${end}&step=${step}&len=${len}`);";
  page += "}";

  page += "async function readOnce(){";
  page += " const slave=document.getElementById('readSlave').value;";
  page += " const fn=document.getElementById('readFn').value;";
  page += " const addr=document.getElementById('readAddr').value;";
  page += " const len=document.getElementById('readLen').value;";
  page += " await fetch(`/readOnce?slave=${slave}&fn=${fn}&addr=${addr}&len=${len}`);";
  page += "}";

  page += "async function stopAll(){ await fetch('/stop'); }";
  page += "setInterval(getStatus, 1000);";
  page += "getStatus();";
  page += "</script>";

  page += "</body></html>";

  server.send(200, "text/html; charset=utf-8", page);
}

void handleApplySerial() {
  if (server.hasArg("baud")) {
    serialCfg.baud = (uint32_t)server.arg("baud").toInt();
  }
  if (server.hasArg("parity")) {
    String p = server.arg("parity");
    p.toUpperCase();
    if (p == "N" || p == "E" || p == "O") serialCfg.parity = p[0];
  }
  if (server.hasArg("stopBits")) {
    uint8_t s = (uint8_t)server.arg("stopBits").toInt();
    serialCfg.stopBits = (s == 2) ? 2 : 1;
  }

  stopAllOperations();
  applySerialSettings();
  server.send(200, "text/plain", "OK");
}

void handleStartSlaveScan() {
  stopAllOperations();
  resetSlaveResults();

  if (server.hasArg("idStart")) slaveScanIdStart = (uint8_t)max(1L, server.arg("idStart").toInt());
  if (server.hasArg("idEnd"))   slaveScanIdEnd   = (uint8_t)min(247L, server.arg("idEnd").toInt());
  if (server.hasArg("fn"))      slaveScanFn      = (uint8_t)server.arg("fn").toInt();
  if (server.hasArg("addr"))    slaveScanAddr    = (uint16_t)server.arg("addr").toInt();

  if (slaveScanIdEnd < slaveScanIdStart) {
    uint8_t t = slaveScanIdStart;
    slaveScanIdStart = slaveScanIdEnd;
    slaveScanIdEnd = t;
  }

  slaveScanCurrent = slaveScanIdStart;
  opMode = OP_SLAVE_SCAN_SEND;
  addLog("INFO", "Iniciando scan de slaves.");
  server.send(200, "text/plain", "OK");
}

void handleStartRegisterScan() {
  stopAllOperations();
  resetRegisterResults();

  if (server.hasArg("slave")) regScanSlave = (uint8_t)max(1L, min(247L, server.arg("slave").toInt()));
  if (server.hasArg("fn"))    regScanFn    = (uint8_t)server.arg("fn").toInt();
  if (server.hasArg("start")) regScanStart = (uint16_t)server.arg("start").toInt();
  if (server.hasArg("end"))   regScanEnd   = (uint16_t)server.arg("end").toInt();
  if (server.hasArg("step"))  regScanStep  = (uint16_t)max(1L, server.arg("step").toInt());
  if (server.hasArg("len"))   regScanLen   = (uint16_t)max(1L, min((long)MAX_READ_LEN, server.arg("len").toInt()));

  regScanCurrent = regScanStart;
  regScanning = true;
  regScanFinished = false;
  okCount = 0;
  failCount = 0;

  opMode = OP_REG_SCAN_SEND;
  addLog("INFO", "Iniciando scan de registradores.");
  server.send(200, "text/plain", "OK");
}

void handleReadOnce() {
  stopAllOperations();

  if (server.hasArg("slave")) readSlave = (uint8_t)max(1L, min(247L, server.arg("slave").toInt()));
  if (server.hasArg("fn"))    readFn    = (uint8_t)server.arg("fn").toInt();
  if (server.hasArg("addr"))  readAddr  = (uint16_t)server.arg("addr").toInt();
  if (server.hasArg("len"))   readLen   = (uint16_t)max(1L, min((long)MAX_READ_LEN, server.arg("len").toInt()));

  readFinished = false;
  readSuccess = false;
  readLastRc = 0;

  opMode = OP_READ_ONCE_SEND;
  server.send(200, "text/plain", "OK");
}

void handleStop() {
  stopAllOperations();
  server.send(200, "text/plain", "STOPPED");
}

void handleStatus() {
  String json = "{";

  String modeText = "IDLE";
  switch (opMode) {
    case OP_SLAVE_SCAN_SEND: modeText = "SLAVE_SCAN_SEND"; break;
    case OP_SLAVE_SCAN_WAIT: modeText = "SLAVE_SCAN_WAIT"; break;
    case OP_REG_SCAN_SEND: modeText = "REG_SCAN_SEND"; break;
    case OP_REG_SCAN_WAIT: modeText = "REG_SCAN_WAIT"; break;
    case OP_READ_ONCE_SEND: modeText = "READ_ONCE_SEND"; break;
    case OP_READ_ONCE_WAIT: modeText = "READ_ONCE_WAIT"; break;
    default: break;
  }

  json += "\"modeText\":\"" + modeText + "\",";
  json += "\"regScanning\":" + String(regScanning ? "true" : "false") + ",";
  json += "\"regScanFinished\":" + String(regScanFinished ? "true" : "false") + ",";
  json += "\"regCurrent\":" + String(regScanCurrent) + ",";
  json += "\"okCount\":" + String(okCount) + ",";
  json += "\"failCount\":" + String(failCount) + ",";
  json += "\"foundCount\":" + String(foundCount) + ",";
  json += "\"slaveHitCount\":" + String(slaveHitCount) + ",";
  json += "\"readFinished\":" + String(readFinished ? "true" : "false") + ",";
  json += "\"readSuccess\":" + String(readSuccess ? "true" : "false") + ",";
  json += "\"readRc\":" + String(readLastRc) + ",";

  json += "\"slaves\":[";
  for (uint16_t i = 0; i < slaveHitCount; i++) {
    if (i) json += ",";
    json += "{";
    json += "\"id\":" + String(slaveHits[i].id) + ",";
    json += "\"success\":" + String(slaveHits[i].success ? "true" : "false") + ",";
    json += "\"rc\":" + String(slaveHits[i].rc);
    json += "}";
  }
  json += "],";

  json += "\"items\":[";
  uint16_t start = 0;
  if (foundCount > 120) start = foundCount - 120;
  for (uint16_t i = start; i < foundCount; i++) {
    if (i > start) json += ",";

    float f_hilo = regsToFloat(found[i].v[0], found[i].v[1]);
    float f_lohi = regsToFloat(found[i].v[1], found[i].v[0]);

    json += "{";
    json += "\"slave\":" + String(found[i].slave) + ",";
    json += "\"fn\":" + String(found[i].fn) + ",";
    json += "\"addr\":" + String(found[i].addr) + ",";
    json += "\"len\":" + String(found[i].len) + ",";
    json += "\"v0\":" + String(found[i].v[0]) + ",";
    json += "\"v1\":" + String(found[i].v[1]) + ",";
    json += "\"v2\":" + String(found[i].v[2]) + ",";
    json += "\"v3\":" + String(found[i].v[3]) + ",";
    json += "\"f_hilo\":\"" + String(f_hilo, 4) + "\",";
    json += "\"f_lohi\":\"" + String(f_lohi, 4) + "\"";
    json += "}";
  }
  json += "],";

  json += "\"logs\":" + buildLogsJson();
  json += "}";

  server.send(200, "application/json", json);
}
