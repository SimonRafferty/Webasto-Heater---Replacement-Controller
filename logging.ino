//*******************************************************************************************
// Send all the interesting data to Serial port
//*******************************************************************************************
void LoggingEvents(void * pvParameters){
//This is running in it's own process so as not to disturb heater process
  while(true) {
    if(Seconds_Ticker()){   //Perform actions once per second
      logging();
      battery_voltage = float(analogRead(batt_voltage_pin)) / 100.0;    
    }
  }
}

void logging(){
    Serial.print("BTN: ");
    Serial.print(analogRead(ON_OFF_pin)>250);

    Serial.print(" | Bat: ");
    Serial.print(battery_voltage);

//    Serial.print(" | ButV: ");
//    Serial.print(analogRead(ON_OFF_pin));

    Serial.print(" | Gl: ");
    Serial.print(glow_plug_on);
    
//    Serial.print(" | Pump: ");
//    Serial.print(water_pump_speed);    
    
    Serial.print(" | F: ");
    Serial.print(Ignition_Failures);

    Serial.print(" | Burn: ");
    Serial.print(burn);

    Serial.print(" | BMode: ");
    Serial.print(burn_mode);

//    if(burn_mode == 0)
//      Serial.print("OFF");
//    if(burn_mode == 1)
//      Serial.print("Starting");
//    if(burn_mode == 2)
//      Serial.print("Running");
//    if(burn_mode == 3)
//      Serial.print("Shuting Down");

    Serial.print(" | W_Tmp: ");
    Serial.print(int(water_temp));
    Serial.print(" | Flm: ");
    Serial.print(Is_Flame());

    Serial.print(" | F_Tmp: ");
    Serial.print(int(get_flame_temp()));
    //Useful as a comparison for accuracy of flame sensor
    Serial.print(" | E_Tmp: ");
    Serial.print(int(get_exhaust_temp()));

    
    
    Serial.print(" | Rm T: ");
    Serial.print(int(room_temp));
    Serial.print(" | Set T: ");
    Serial.print(int(room_temp_target));
    
    Serial.print(" | Fan: ");
    Serial.print(int(fan_speed));

    Serial.print(" | Fuel: ");
    Serial.print(fuel_need);
    
//    Serial.print(" | Glow For (Sec): ");
//    Serial.print(glow_time);
//    Serial.print(" | Glow Left: ");

//    Serial.print(glow_left);
    
    Serial.print(" | Time: ");


    
    Serial.print(seconds);
    Serial.print(" |  ");
    Serial.println(InfoMessage); 

  //Update the LCD Display to show the IP address of the WiFi interface plus the ON/Off status
  IPAddress ip = WiFi.localIP();
  String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  updateDisplay(ipStr, burn_mode);

}

//*******************************************************************************************
// Draw the background on the LCD Display
//*******************************************************************************************
void drawLayout() {
  display.drawRect(0, 0, 128, 17, SSD1306_WHITE); // IP Address box
  display.drawRect(0, 16, 128, 16, SSD1306_WHITE); // Heater status box
  display.display();
}

//*******************************************************************************************
// Update values on the display
//*******************************************************************************************
void updateDisplay(String ip, byte heaterStatus) {
  display.clearDisplay();
  drawLayout();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(24, 4);
  display.print(ip);

  display.setCursor(3, 20); // Center the heater status number
  display.print("Heater State:");
  display.drawRect(85, 18, 40, 12, SSD1306_WHITE); // Heater status box
  display.fillRect(86, 19, 38, 10, heaterStatus == 0 ? SSD1306_BLACK : SSD1306_WHITE); // Draw heater status rectangle
  display.setTextColor(heaterStatus == 0 ? SSD1306_WHITE : SSD1306_BLACK);
  display.setCursor(102, 21); // Center the heater status number
  display.print(heaterStatus);
  display.display();
}

