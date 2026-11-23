//*******************************************************************************************
// WiFi & Web Server
//
// How it fits together:
//  * The page itself (html.ino) is static.  It loads the settings from /api/settings and
//    receives live data from /events (Server-Sent Events: one connection, pushed once a second).
//  * The async web server runs in its own task.  To keep the heater code single-threaded the
//    handlers never touch the heater directly - they set request flags which loop() acts on
//    in Web_Requests(), and they read the live data from a copy built by loop().
//  * Nothing safety-related depends on WiFi.  If the WiFi drops the heater carries on.
//
// API:
//  GET  /               the page
//  GET  /api/status     live data (JSON)
//  GET  /api/settings   all settings (JSON)
//  POST /api/settings   form fields named as in Settings[] (+ pump_size, run_to_temp=0|1)
//                       Only the fields sent are changed
//  POST /api/cmd        action=heater_on|heater_off|purge_on|purge_off|fan_on|fan_off|
//                              clear_errors|system_test|reboot|factory_reset|forget_phones
//                              room_target=<C>|run_to_temp=<0|1>   (see Process_Command)
//  POST /api/wifi       ssid=...&pass=...   (saved, then reboots)
//*******************************************************************************************

SemaphoreHandle_t Web_Mutex = NULL;     //Protects Status_JSON and the pending WiFi strings
String Status_JSON = "{}";              //Built by loop(), served by the web task
String Pending_SSID, Pending_PASS;
volatile bool WiFi_Save_Pending = false;
unsigned long Reboot_At = 0;            //millis() at which to restart (0 = not pending)

//*******************************************************************************************
// Initial setup of WiFi.  Called from setup() before the watchdog is started.
//*******************************************************************************************
void WiFi_setup() {
  WiFi.persistent(false);       //Credentials live in our own preferences
  WiFi.setHostname(MDNS_NAME);

  if (WiFi_SSID.length() > 0) {
    //Wifi details set - try connecting
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WiFi_SSID.c_str(), WiFi_PASS.c_str());
    Serial.print("Connecting to "); Serial.print(WiFi_SSID); Serial.println("....");
    if (WiFi.waitForConnectResult(10000) == WL_CONNECTED) {
      AP_mode = false;
    } else {
      Serial.println("Connect failed, setting up local Access Point....");
      Start_AP();
    }
  } else {
    Start_AP();
  }

  if (MDNS.begin(MDNS_NAME)) {
    MDNS.addService("http", "tcp", 80);
  }
  printWiFiStatus();
}

void Start_AP() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  AP_mode = true;
}

//*******************************************************************************************
// Called every loop - never blocks.
// In STA mode the WiFi library reconnects by itself.
// In AP (fallback) mode, every 5 minutes while the heater is off and nobody is connected to the
// AP, try the home network again in the background.
//*******************************************************************************************
void WiFi_Maintain() {
static unsigned long last_try = 0;
static bool trying = false;

  if (!AP_mode || WiFi_SSID.length() == 0) return;

  if (!trying) {
    if (millis() - last_try > 300000UL && burn_mode == 0 && WiFi.softAPgetStationNum() == 0) {
      Serial.println("AP mode: retrying WiFi in the background");
      WiFi.mode(WIFI_AP_STA);
      WiFi.begin(WiFi_SSID.c_str(), WiFi_PASS.c_str());
      trying = true;
      last_try = millis();
    }
  } else {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi reconnected - closing Access Point");
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
      WiFi.setAutoReconnect(true);
      AP_mode = false;
      trying = false;
      printWiFiStatus();
    } else if (millis() - last_try > 20000UL) {
      WiFi.disconnect();
      WiFi.mode(WIFI_AP);
      trying = false;
      last_try = millis();
    }
  }
}

String WiFi_IP() {
  return AP_mode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
}

//*******************************************************************************************
// Wifi Status
//*******************************************************************************************
void printWiFiStatus() {
  Serial.println("Webastardo WiFi:");
  if (AP_mode) {
    Serial.print("Access Point: "); Serial.print(AP_SSID);
    Serial.print("  password: "); Serial.println(AP_PASS);
  } else {
    Serial.print("SSID: "); Serial.println(WiFi.SSID());
    Serial.print("signal strength (RSSI): "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
  }
  Serial.print("Web page: http://"); Serial.print(WiFi_IP());
  Serial.println("  or  http://" MDNS_NAME ".local");
}

//*******************************************************************************************
// Set up the web server routes.  The server runs in the AsyncTCP task.
//*******************************************************************************************
void WebServer_setup() {
  Web_Mutex = xSemaphoreCreateMutex();
  Build_Status_JSON();

  // Send web page to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", (const uint8_t *)index_html, strlen(index_html));
  });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", Get_Status_JSON());
  });

  server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", Settings_JSON());
  });

  server.on("/api/settings", HTTP_POST, Handle_Settings_Post);
  server.on("/api/cmd", HTTP_POST, Handle_Command);
  server.on("/api/wifi", HTTP_POST, Handle_WiFi_Post);

  //Live data.  Send the latest straight away when a page connects
  events.onConnect([](AsyncEventSourceClient *client) {
    client->send(Get_Status_JSON().c_str(), "status", millis());
  });
  server.addHandler(&events);

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });
  server.begin();
}

//*******************************************************************************************
// Settings received from the web page.  Every value is range checked; nothing is changed
// unless the whole lot is valid.  Saving to flash happens in loop().
//*******************************************************************************************
void Handle_Settings_Post(AsyncWebServerRequest *request) {
  float new_values[Num_Settings];
  String problems = "";

  for (int i = 0; i < Num_Settings; i++) {
    new_values[i] = *Settings[i].value;
    if (request->hasParam(Settings[i].key, true)) {
      String s = request->getParam(Settings[i].key, true)->value();
      s.trim();
      if (!isFloat(s) || s.length() == 0) {
        problems += String(Settings[i].key) + " is not a number. ";
        continue;
      }
      float v = s.toFloat();
      if (v < Settings[i].min_val || v > Settings[i].max_val) {
        problems += String(Settings[i].key) + " must be " + String(Settings[i].min_val, 2) +
                    " to " + String(Settings[i].max_val, 2) + ". ";
        continue;
      }
      new_values[i] = v;
    }
  }

  //The water thresholds must stay in order
  float n_min = new_values[Find_Setting("heater_min") - Settings];
  float n_target = new_values[Find_Setting("heater_target") - Settings];
  float n_warn = new_values[Find_Setting("water_warning") - Settings];
  float n_over = new_values[Find_Setting("water_overheat") - Settings];
  if (!(n_min < n_target && n_target < n_warn && n_warn < n_over)) {
    problems += "Temperatures must be Minimum < Target < High < Over. ";
  }

  if (problems != "") {
    request->send(400, "application/json", "{\"ok\":false,\"msg\":\"" + Json_Escape(problems) + "\"}");
    return;
  }

  for (int i = 0; i < Num_Settings; i++) *Settings[i].value = new_values[i];
  if (request->hasParam("pump_size", true)) {
    pump_size = Valid_Pump_Size(request->getParam("pump_size", true)->value().toFloat());
  }
  if (request->hasParam("ble_pin", true)) {
    long pin = request->getParam("ble_pin", true)->value().toInt();
    if (pin >= 100000 && pin <= 999999 && (uint32_t)pin != BLE_Pin) {
      BLE_Pin = pin;
      BLE_Pin_Changed = true;    //loop() applies it and forgets the old pairings
    }
  }
  if (request->hasParam("run_to_temp", true)) {
    RunToTemp = (request->getParam("run_to_temp", true)->value() == "1");
  }
  Save_Settings_Pending = true;
  request->send(200, "application/json", "{\"ok\":true,\"msg\":\"Settings saved\"}");
}

//*******************************************************************************************
// Buttons on the web page
//*******************************************************************************************
void Handle_Command(AsyncWebServerRequest *request) {
  if (!request->hasParam("action", true)) {
    request->send(400, "application/json", "{\"ok\":false,\"msg\":\"No action\"}");
    return;
  }
  String msg;
  bool ok = Process_Command(request->getParam("action", true)->value(), msg);
  request->send(ok ? 200 : 409, "application/json",
                String("{\"ok\":") + (ok ? "true" : "false") + ",\"msg\":\"" + Json_Escape(msg) + "\"}");
}

//*******************************************************************************************
// Commands shared by the web page and the Bluetooth app.  Called from the web server and BLE
// tasks, so it only sets request flags / simple values - loop() does the work.
//  heater_on|heater_off|purge_on|purge_off|fan_on|fan_off|clear_errors|system_test|reboot|
//  factory_reset|forget_phones|room_target=<C>|run_to_temp=<0|1>
//*******************************************************************************************
bool Process_Command(String action, String &msg) {
  action.trim();
  msg = "OK";

  if (action == "heater_on") {
    if (Errors_Present) { msg = "Clear the errors first"; return false; }
    Web_heater_ON = true;
  } else if (action == "heater_off") {
    Web_heater_ON = false;
    if (Pushbutton()) msg = "Remote request cancelled - the HEAT input is still on";
  } else if (action == "purge_on") {
    if (burn_mode != 0 || Web_heater_ON || Pushbutton()) { msg = "Switch the heater off first"; return false; }
    Web_purge_ON = true;
  } else if (action == "purge_off") {
    Web_purge_ON = false;
  } else if (action == "fan_on") {
    Web_conv_fan_ON = true;
  } else if (action == "fan_off") {
    Web_conv_fan_ON = false;
  } else if (action.startsWith("room_target=")) {
    String v = action.substring(12);
    if (!isFloat(v) || v.toFloat() < 5 || v.toFloat() > 35) { msg = "Target must be 5 to 35C"; return false; }
    room_temp_target = v.toFloat();
    Save_Settings_Pending = true;
    msg = "Target " + String(room_temp_target, 1) + "C";
  } else if (action.startsWith("run_to_temp=")) {
    RunToTemp = (action.substring(12) == "1");
    Save_Settings_Pending = true;
    msg = RunToTemp ? "Run to Temp ON" : "Run to Temp OFF";
  } else if (action == "clear_errors") {
    Clear_Errors_Pending = true;
  } else if (action == "system_test") {
    System_Test_Pending = true;
  } else if (action == "forget_phones") {
    Forget_Phones_Pending = true;
    msg = "Paired phones forgotten - they must pair again";
  } else if (action == "reboot") {
    if (burn_mode != 0) { msg = "Heater is running - switch off first"; return false; }
    Reboot_Pending = true; msg = "Rebooting...";
  } else if (action == "factory_reset") {
    if (burn_mode != 0) { msg = "Heater is running - switch off first"; return false; }
    Factory_Reset_Pending = true; msg = "Defaults restored - rebooting...";
  } else {
    msg = "Unknown action";
    return false;
  }
  return true;
}

//*******************************************************************************************
// New WiFi credentials (saved and applied by loop(), followed by a reboot)
//*******************************************************************************************
void Handle_WiFi_Post(AsyncWebServerRequest *request) {
  if (!request->hasParam("ssid", true)) {
    request->send(400, "application/json", "{\"ok\":false,\"msg\":\"No SSID\"}");
    return;
  }
  if (burn_mode != 0) {
    request->send(409, "application/json", "{\"ok\":false,\"msg\":\"Heater is running - switch off first\"}");
    return;
  }
  String ssid = request->getParam("ssid", true)->value();
  String pass = request->hasParam("pass", true) ? request->getParam("pass", true)->value() : "";
  if (ssid.length() > 32 || pass.length() > 63) {
    request->send(400, "application/json", "{\"ok\":false,\"msg\":\"SSID or password too long\"}");
    return;
  }
  if (xSemaphoreTake(Web_Mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
    Pending_SSID = ssid;
    Pending_PASS = pass;
    WiFi_Save_Pending = true;
    xSemaphoreGive(Web_Mutex);
  }
  request->send(200, "application/json",
                "{\"ok\":true,\"msg\":\"Saved - rebooting.  Reconnect to your network (or the Webastardo AP if it fails).\"}");
}

//*******************************************************************************************
// Act on requests from the web page.  Runs in loop() so the heater code stays single-threaded
//*******************************************************************************************
void Web_Requests() {
  if (Save_Settings_Pending) {
    Save_Settings_Pending = false;
    SavePreferences();
  }
  if (Clear_Errors_Pending) {
    Clear_Errors_Pending = false;
    Serial.println("**** CLEAR ERRORS ****");
    ClearErrors();
    Heater_fail = false;
    Ignition_Failures = 0;
    System_Test();      //Sensor faults come straight back if they're still there
  }
  if (BLE_Pin_Changed) {
    BLE_Pin_Changed = false;
    preferences.putUInt("ble_pin", BLE_Pin);
    BLE_Set_Pin(BLE_Pin);
    BLE_Forget_Phones();       //Phones paired with the old PIN must pair again
  }
  if (Forget_Phones_Pending) {
    Forget_Phones_Pending = false;
    BLE_Forget_Phones();
  }
  if (System_Test_Pending) {
    System_Test_Pending = false;
    System_Test();
  }
  if (WiFi_Save_Pending && xSemaphoreTake(Web_Mutex, 0) == pdTRUE) {
    WiFi_SSID = Pending_SSID;
    WiFi_PASS = Pending_PASS;
    WiFi_Save_Pending = false;
    xSemaphoreGive(Web_Mutex);
    SaveWiFiPreferences();
    Reboot_At = millis() + 1000;
  }
  if (Factory_Reset_Pending) {
    Factory_Reset_Pending = false;
    ResetPreferences();
    BLE_Forget_Phones();
    Reboot_At = millis() + 1000;
  }
  if (Reboot_Pending) {
    Reboot_Pending = false;
    Reboot_At = millis() + 1000;    //Give the web page time to get its reply
  }
  if (Reboot_At != 0 && (long)(millis() - Reboot_At) >= 0) {
    if (burn_mode == 0) {
      Serial.println("Rebooting (web request)");
      Serial.flush();
      ESP.restart();
    }
    Reboot_At = 0;   //Heater started in the meantime - cancel
  }
}

//*******************************************************************************************
// Live data.  Built in loop() once a second, then pushed to any open pages
//*******************************************************************************************
void Send_Live_Data() {
  Build_Status_JSON();
  if (events.count() > 0) {
    events.send(Get_Status_JSON().c_str(), "status", millis());
  }
}

String Get_Status_JSON() {
  String copy;
  if (Web_Mutex && xSemaphoreTake(Web_Mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    copy = Status_JSON;
    xSemaphoreGive(Web_Mutex);
  } else {
    copy = "{}";
  }
  return copy;
}

const char *Burn_Mode_Text() {
  switch (burn_mode) {
    case 0: return "Off";
    case 1: return "Starting";
    case 2: return "Running";
    case 3: return "Shutting down";
  }
  return "?";
}

//Number or null (JSON has no NaN)
String Json_Num(float v, int decimals) {
  if (isnan(v) || isinf(v)) return "null";
  return String(v, decimals);
}

void Build_Status_JSON() {
  String j;
  j.reserve(900);
  j = "{";
  j += "\"ver\":\"" FIRMWARE_VERSION "\"";
  j += ",\"uptime\":" + String(millis() / 1000);
  j += ",\"reset\":\"" + Json_Escape(Reset_Reason) + "\"";
  j += ",\"mode\":" + String(burn_mode);
  j += ",\"mode_text\":\"" + String(Burn_Mode_Text()) + "\"";
  j += ",\"info\":\"" + Json_Escape(InfoMessage) + "\"";
  j += ",\"error\":\"" + Json_Escape(ErrorMessage) + "\"";
  j += ",\"heat_in\":" + String(Pushbutton() ? "true" : "false");
  j += ",\"web_on\":" + String(Web_heater_ON ? "true" : "false");
  j += ",\"purge\":" + String(Web_purge_ON ? "true" : "false");
  j += ",\"conv_fan\":" + String(conv_fan_on ? "true" : "false");
  j += ",\"conv_manual\":" + String(Web_conv_fan_ON ? "true" : "false");
  j += ",\"batt\":" + Json_Num(battery_voltage, 2);
  j += ",\"bench\":" + String(bench_mode ? "true" : "false");
  j += ",\"batt_lock\":" + String(battery_lockout ? "true" : "false");
  j += ",\"water\":" + (water_sensor_fault ? String("null") : Json_Num(water_temp, 1));
  j += ",\"exhaust\":" + (exhaust_sensor_fault ? String("null") : Json_Num(exhaust_temp, 1));
  j += ",\"room\":" + Json_Num(room_temp, 1);
  j += ",\"room_fitted\":" + String(room_sensor_fitted ? "true" : "false");
  j += ",\"room_target\":" + Json_Num(room_temp_target, 1);
  j += ",\"run_to_temp\":" + String(RunToTemp ? "true" : "false");
  j += ",\"target_reached\":" + String(Room_Target_Reached ? "true" : "false");
  j += ",\"water_fault\":" + String(water_sensor_fault ? "true" : "false");
  j += ",\"exh_fault\":" + String(exhaust_sensor_fault ? "true" : "false");
  j += ",\"room_fault\":" + String(room_sensor_fault ? "true" : "false");
  j += ",\"flame\":" + String(Is_Flame() ? "true" : "false");
  j += ",\"fan\":" + Json_Num(fan_speed, 1);
  j += ",\"fuel\":" + Json_Num(fuel_need, 2);
  j += ",\"pump_hz\":" + Json_Num(Fuel_Pump_Hz(), 2);
  j += ",\"pulses\":" + String(fuelPulseCount);
  j += ",\"glow\":" + String(glow_plug_on ? glow_left : 0);
  j += ",\"wpump\":" + Json_Num(water_pump_speed, 0);
  j += ",\"level\":" + String(Fueling_Level);
  j += ",\"secs\":" + String(seconds);
  j += ",\"fails\":" + String(Ignition_Failures);
  j += ",\"ap\":" + String(AP_mode ? "true" : "false");
  j += ",\"ip\":\"" + WiFi_IP() + "\"";
  j += ",\"rssi\":" + String(AP_mode ? 0 : WiFi.RSSI());
  j += ",\"mv\":{\"water\":" + String(water_mv) + ",\"room\":" + String(room_mv) +
       ",\"exh\":" + String(exhaust_mv) + ",\"vbat\":" + String(vbat_mv) + "}";
  j += ",\"sim\":" + String(SIMULATE_SENSORS ? "true" : "false");
  j += "}";

  if (Web_Mutex && xSemaphoreTake(Web_Mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    Status_JSON = j;
    xSemaphoreGive(Web_Mutex);
  }
}

//*******************************************************************************************
// Settings as JSON.  Only reads numbers & the SSID, which loop() only changes at a reboot
//*******************************************************************************************
String Settings_JSON() {
  String j = "{";
  for (int i = 0; i < Num_Settings; i++) {
    j += "\"" + String(Settings[i].key) + "\":" + Json_Num(*Settings[i].value, 3) + ",";
  }
  j += "\"pump_size\":" + String(pump_size);
  j += ",\"run_to_temp\":" + String(RunToTemp ? "true" : "false");
  j += ",\"ble_pin\":" + String(BLE_Pin);
  j += ",\"wifi_ssid\":\"" + Json_Escape(WiFi_SSID) + "\"";
  j += ",\"ap_ssid\":\"" AP_SSID "\"";
  j += "}";
  return j;
}

//*******************************************************************************************
// Make a string safe to put inside JSON quotes
//*******************************************************************************************
String Json_Escape(const String &s) {
  String out;
  out.reserve(s.length() + 8);
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s.charAt(i);
    if (c == '"' || c == '\\') { out += '\\'; out += c; }
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if ((uint8_t)c < 0x20) out += ' ';
    else out += c;
  }
  return out;
}

//*******************************************************************************************
// Validity checking for web inputs
//*******************************************************************************************
bool isFloat(String tString) {
  String tBuf;
  bool decPt = false;
  bool digits = false;

  if (tString.charAt(0) == '+' || tString.charAt(0) == '-') tBuf = tString.substring(1);
  else tBuf = tString;

  for (unsigned int x = 0; x < tBuf.length(); x++)
  {
    if (tBuf.charAt(x) == '.') {
      if (decPt) return false;
      else decPt = true;
    }
    else if (tBuf.charAt(x) < '0' || tBuf.charAt(x) > '9') return false;
    else digits = true;
  }
  return digits;
}
