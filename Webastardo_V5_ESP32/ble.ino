//*******************************************************************************************
// Bluetooth LE for the phone app (docs/index.html - an installable web app using Web Bluetooth)
//
// The phone app has the everyday controls only: heater on/off, room target & Run to Temp,
// convection fan, fuel purge and the live data.  Setup & tuning stay on the WiFi web page.
//
// Security: every characteristic needs an encrypted, authenticated (MITM) link, so a phone has
// to pair with the 6-digit BLE_Pin first.  Android asks for the PIN once and remembers it.
// Changing the PIN (web page) forgets all paired phones.
//
// GATT service 6e57a5d0-0b1e-4c2a-9f3e-5741a7d0c001
//   ...c002 Status  READ / NOTIFY  Compact JSON, sent once a second (keys listed in BLE_Status_JSON)
//   ...c003 Command WRITE          Text command, same as the web page: see Process_Command()
//   ...c004 Reply   READ / NOTIFY  {"ok":true|false,"msg":"..."} after each command
//   ...c005 Errors  READ / NOTIFY  Plain-English fault messages, one per line ("" = none).
//                                  Status "e" changes whenever they change.
//*******************************************************************************************
#define BLE_SERVICE_UUID "6e57a5d0-0b1e-4c2a-9f3e-5741a7d0c001"
#define BLE_STATUS_UUID  "6e57a5d0-0b1e-4c2a-9f3e-5741a7d0c002"
#define BLE_COMMAND_UUID "6e57a5d0-0b1e-4c2a-9f3e-5741a7d0c003"
#define BLE_REPLY_UUID   "6e57a5d0-0b1e-4c2a-9f3e-5741a7d0c004"
#define BLE_ERRORS_UUID  "6e57a5d0-0b1e-4c2a-9f3e-5741a7d0c005"
#define BLE_NAME         "Webastardo"

NimBLEServer *bleServer = NULL;
NimBLECharacteristic *bleStatus = NULL;
NimBLECharacteristic *bleCommand = NULL;
NimBLECharacteristic *bleReply = NULL;
NimBLECharacteristic *bleErrors = NULL;

//*******************************************************************************************
// Connection events
//*******************************************************************************************
class BLE_Server_Callbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override {
    Serial.print("BLE: phone connected "); Serial.println(connInfo.getAddress().toString().c_str());
  }
  void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override {
    Serial.println("BLE: phone disconnected");    //Advertising restarts automatically
  }
  uint32_t onPassKeyDisplay() override {
    Serial.println("BLE: phone is pairing - enter the PIN on the phone");
    return BLE_Pin;
  }
  void onAuthenticationComplete(NimBLEConnInfo &connInfo) override {
    if (!connInfo.isEncrypted()) {
      Serial.println("BLE: pairing failed (wrong PIN?) - disconnecting");
      NimBLEDevice::getServer()->disconnect(connInfo.getConnHandle());
    } else {
      Serial.println("BLE: paired / encrypted");
    }
  }
};

//*******************************************************************************************
// A command written by the phone.  Runs in the BLE task - Process_Command() only sets flags.
//*******************************************************************************************
class BLE_Command_Callbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override {
    std::string v = pCharacteristic->getValue();
    String cmd = String(v.c_str()).substring(0, 40);
    String msg;
    //Only the everyday commands are allowed over Bluetooth
    bool allowed = cmd == "heater_on" || cmd == "heater_off" || cmd == "purge_on" || cmd == "purge_off" ||
                   cmd == "fan_on" || cmd == "fan_off" || cmd.startsWith("room_target=") || cmd.startsWith("run_to_temp=");
    bool ok = false;
    if (allowed) ok = Process_Command(cmd, msg);
    else msg = "Not available over Bluetooth";
    Serial.print("BLE command: "); Serial.print(cmd); Serial.print(" -> "); Serial.println(msg);

    String reply = String("{\"ok\":") + (ok ? "true" : "false") + ",\"msg\":\"" + Json_Escape(msg) + "\"}";
    bleReply->setValue(reply.c_str());
    bleReply->notify();
  }
};

//*******************************************************************************************
// Start Bluetooth
//*******************************************************************************************
void BLE_setup() {
  NimBLEDevice::init(BLE_NAME);
  NimBLEDevice::setMTU(517);                         //Room for the status JSON in one notification
  NimBLEDevice::setSecurityAuth(true, true, true);   //Bonding, MITM protection, Secure Connections
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);  //We "display" a fixed PIN; the phone enters it
  NimBLEDevice::setSecurityPasskey(BLE_Pin);

  bleServer = NimBLEDevice::createServer();
  bleServer->setCallbacks(new BLE_Server_Callbacks());
  bleServer->advertiseOnDisconnect(true);

  NimBLEService *svc = bleServer->createService(BLE_SERVICE_UUID);
  bleStatus = svc->createCharacteristic(BLE_STATUS_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN | NIMBLE_PROPERTY::NOTIFY);
  bleCommand = svc->createCharacteristic(BLE_COMMAND_UUID,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHEN);
  bleReply = svc->createCharacteristic(BLE_REPLY_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN | NIMBLE_PROPERTY::NOTIFY);
  bleErrors = svc->createCharacteristic(BLE_ERRORS_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN | NIMBLE_PROPERTY::NOTIFY);
  bleErrors->setValue("");
  bleCommand->setCallbacks(new BLE_Command_Callbacks());
  bleStatus->setValue(BLE_Status_JSON().c_str());
  bleReply->setValue("{}");

  //Service UUID in the advert (so the app can find it), name in the scan response
  NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(BLE_SERVICE_UUID);
  NimBLEAdvertisementData scan;
  scan.setName(BLE_NAME);
  adv->setScanResponseData(scan);
  adv->enableScanResponse(true);
  adv->start();

  Serial.print("Bluetooth: \"" BLE_NAME "\" PIN "); Serial.println(BLE_Pin);
}

void BLE_Set_Pin(uint32_t pin) {
  NimBLEDevice::setSecurityPasskey(pin);
  Serial.print("Bluetooth PIN changed to "); Serial.println(pin);
}

void BLE_Forget_Phones() {
  //Drop anyone connected, then delete the stored pairings
  if (bleServer) {
    std::vector<uint16_t> peers = bleServer->getPeerDevices();
    for (uint16_t h : peers) bleServer->disconnect(h);
  }
  NimBLEDevice::deleteAllBonds();
  Serial.println("Bluetooth: all paired phones forgotten");
}

//*******************************************************************************************
// Once a second from loop(): update the status and notify a connected phone
//*******************************************************************************************
void BLE_Send_Status() {
  static uint32_t Sent_Error_Seq = 0xFFFFFFFF;
  if (!bleStatus) return;
  if (Error_Seq != Sent_Error_Seq) {
    Sent_Error_Seq = Error_Seq;
    String e = ErrorMessage;
    if (e.length() > 500) e = e.substring(0, 497) + "...";   //One ATT read holds up to 512 bytes
    bleErrors->setValue(e.c_str());
    if (bleServer->getConnectedCount() > 0) bleErrors->notify();
  }
  bleStatus->setValue(BLE_Status_JSON().c_str());
  if (bleServer->getConnectedCount() > 0) bleStatus->notify();
}

//*******************************************************************************************
// Compact status for the phone app (short keys to fit one notification):
//  v version, m burn_mode, mt mode text, i info, e error sequence (0 = no errors), h HEAT input, w remote ON, p purge,
//  cf conv fan on, cm conv fan manual, b battery V, bn bench mode, bl battery lockout,
//  wt water C, et exhaust C, rt room C, rf room fitted, rx room fault, tg room target,
//  rr Run to Temp, tr target reached, wf/ef water/exhaust sensor fault, fl flame,
//  fn fan %, fu fuel Hz, ph pump Hz, g glow s left, wp water pump %, s cycle s, f ignition failures,
//  sim simulated sensors
//*******************************************************************************************
String BLE_Status_JSON() {
  String j;
  j.reserve(480);
  j = "{\"v\":\"" FIRMWARE_VERSION "\"";
  j += ",\"m\":" + String(burn_mode);
  j += ",\"mt\":\"" + String(Burn_Mode_Text()) + "\"";
  j += ",\"i\":\"" + Json_Escape(InfoMessage) + "\"";
  j += ",\"e\":" + String(ErrorMessage.length() > 0 ? Error_Seq : 0);
  j += ",\"h\":" + String(Pushbutton() ? 1 : 0);
  j += ",\"w\":" + String(Web_heater_ON ? 1 : 0);
  j += ",\"p\":" + String(Web_purge_ON ? 1 : 0);
  j += ",\"cf\":" + String(conv_fan_on ? 1 : 0);
  j += ",\"cm\":" + String(Web_conv_fan_ON ? 1 : 0);
  j += ",\"b\":" + Json_Num(battery_voltage, 1);
  j += ",\"bn\":" + String(bench_mode ? 1 : 0);
  j += ",\"bl\":" + String(battery_lockout ? 1 : 0);
  j += ",\"wt\":" + (water_sensor_fault ? String("null") : Json_Num(water_temp, 1));
  j += ",\"et\":" + (exhaust_sensor_fault ? String("null") : Json_Num(exhaust_temp, 0));
  j += ",\"rt\":" + Json_Num(room_temp, 1);
  j += ",\"rf\":" + String(room_sensor_fitted ? 1 : 0);
  j += ",\"rx\":" + String(room_sensor_fault ? 1 : 0);
  j += ",\"tg\":" + Json_Num(room_temp_target, 1);
  j += ",\"rr\":" + String(RunToTemp ? 1 : 0);
  j += ",\"tr\":" + String(Room_Target_Reached ? 1 : 0);
  j += ",\"wf\":" + String(water_sensor_fault ? 1 : 0);
  j += ",\"ef\":" + String(exhaust_sensor_fault ? 1 : 0);
  j += ",\"fl\":" + String(Is_Flame() ? 1 : 0);
  j += ",\"fn\":" + Json_Num(fan_speed, 0);
  j += ",\"fu\":" + Json_Num(fuel_need, 2);
  j += ",\"ph\":" + Json_Num(Fuel_Pump_Hz(), 2);
  j += ",\"g\":" + String(glow_plug_on ? glow_left : 0);
  j += ",\"wp\":" + Json_Num(water_pump_speed, 0);
  j += ",\"s\":" + String(seconds);
  j += ",\"f\":" + String(Ignition_Failures);
  j += ",\"sim\":" + String(SIMULATE_SENSORS ? 1 : 0);
  j += "}";
  return j;
}
