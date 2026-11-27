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
static const uint32_t RTU_TIMEOUT_MS = 350;
static const uint16_t MAX_SCAN_ITEMS  = 700;
static const uint16_t MAX_SLAVE_HITS  = 247;
static const uint16_t MAX_LOG_LINES   = 80;
static const uint16_t MAX_READ_LEN    = 4;

// ========================= OBJETOS =========================
ESP8266WebServer server(80);
ModbusRTU mbRTU;

// ========================= CONFIG RTU =========================
struct SerialSettings {
  uint32_t baud = 38400;
  char parity = 'N';    // N / E / O
  uint8_t stopBits = 1; // 1 / 2
} serialCfg;

// ========================= LOGS =========================
String logLines[MAX_LOG_LINES];
uint16_t logCount = 0;
uint16_t logWriteIdx = 0;

// ========================= ESTADO RTU =========================
struct RtuReq {
  bool busy = false;
  bool cbCalled = false;
  bool ok = false;
  uint8_t rc = 0;
  uint8_t slave = 1;
  uint8_t fn = 3;
  uint16_t addr = 0;
  uint16_t len = 1;
} rtu;

uint32_t rtuTxnStart = 0;
uint16_t rxRegs[MAX_READ_LEN];
bool     rxBits[MAX_READ_LEN];

// ========================= RESULTADOS =========================
struct ScanItem {
  uint8_t fn;
  uint8_t slave;
  uint16_t addr;
  uint16_t len;
  uint16_t v[4];
};

ScanItem found[MAX_SCAN_ITEMS];
uint16_t foundCount = 0;

struct SlaveHit {
  uint8_t id;
  bool success;
  uint8_t rc;
};

SlaveHit slaveHits[MAX_SLAVE_HITS];
uint16_t slaveHitCount = 0;

// ========================= MODO DE OPERAÇÃO =========================
enum OpMode : uint8_t {
  OP_IDLE = 0,
  OP_SLAVE_SCAN_SEND,
  OP_SLAVE_SCAN_WAIT,
  OP_REG_SCAN_SEND,
  OP_REG_SCAN_WAIT,
  OP_READ_ONCE_SEND,
  OP_READ_ONCE_WAIT
};

OpMode opMode = OP_IDLE;

// ========================= PARAMETROS SLAVE SCAN =========================
uint8_t slaveScanIdStart = 1;
uint8_t slaveScanIdEnd   = 247;
uint8_t slaveScanCurrent = 1;
uint8_t slaveScanFn      = 3;
uint16_t slaveScanAddr   = 0;

// ========================= PARAMETROS REG SCAN =========================
uint8_t regScanSlave = 1;
uint8_t regScanFn    = 3;
uint16_t regScanStart = 0;
uint16_t regScanEnd   = 1198;
uint16_t regScanStep  = 2;
uint16_t regScanLen   = 2;
uint16_t regScanCurrent = 0;

bool regScanning = false;
bool regScanFinished = false;
uint32_t okCount = 0;
uint32_t failCount = 0;

// ========================= PARAMETROS READ ONCE =========================
uint8_t readSlave = 1;
uint8_t readFn    = 3;
uint16_t readAddr = 0;
uint16_t readLen  = 1;

bool readFinished = false;
bool readSuccess  = false;
uint8_t readLastRc = 0;

// ========================= HELPERS GERAIS =========================
void addLog(const String& level, const String& msg);
String htmlEscape(const String& s);
String jsonEscape(const String& s);
float regsToFloat(uint16_t hi, uint16_t lo);
void resetRegisterResults();
void resetSlaveResults();
void stopAllOperations();
void applySerialSettings();
bool startModbusRead(uint8_t slave, uint8_t fn, uint16_t addr, uint16_t len);
void processSlaveScanLoop();
void processRegisterScanLoop();
void processReadOnceLoop();
void handleRoot();
void handleApplySerial();
void handleStartSlaveScan();
void handleStartRegisterScan();
void handleReadOnce();
void handleStop();
void handleStatus();

// ========================= CALLBACK MODBUS =========================
bool modbusCb(Modbus::ResultCode event, uint16_t, void*) {
  rtu.cbCalled = true;
  rtu.ok = (event == Modbus::EX_SUCCESS);
  rtu.rc = (uint8_t)event;
  rtu.busy = false;
  return true;
}

// ========================= SETUP =========================
void setup() {
  pinMode(HALF_OR_FULL_PIN, OUTPUT);
  pinMode(DE_RE, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);

  digitalWrite(HALF_OR_FULL_PIN, LOW);   // half duplex
  digitalWrite(DE_RE, LOW);
  digitalWrite(LED_PIN, HIGH);           // led apagado (ativo em LOW)

  applySerialSettings();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  server.on("/", handleRoot);
  server.on("/applySerial", handleApplySerial);
  server.on("/startSlaveScan", handleStartSlaveScan);
  server.on("/startRegisterScan", handleStartRegisterScan);
  server.on("/readOnce", handleReadOnce);
  server.on("/stop", handleStop);
  server.on("/status", handleStatus);
  server.begin();

  addLog("INFO", "=== Scanner Modbus iniciado ===");
  addLog("INFO", "AP IP: " + WiFi.softAPIP().toString());
  addLog("INFO", "Abra a página e ajuste baud/paridade/stop bits.");
}

// ========================= LOOP =========================
void loop() {
  server.handleClient();
  mbRTU.task();
  yield();

  if (rtu.busy && (millis() - rtuTxnStart > RTU_TIMEOUT_MS)) {
    rtu.busy = false;
    rtu.ok = false;
    rtu.cbCalled = false;
    rtu.rc = 0xE0; // timeout local
    addLog("ERR", "Timeout RTU");
  }

  processSlaveScanLoop();
  processRegisterScanLoop();
  processReadOnceLoop();
}