#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ModbusRTU.h>

// ========================= PINOS =========================
#define LED_PIN           LED_BUILTIN
#define DE_RE             12
#define BTN_PIN           0
#define HALF_OR_FULL_PIN  13

// ========================= WIFI AP =========================
const char* AP_SSID = "MMW03-SCAN";
const char* AP_PASS = "12345678";

// ========================= MODBUS =========================
static const uint8_t  SLAVE_ID = 1;
static const uint32_t RTU_BAUD = 38400;    // ajuste aqui se quiser 38400
static const uint32_t RTU_TIMEOUT_MS = 300;

// ========================= SCAN =========================
// Faixa padrão: 0 a 1198, lendo de 2 em 2 words
// Se quiser testar 1-based depois, troque SCAN_START para 1
static uint16_t SCAN_START = 0;
static uint16_t SCAN_END   = 1198;
static uint16_t SCAN_STEP  = 2;
static uint16_t READ_LEN   = 2;

// ========================= OBJETOS =========================
ESP8266WebServer server(80);
ModbusRTU mbRTU;

// ========================= ESTADO RTU =========================
struct RtuReq {
  bool busy = false;
  bool ok = false;
  uint16_t addr = 0;
  uint16_t len = 0;
  uint16_t* buf = nullptr;
} rtu;

uint32_t rtuTxnStart = 0;
uint16_t rxBuf[4];

// ========================= RESULTADOS =========================
struct ScanItem {
  uint16_t addr;
  uint16_t w0;
  uint16_t w1;
};

static const uint16_t MAX_ITEMS = 700;
ScanItem found[MAX_ITEMS];
uint16_t foundCount = 0;

bool scanning = false;
bool scanFinished = false;
uint16_t currentAddr = 0;
uint32_t lastStartMs = 0;
uint32_t okCount = 0;
uint32_t failCount = 0;

// ========================= HELPERS =========================
static inline void rs485TxEnable(bool en) {
  digitalWrite(DE_RE, en ? HIGH : LOW);
}

bool rtuReadHreg(uint16_t addr, uint16_t* dst, uint16_t len) {
  if (rtu.busy) return false;

  rtu.busy = true;
  rtu.ok = false;
  rtu.addr = addr;
  rtu.len = len;
  rtu.buf = dst;
  rtuTxnStart = millis();

  auto cb = [](Modbus::ResultCode event, uint16_t, void*) -> bool {
    rtu.ok = (event == Modbus::EX_SUCCESS);
    rtu.busy = false;
    return true;
  };

  bool started = mbRTU.readHreg(SLAVE_ID, addr, dst, len, cb);
  if (!started) {
    rtu.busy = false;
    rtu.ok = false;
    return false;
  }
  return true;
}

void resetScanState() {
  scanning = false;
  scanFinished = false;
  currentAddr = SCAN_START;
  foundCount = 0;
  okCount = 0;
  failCount = 0;
}

void startScan() {
  resetScanState();
  scanning = true;
  currentAddr = SCAN_START;
}

void saveFound(uint16_t addr, uint16_t w0, uint16_t w1) {
  if (foundCount >= MAX_ITEMS) return;
  found[foundCount].addr = addr;
  found[foundCount].w0 = w0;
  found[foundCount].w1 = w1;
  foundCount++;
}

String htmlEscape(const String& s) {
  String out = s;
  out.replace("&", "&amp;");
  out.replace("<", "&lt;");
  out.replace(">", "&gt;");
  out.replace("\"", "&quot;");
  return out;
}

float regsToFloat(uint16_t hi, uint16_t lo) {
  union {
    uint32_t u32;
    float f;
  } cvt;
  cvt.u32 = ((uint32_t)hi << 16) | lo;
  return cvt.f;
}

// ========================= WEB =========================
void handleRoot() {
  String page;
  page.reserve(6000);

  page += "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  page += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  page += "<title>MMW03 Scanner</title>";
  page += "<style>";
  page += "body{font-family:Arial,sans-serif;margin:20px;background:#f5f5f5;color:#222}";
  page += "h1{font-size:22px}";
  page += ".box{background:#fff;padding:14px;border-radius:10px;margin-bottom:14px;box-shadow:0 1px 4px rgba(0,0,0,.08)}";
  page += "input{width:90px;padding:6px;margin:4px}";
  page += "button{padding:8px 12px;margin:4px}";
  page += "table{border-collapse:collapse;width:100%;background:#fff}";
  page += "th,td{border:1px solid #ccc;padding:6px;text-align:left;font-size:14px}";
  page += "th{background:#eee;position:sticky;top:0}";
  page += ".mono{font-family:Consolas,monospace}";
  page += "</style></head><body>";

  page += "<h1>Scanner Modbus MMW03</h1>";

  page += "<div class='box'>";
  page += "<div><b>Wi-Fi AP:</b> " + htmlEscape(String(AP_SSID)) + "</div>";
  page += "<div><b>IP:</b> " + WiFi.softAPIP().toString() + "</div>";
  page += "<div><b>Slave ID:</b> " + String(SLAVE_ID) + "</div>";
  page += "<div><b>Baud:</b> " + String(RTU_BAUD) + "</div>";
  page += "</div>";

  page += "<div class='box'>";
  page += "<label>Início <input id='start' type='number' value='" + String(SCAN_START) + "'></label>";
  page += "<label>Fim <input id='end' type='number' value='" + String(SCAN_END) + "'></label>";
  page += "<label>Passo <input id='step' type='number' value='" + String(SCAN_STEP) + "'></label>";
  page += "<label>Len <input id='len' type='number' value='" + String(READ_LEN) + "'></label>";
  page += "<br>";
  page += "<button onclick='startScan()'>Iniciar varredura</button>";
  page += "<button onclick='stopScan()'>Parar</button>";
  page += "</div>";

  page += "<div class='box mono' id='status'>Carregando...</div>";

  page += "<div class='box'>";
  page += "<table>";
  page += "<thead><tr><th>Addr</th><th>Word 0</th><th>Word 1</th><th>Float (HI,LO)</th><th>Float (LO,HI)</th></tr></thead>";
  page += "<tbody id='rows'></tbody>";
  page += "</table>";
  page += "</div>";

  page += "<script>";
  page += "async function getStatus(){";
  page += "  const r = await fetch('/status');";
  page += "  const j = await r.json();";
  page += "  document.getElementById('status').innerHTML = "
          "'scanning=' + j.scanning + "
          "' | finished=' + j.finished + "
          "' | atual=' + j.current + "
          "' | ok=' + j.okCount + "
          "' | fail=' + j.failCount + "
          "' | encontrados=' + j.foundCount;";
  page += "  let html='';";
  page += "  for (const it of j.items) {";
  page += "    html += `<tr>` +";
  page += "            `<td>${it.addr}</td>` +";
  page += "            `<td>${it.w0}</td>` +";
  page += "            `<td>${it.w1}</td>` +";
  page += "            `<td>${it.f_hilo}</td>` +";
  page += "            `<td>${it.f_lohi}</td>` +";
  page += "            `</tr>`;";
  page += "  }";
  page += "  document.getElementById('rows').innerHTML = html;";
  page += "}";
  page += "async function startScan(){";
  page += "  const s=document.getElementById('start').value;";
  page += "  const e=document.getElementById('end').value;";
  page += "  const p=document.getElementById('step').value;";
  page += "  const l=document.getElementById('len').value;";
  page += "  await fetch(`/start?start=${s}&end=${e}&step=${p}&len=${l}`);";
  page += "}";
  page += "async function stopScan(){ await fetch('/stop'); }";
  page += "setInterval(getStatus, 1000);";
  page += "getStatus();";
  page += "</script>";

  page += "</body></html>";

  server.send(200, "text/html; charset=utf-8", page);
}

void handleStart() {
  if (server.hasArg("start")) SCAN_START = (uint16_t)server.arg("start").toInt();
  if (server.hasArg("end"))   SCAN_END   = (uint16_t)server.arg("end").toInt();
  if (server.hasArg("step"))  SCAN_STEP  = max((int)1, (int)server.arg("step").toInt());
  if (server.hasArg("len"))   READ_LEN   = max((int)1, (int)server.arg("len").toInt());

  startScan();
  server.send(200, "text/plain", "OK");
}

void handleStop() {
  scanning = false;
  server.send(200, "text/plain", "STOPPED");
}

void handleStatus() {
  String json = "{";
  json += "\"scanning\":" + String(scanning ? "true" : "false") + ",";
  json += "\"finished\":" + String(scanFinished ? "true" : "false") + ",";
  json += "\"current\":" + String(currentAddr) + ",";
  json += "\"okCount\":" + String(okCount) + ",";
  json += "\"failCount\":" + String(failCount) + ",";
  json += "\"foundCount\":" + String(foundCount) + ",";
  json += "\"items\":[";

  uint16_t start = 0;
  if (foundCount > 120) start = foundCount - 120;  // mostra últimos 120

  for (uint16_t i = start; i < foundCount; i++) {
    if (i > start) json += ",";
    float f_hilo = regsToFloat(found[i].w0, found[i].w1);
    float f_lohi = regsToFloat(found[i].w1, found[i].w0);

    json += "{";
    json += "\"addr\":" + String(found[i].addr) + ",";
    json += "\"w0\":" + String(found[i].w0) + ",";
    json += "\"w1\":" + String(found[i].w1) + ",";
    json += "\"f_hilo\":\"" + String(f_hilo, 4) + "\",";
    json += "\"f_lohi\":\"" + String(f_lohi, 4) + "\"";
    json += "}";
  }

  json += "]}";
  server.send(200, "application/json", json);
}

// ========================= SETUP =========================
void setup() {
  Serial.begin(RTU_BAUD, SERIAL_8N1);
  delay(300);

  pinMode(HALF_OR_FULL_PIN, OUTPUT);
  pinMode(DE_RE, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);

  digitalWrite(HALF_OR_FULL_PIN, LOW);   // Half duplex
  digitalWrite(LED_PIN, HIGH);           // LED apagado (ativo em LOW)
  rs485TxEnable(false);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  server.on("/", handleRoot);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.on("/status", handleStatus);
  server.begin();

  mbRTU.begin(&Serial, DE_RE);
  mbRTU.master();

  resetScanState();

  Serial.println();
  Serial.println("=== MMW03 Scanner ===");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Abra no navegador e inicie a varredura.");
}

// ========================= LOOP =========================
void loop() {
  server.handleClient();
  mbRTU.task();
  yield();

  // timeout RTU
  if (rtu.busy && (millis() - rtuTxnStart > RTU_TIMEOUT_MS)) {
    rtu.busy = false;
    rtu.ok = false;
  }

  // conclusão de leitura
  static bool lastBusy = false;
  if (lastBusy && !rtu.busy && scanning) {
    if (rtu.ok) {
      okCount++;
      uint16_t w0 = 0;
      uint16_t w1 = 0;

      if (READ_LEN >= 1) w0 = rxBuf[0];
      if (READ_LEN >= 2) w1 = rxBuf[1];

      saveFound(currentAddr, w0, w1);
      digitalWrite(LED_PIN, LOW);
    } else {
      failCount++;
      digitalWrite(LED_PIN, HIGH);
    }

    // próximo endereço
    if ((uint32_t)currentAddr + SCAN_STEP > SCAN_END) {
      scanning = false;
      scanFinished = true;
    } else {
      currentAddr += SCAN_STEP;
      lastStartMs = millis();
    }
  }
  lastBusy = rtu.busy;

  // inicia próxima leitura
  if (scanning && !rtu.busy) {
    if (millis() - lastStartMs >= 20) {
      memset(rxBuf, 0, sizeof(rxBuf));
      if (!rtuReadHreg(currentAddr, rxBuf, READ_LEN)) {
        failCount++;
        if ((uint32_t)currentAddr + SCAN_STEP > SCAN_END) {
          scanning = false;
          scanFinished = true;
        } else {
          currentAddr += SCAN_STEP;
          lastStartMs = millis();
        }
      }
    }
  }
}