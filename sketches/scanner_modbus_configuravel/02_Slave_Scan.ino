extern ModbusRTU mbRTU;
extern RtuReq rtu;
extern uint32_t rtuTxnStart;
extern uint16_t rxRegs[MAX_READ_LEN];
extern bool rxBits[MAX_READ_LEN];

extern OpMode opMode;

extern uint8_t slaveScanIdStart;
extern uint8_t slaveScanIdEnd;
extern uint8_t slaveScanCurrent;
extern uint8_t slaveScanFn;
extern uint16_t slaveScanAddr;

extern SlaveHit slaveHits[MAX_SLAVE_HITS];
extern uint16_t slaveHitCount;

void addLog(const String& level, const String& msg);
bool startModbusRead(uint8_t slave, uint8_t fn, uint16_t addr, uint16_t len);

static void saveSlaveHit(uint8_t id, bool success, uint8_t rc) {
  if (slaveHitCount >= MAX_SLAVE_HITS) return;
  slaveHits[slaveHitCount].id = id;
  slaveHits[slaveHitCount].success = success;
  slaveHits[slaveHitCount].rc = rc;
  slaveHitCount++;
}

void processSlaveScanLoop() {
  if (opMode == OP_SLAVE_SCAN_SEND) {
    if (slaveScanCurrent > slaveScanIdEnd) {
      opMode = OP_IDLE;
      addLog("INFO", "Varredura de slaves finalizada.");
      return;
    }

    if (!rtu.busy) {
      if (startModbusRead(slaveScanCurrent, slaveScanFn, slaveScanAddr, 1)) {
        addLog("INFO", "Testando slave ID=" + String(slaveScanCurrent));
        opMode = OP_SLAVE_SCAN_WAIT;
      }
    }
    return;
  }

  if (opMode == OP_SLAVE_SCAN_WAIT) {
    if (rtu.busy) return;

    bool exists = rtu.cbCalled; // qualquer resposta já indica que há dispositivo
    if (exists) {
      saveSlaveHit(slaveScanCurrent, rtu.ok, rtu.rc);
      addLog("OK", "Slave " + String(slaveScanCurrent) + " respondeu. RC=0x" + String(rtu.rc, HEX));
      digitalWrite(LED_PIN, LOW);
    } else {
      addLog("ERR", "Slave " + String(slaveScanCurrent) + " sem resposta.");
      digitalWrite(LED_PIN, HIGH);
    }

    slaveScanCurrent++;
    opMode = OP_SLAVE_SCAN_SEND;
  }
}