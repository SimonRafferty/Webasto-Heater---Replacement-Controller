bool rebootInitiated = false;

//*******************************************************************************************
//404 Not found InfoMessage
//*******************************************************************************************
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

//*******************************************************************************************
// When the HTML is being rendered, placeholders are replaced withreal values from globals
// Look in html.ino for something like %TEMPERATURE%.  This gets replaced with a value
//*******************************************************************************************
String processor(const String& var){
  if(burn_mode !=2) Fueling_Level = 0; //Switch off all highlights

  //Serial.println(var);
  if(var == "ROOM_TEMP"){
    return (String)room_temp; //Room Temp
  }
  else if(var == "THRESHOLD"){
    return (String)room_temp_target;
  }
  else if(var == "RUN_TO_TEMP"){ //Checkbox value "true" / "false"
    if(RunToTemp){
      return "checked";
    }else{
      return "unchecked";
    }
  }
  else if(var == "PUSHBUTTON"){
    if(Pushbutton()){
      return "ON";
    }else{
      return "OFF";
    }   
  }
  else if(var == "BATT_VOLTS"){
    return (String)battery_voltage;
  }
  else if(var == "BATT_AMPS"){
    float Batt_Amps = ACS.mA_DC(100)*0.0012;
    if(Batt_Amps <0) Batt_Amps = 0;
    return (String)Batt_Amps;
  }
  else if(var == "WATER_TEMP"){
    return (String)water_temp;
  }
  else if(var == "EXHAUST_TEMP"){
    return (String)get_flame_temp(); //Exhaust Temp either from flame sensor or thermistor
  }
  else if(var == "FLAME"){
    if(Is_Flame()){
      return "ALIGHT";
    }else{
      return "EXTINGUISHED";
    }
  }
  else if(var == "WATER_PUMP"){
    return (String)water_pump_speed;
  }
  else if(var == "FAN"){
    return (String)fan_speed;
  }
  else if(var == "FUEL"){
    return (String)fuel_need;
  }
  else if(var == "GLOW_TIME"){
    return (String)glow_left;
  }
  else if(var == "SECONDS"){
    return (String)seconds;
  }
  else if(var == "BURN_MODE"){
    return (String)burn_mode;
  }
  else if(var == "ERRORMESSAGE"){
    return ErrorMessage;
  }
  else if(var == "INFOMESSAGE"){
    return InfoMessage;
  }
  else if(var == "HIGH_FUEL"){
    return (String)throttling_high_fuel;
  }
  else if(var == "HIGH_FAN"){
    return (String)throttling_high_fan;
  }
  else if(var == "STEADY_FUEL"){
    return (String)throttling_steady_fuel;
  }
  else if(var == "STEADY_FAN"){
    return (String)throttling_steady_fan;
  }
  else if(var == "LOW_FUEL"){
    return (String)throttling_low_fuel;
  }
  else if(var == "LOW_FAN"){
    return (String)throttling_low_fan;
  }
  else if(var == "IDLE_FUEL"){
    return (String)throttling_idle_fuel;
  }
  else if(var == "IDLE_FAN"){
    return (String)throttling_idle_fan;
  }
  else if(var == "RUNNING"){
    if(Web_heater_ON){
      return "ON";
    } else {
      return "OFF";
    }
  }  
  else if(var == "HIGHLIGHT1"){
    if(Fueling_Level==4){
      return "ffcc33";
    }else{
      return "cccccc";
    }
  }
  else if(var == "HIGHLIGHT1B"){
    if(Fueling_Level==4){
      return "ffcc33";
    }else{
      return "ffffff";
    }
  }
  else if(var == "HIGHLIGHT2"){
    if(Fueling_Level==3){
      return "ffcc33";
    }else{
      return "cccccc";
    }
  }
  else if(var == "HIGHLIGHT2B"){
    if(Fueling_Level==3){
      return "ffcc33";
    }else{
      return "ffffff";
    }
  }
  else if(var == "HIGHLIGHT3"){
    if(Fueling_Level==2){
      return "ffcc33";
    }else{
      return "cccccc";
    }
  }
  else if(var == "HIGHLIGHT3B"){
    if(Fueling_Level==2){
      return "ffcc33";
    }else{
      return "ffffff";
    }
  }
  else if(var == "HIGHLIGHT4"){
    if(Fueling_Level==1){
      return "ffcc33";
    }else{
      return "cccccc";
    }
  }
  else if(var == "HIGHLIGHT4B"){
    if(Fueling_Level==1){
      return "ffcc33";
    }else{
      return "ffffff";
    }
  }
  else if(var == "PURGE"){
    if(Web_purge_ON){
      return "ON";
    } else {
      return "OFF";
    }
  }

  else if(var == "MIN_TEMP"){
    return (String)heater_min;
  }
  else if(var == "TARGET_TEMP"){
    return (String)heater_target;
  }
  else if(var == "HIGH_TEMP"){
    return (String)water_warning;
  }
  else if(var == "OVER_TEMP"){
    return (String)water_overheat;
  }
  else if(var == "FLAME_TEMP"){
    return (String)flame_threshold;
  }
  else if(var == "PUMP_SIZE"){
    return (String)pump_size;
  }


  return String();
}

//*******************************************************************************************
// Is WiFi still connected?  If not, try reconnecting
//*******************************************************************************************
bool Check_WiFi(){
  if(WiFi.status() == WL_CONNECTED){
    return true;
  } else {
    WiFi.disconnect();
    WiFi.reconnect();
    if(WiFi.status() == WL_CONNECTED){
      return true;
    } else {
      return false;
    }
  }
}




//*******************************************************************************************
// Initial setup of WiFi & Web Server
//*******************************************************************************************
void WiFi_setup(){
//Setup the wifi for the web server
IPAddress IP; 

  WiFi.mode(WIFI_STA);
  if(SECRET_SSID != "" || SECRET_PASS != ""){
    //Wifi details set - try connecting
  
    WiFi.begin(SECRET_SSID, SECRET_PASS);
    //Wait 10 sec for a connection
    Serial.println("Connecting to Access point....");
    if (WiFi.waitForConnectResult(4000) != WL_CONNECTED) {
      //Setup a local server
      Serial.println("Connect failed, setting up local Access Point....");
      WiFi.softAP("Webastardo", "Webastardo"); //SSID & Password 'Webastardo'
      IP = WiFi.softAPIP();
    } else {
      IP = WiFi.localIP();
    }
  } else {
    //Setup a local server
    WiFi.softAP("Webastardo", "Webastardo");
    IP = WiFi.softAPIP();
  }
  
  if(WiFi.status() != WL_CONNECTED){
    //No WiFi connection, no webserver
    Serial.println("Failed to connect to WiFi / setup AP - Reboot in 5");
    vTaskDelay(5000);
    ESP.restart();
  }

  printWiFiStatus();
  vTaskDelay(100);
}

//*******************************************************************************************
// Wifi Status
//*******************************************************************************************
void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.println("Webastardo WiFi:");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi 101 Shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: http:// ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
//*******************************************************************************************
// Initiate the webserver events inside another task, running on core 0
//*******************************************************************************************
void WebServerEvents(void * pvParameters){

  Serial.print("Webserver running on core ");
  Serial.println(xPortGetCoreID());


  //These are callback functions triggered by the web server
  // Send web page to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", data_html, processor);
  });

  // Handle the reboot command
  server.on("/doReboot", HTTP_GET, [](AsyncWebServerRequest *request) {
    rebootInitiated = true; // Set the flag
    request->send(200, "text/plain", "Rebooting..."); // Acknowledge the reboot
  });

  // Serve a simple Rebooting page
  server.on("/rebooting", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (rebootInitiated) {
      request->send(200, "text/html", "<html><body><h1>Rebooting, please wait...</h1><br> Refresh browser in 20 seconds</body></html>");
      delay(1000); // Give some time for the page to load
      ESP.restart(); // Reboot the ESP32
    } else {
      // Redirect to home if not in a reboot process
      request->redirect("/");
    }
  });


  // Receive an HTTP GET request at <ESP_IP>/get?threshold_input=<inputMessage>&enable_arm_input=<inputMessage2>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    // One or more values have been sent by the web page.  Read the values and act accordingly.  This will happen automatically, doesn't need to be polled
    if (request->hasParam("heater_State")) {
      inputMessage = request->getParam("heater_State")->value();
      //Toggle on/off state
      if(inputMessage=="ON"){
        Web_heater_ON = false;
      } else {
        Web_heater_ON = true;
      }
    }
    if (request->hasParam("reboot")) {
      request->redirect("/"); // Redirect to the same page without the query to clear it
    }

    if (request->hasParam("threshold_input")) {
        inputMessage = request->getParam("threshold_input")->value();
        if(isFloat(inputMessage)) room_temp_target = inputMessage.toFloat();
        SavePreferences(); //Save any changes to flash
    }
    if (request->hasParam("run_to_temp")) {
        inputMessage = request->getParam("run_to_temp")->value();
        if((inputMessage=="on")){
          RunToTemp = true;
        }
        Serial.print("Run_to_temp: "); Serial.println(RunToTemp);
        SavePreferences(); //Save any changes to flash
    }
    if (request->hasParam("run_to_temp_state") && (!request->hasParam("run_to_temp"))) {
        inputMessage = request->getParam("run_to_temp_state")->value();
        if((inputMessage=="checked")){
          RunToTemp = false;
        }
        Serial.print("Run_to_temp: "); Serial.println(RunToTemp);
        SavePreferences(); //Save any changes to flash
    }
    if (request->hasParam("high_fuel_input")) {
        inputMessage = request->getParam("high_fuel_input")->value();
        float new_throttling_high_fuel;
        if(isFloat(inputMessage)) new_throttling_high_fuel = inputMessage.toFloat();
        if(new_throttling_high_fuel != throttling_high_fuel){
          throttling_high_fuel = new_throttling_high_fuel;
          SavePreferences(); //Save any changes to flash
        }
    }
    if (request->hasParam("high_fan_input")) {
        inputMessage = request->getParam("high_fan_input")->value();
        float new_throttling_high_fan;
        if(isFloat(inputMessage)) new_throttling_high_fan = inputMessage.toFloat();
        if(new_throttling_high_fan != throttling_high_fan){
          throttling_high_fan = new_throttling_high_fan;
          SavePreferences(); //Save any changes to flash
        }
    }
    if (request->hasParam("steady_fuel_input")) {
        inputMessage = request->getParam("steady_fuel_input")->value();
        float new_throttling_steady_fuel;
        if(isFloat(inputMessage)) new_throttling_steady_fuel = inputMessage.toFloat();
        if(new_throttling_steady_fuel != throttling_steady_fuel){
          throttling_steady_fuel = new_throttling_steady_fuel;
          SavePreferences(); //Save any changes to flash
        }
    }
    if (request->hasParam("steady_fan_input")) {
        inputMessage = request->getParam("steady_fan_input")->value();
        float new_throttling_steady_fan;
        if(isFloat(inputMessage)) new_throttling_steady_fan = inputMessage.toFloat();
        if(new_throttling_steady_fan != throttling_steady_fan){
          throttling_steady_fan = new_throttling_steady_fan;
          SavePreferences(); //Save any changes to flash
        }
    }
    if (request->hasParam("low_fuel_input")) {
        inputMessage = request->getParam("low_fuel_input")->value();
        float new_throttling_low_fuel;
        if(isFloat(inputMessage)) new_throttling_low_fuel = inputMessage.toFloat();
        if(new_throttling_low_fuel != throttling_low_fuel){
          throttling_low_fuel = new_throttling_low_fuel;
          SavePreferences(); //Save any changes to flash
        }
    }
    if (request->hasParam("low_fan_input")) {
        inputMessage = request->getParam("low_fan_input")->value();
        float new_throttling_low_fan;
        if(isFloat(inputMessage)) new_throttling_low_fan = inputMessage.toFloat();
        if(new_throttling_low_fan != throttling_low_fan){
          throttling_low_fan =new_throttling_low_fan;
          SavePreferences(); //Save any changes to flash
        }
    }
    if (request->hasParam("idle_fuel_input")) {
        inputMessage = request->getParam("idle_fuel_input")->value();
        float new_throttling_idle_fuel;
        if(isFloat(inputMessage)) new_throttling_idle_fuel = inputMessage.toFloat();
        if(new_throttling_idle_fuel != throttling_idle_fuel){
          throttling_idle_fuel = new_throttling_idle_fuel;
          SavePreferences(); //Save any changes to flash
        }
    }
    if (request->hasParam("idle_fan_input")) {
        inputMessage = request->getParam("idle_fan_input")->value();
        float new_throttling_idle_fan;
        if(isFloat(inputMessage)) new_throttling_idle_fan = inputMessage.toFloat();
        if(new_throttling_idle_fan != throttling_idle_fan){
          throttling_idle_fan = new_throttling_idle_fan;
          SavePreferences(); //Save any changes to flash
        }
    }
    if (request->hasParam("min_temp_input")) {
        inputMessage = request->getParam("min_temp_input")->value();
        float new_heater_min;
        if(isFloat(inputMessage)) new_heater_min = inputMessage.toFloat();
        if(new_heater_min != heater_min){
          heater_min = new_heater_min;
          SavePreferences(); //Save any changes to flash
        }
    }
    if (request->hasParam("target_temp_input")) {
        inputMessage = request->getParam("target_temp_input")->value();
        float new_heater_target;
        if(isFloat(inputMessage)) new_heater_target = inputMessage.toFloat();
        if(new_heater_target != heater_target){
          heater_target = new_heater_target;
          SavePreferences(); //Save any changes to flash
        }
    }
    if (request->hasParam("high_temp_input")) {
        inputMessage = request->getParam("high_temp_input")->value();
        float new_water_warning;
        if(isFloat(inputMessage)) new_water_warning = inputMessage.toFloat();
        if(new_water_warning != water_warning){
          water_warning = new_water_warning;
          SavePreferences(); //Save any changes to flash
        }
    }
  
    if (request->hasParam("over_temp_input")) {
        inputMessage = request->getParam("over_temp_input")->value();
        float new_water_overheat;
        if(isFloat(inputMessage)) new_water_overheat = inputMessage.toFloat();
        if(new_water_overheat != water_overheat){
          water_overheat = new_water_overheat;
          SavePreferences(); //Save any changes to flash
        }
    }


    if (request->hasParam("fl_threshold_input")) {
        inputMessage = request->getParam("fl_threshold_input")->value();
        float new_flame_threshold;
        if(isFloat(inputMessage)) new_flame_threshold = inputMessage.toFloat();
        if(new_flame_threshold != flame_threshold){
          flame_threshold = new_flame_threshold;
          SavePreferences(); //Save any changes to flash
        }
    }

    if (request->hasParam("pump_size_input")) {
        inputMessage = request->getParam("pump_size_input")->value();
        float new_pump_size;
        if(isFloat(inputMessage)) new_pump_size = inputMessage.toFloat();
        if(new_pump_size <= 0) new_pump_size = 0;
        if((new_pump_size > 0) && (new_pump_size<30)) new_pump_size = 22;
        if((new_pump_size >= 30) && (new_pump_size<60)) new_pump_size = 33;
        if((new_pump_size >= 60)) new_pump_size = 60;    
        if(new_pump_size != pump_size){   
          pump_size = new_pump_size;
          SavePreferences(); //Save any changes to flash
        }
    }


    if (request->hasParam("Purge_State")) {
        inputMessage = request->getParam("Purge_State")->value();
        if(inputMessage=="ON"){
          Web_purge_ON = false;
        } else {
          if(!heater_on()){ //Only allow Purge if heater is off
            Web_purge_ON = true;
          }else{
            Web_purge_ON = false;
          }
        }
    }
    if (request->hasParam("reset")) {
        inputMessage = request->getParam("reset")->value();
        ResetPreferences();  //Purge the SPIFFS data and reboot to load defaults
        
    }
    if (request->hasParam("clearerrors")) {
        inputMessage = request->getParam("clearerrors")->value();
        // Restart ESP to reload all the variables with default values
        Serial.print("**** CLEAR ERRORS ****");
        ErrorMessage = "";
    }
    
    
    request->send_P(200, "text/html", redirect_html, processor);
    //request->send(200, "text/html", "HTTP GET request sent to your ESP.<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);
  server.begin(); 

  //This is an endless loop to keep the task running
  for(;;) {
      // This is a simple way to keep the task running without doing anything.
      vTaskDelay(portMAX_DELAY);
  }



}


//*******************************************************************************************
// Validity checking for web inputs
//*******************************************************************************************
boolean isFloat(String tString) {
  String tBuf;
  boolean decPt = false;
  
  if(tString.charAt(0) == '+' || tString.charAt(0) == '-') tBuf = &tString[1];
  else tBuf = tString;  

  for(int x=0;x<tBuf.length();x++)
  {
    if(tBuf.charAt(x) == '.') {
      if(decPt) return false;
      else decPt = true;  
    }    
    else if(tBuf.charAt(x) < '0' || tBuf.charAt(x) > '9') return false;
  }
  return true;
}