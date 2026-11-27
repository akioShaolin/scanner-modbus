extern ESP8266WebServer server;
extern ModbusRTU mbRTU;
extern SerialSettings serialCfg;

extern String logLines[MAX_LOG_LINES];
extern uint16_t logCount;
extern uint16_t logWriteIdx;

extern bool regScanning;
extern bool regScanFinished;
extern bool readFinished;
extern bool readSuccess;
extern uint8_t readLastRc;
extern OpMode opMode;

extern uint16_t foundCount;
extern uint16_t slaveHitCount;

String htmlEscape(const String& s) {
  String out = s;
  out.replace("&", "&amp;");
  out.replace("<", "&lt;");
  out.replace(">", "&gt;");
  out.replace("\"", "&quot;");
  return out;
}

String jsonEscape(const String& s) {
  String out = s;
  out.replace("\\", "\\\\");
  out.replace("\"", "\\\"");
  out.replace("\n", "\\n");
  out.replace("\r", "");
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

void addLog(const String& level, const String& msg) {
  String line = "[" + level + "] " + msg;
  logLines[logWriteIdx] = line;
  logWriteIdx = (logWriteIdx + 1) % MAX_LOG_LINES;
  if (logCount < MAX_LOG_LINES) logCount++;
  Serial.println(line);
}

static SerialConfig getSerialConfig(char parity, uint8_t stopBits) {
  if (parity == 'E' && stopBits == 1) return SERIAL_8E1;
  if (parity == 'E' && stopBits == 2) return SERIAL_8E2;
  if (parity == 'O' && stopBits == 1) return SERIAL_8O1;
  if (parity == 'O' && stopBits == 2) return SERIAL_8O2;
  if (parity == 'N' && stopBits == 2) return SERIAL_8N2;
  return SERIAL_8N1;
}

void applySerialSettings() {
  Serial.flush();
  Serial.begin(serialCfg.baud, getSerialConfig(serialCfg.parity, serialCfg.stopBits));
  delay(50);

  mbRTU.begin(&Serial, DE_RE);
  mbRTU.master();

  addLog("INFO",
    "RTU aplicado: baud=" + String(serialCfg.baud) +
    " parity=" + String(serialCfg.parity) +
    " stop=" + String(serialCfg.stopBits));
}

void resetRegisterResults() {
  foundCount = 0;
}

void resetSlaveResults() {
  slaveHitCount = 0;
}

void stopAllOperations() {
  opMode = OP_IDLE;
  regScanning = false;
  regScanFinished = false;
  readFinished = false;
  readSuccess = false;
  readLastRc = 0;
  addLog("INFO", "Operações interrompidas.");
}