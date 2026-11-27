extern ModbusRTU mbRTU;
extern RtuReq rtu;
extern uint32_t rtuTxnStart;
extern uint16_t rxRegs[MAX_READ_LEN];
extern bool rxBits[MAX_READ_LEN];

extern OpMode opMode;

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

void addLog(const String& level, const String& msg);

bool startModbusRead(uint8_t slave, uint8_t fn, uint16_t addr, uint16_t len) {
  if (rtu.busy) return false;

  memset(rxRegs, 0, sizeof(rxRegs));
  memset(rxBits, 0, sizeof(rxBits));

  rtu.busy = true;
  rtu.cbCalled = false;
  rtu.ok = false;
  rtu.rc = 0;
  rtu.slave = slave;
  rtu.fn = fn;
  rtu.addr = addr;
  rtu.len = len;
  rtuTxnStart = millis();

  bool started = false;
  switch (fn) {
    case 1: started = mbRTU.readCoil(slave, addr, rxBits, len, modbusCb); break;
    case 2: started = mbRTU.readIsts(slave, addr, rxBits, len, modbusCb); break;
    case 3: started = mbRTU.readHreg(slave, addr, rxRegs, len, modbusCb); break;
    case 4: started = mbRTU.readIreg(slave, addr, rxRegs, len, modbusCb); break;
    default: started = false; break;
  }

  if (!started) {
    rtu.busy = false;
    rtu.ok = false;
    rtu.cbCalled = false;
    rtu.rc = 0xE1;
    addLog("ERR", "Falha ao iniciar transação Modbus.");
    return false;
  }
  return true;
}

static void saveRegisterHit(uint8_t fn, uint8_t slave, uint16_t addr, uint16_t len) {
  if (foundCount >= MAX_SCAN_ITEMS) return;
  found[foundCount].fn = fn;
  found[foundCount].slave = slave;
  found[foundCount].addr = addr;
  found[foundCount].len = len;

  for (uint16_t i = 0; i < 4; i++) found[foundCount].v[i] = 0;

  if (fn == 1 || fn == 2) {
    for (uint16_t i = 0; i < len && i < 4; i++) {
      found[foundCount].v[i] = rxBits[i] ? 1 : 0;
    }
  } else {
    for (uint16_t i = 0; i < len && i < 4; i++) {
      found[foundCount].v[i] = rxRegs[i];
    }
  }

  foundCount++;
}

void processRegisterScanLoop() {
  if (opMode == OP_REG_SCAN_SEND) {
    if (regScanCurrent > regScanEnd) {
      regScanning = false;
      regScanFinished = true;
      opMode = OP_IDLE;
      addLog("INFO", "Varredura de registradores finalizada.");
      return;
    }

    if (!rtu.busy) {
      if (startModbusRead(regScanSlave, regScanFn, regScanCurrent, regScanLen)) {
        opMode = OP_REG_SCAN_WAIT;
      } else {
        failCount++;
        regScanCurrent = (uint16_t)(regScanCurrent + regScanStep);
      }
    }
    return;
  }

  if (opMode == OP_REG_SCAN_WAIT) {
    if (rtu.busy) return;

    if (rtu.ok) {
      okCount++;
      saveRegisterHit(regScanFn, regScanSlave, regScanCurrent, regScanLen);
      addLog("OK", "Addr " + String(regScanCurrent) + " respondeu.");
      digitalWrite(LED_PIN, LOW);
    } else {
      failCount++;
      addLog("ERR", "Addr " + String(regScanCurrent) + " falhou. RC=0x" + String(rtu.rc, HEX));
      digitalWrite(LED_PIN, HIGH);
    }

    if ((uint32_t)regScanCurrent + regScanStep > regScanEnd) {
      regScanCurrent = regScanEnd + 1;
    } else {
      regScanCurrent = (uint16_t)(regScanCurrent + regScanStep);
    }
    opMode = OP_REG_SCAN_SEND;
  }
}

void processReadOnceLoop() {
  if (opMode == OP_READ_ONCE_SEND) {
    if (!rtu.busy) {
      if (startModbusRead(readSlave, readFn, readAddr, readLen)) {
        opMode = OP_READ_ONCE_WAIT;
        addLog("INFO", "Lendo slave=" + String(readSlave) + " fn=" + String(readFn) + " addr=" + String(readAddr));
      }
    }
    return;
  }

  if (opMode == OP_READ_ONCE_WAIT) {
    if (rtu.busy) return;

    readFinished = true;
    readSuccess = rtu.ok;
    readLastRc = rtu.rc;

    if (rtu.ok) {
      addLog("OK", "Leitura pontual concluída.");
      saveRegisterHit(readFn, readSlave, readAddr, readLen);
      digitalWrite(LED_PIN, LOW);
    } else {
      addLog("ERR", "Leitura pontual falhou. RC=0x" + String(rtu.rc, HEX));
      digitalWrite(LED_PIN, HIGH);
    }

    opMode = OP_IDLE;
  }
}