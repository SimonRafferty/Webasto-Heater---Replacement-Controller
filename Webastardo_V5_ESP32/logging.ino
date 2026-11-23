//*******************************************************************************************
// Send all the interesting data to Serial port (once a second, from loop())
//*******************************************************************************************
void logging() {
    Serial.print("BTN: ");
    Serial.print(Pushbutton());
    Serial.print(" | Web: ");
    Serial.print(Web_heater_ON);

    Serial.print(" | Bat: ");
    Serial.print(battery_voltage, 1);
    if (bench_mode) Serial.print(" BENCH");
    if (battery_lockout) Serial.print(" LOCKOUT");

    Serial.print(" | Gl: ");
    Serial.print(glow_plug_on);

    Serial.print(" | F: ");
    Serial.print(Ignition_Failures);

    Serial.print(" | Burn: ");
    Serial.print(burn);

    Serial.print(" | BMode: ");
    Serial.print(burn_mode);

    Serial.print(" | W_Tmp: ");
    if (water_sensor_fault) Serial.print("FAULT"); else Serial.print(water_temp, 1);
    Serial.print(" | Flm: ");
    Serial.print(Is_Flame());

    Serial.print(" | E_Tmp: ");
    if (exhaust_sensor_fault) Serial.print("FAULT"); else Serial.print(exhaust_temp, 1);

    Serial.print(" | Rm T: ");
    if (isnan(room_temp)) Serial.print("-"); else Serial.print(room_temp, 1);

    Serial.print(" | Fan: ");
    Serial.print(int(fan_speed));

    Serial.print(" | Fuel: ");
    Serial.print(fuel_need);

    Serial.print(" | CFan: ");
    Serial.print(conv_fan_on);

    Serial.print(" | Time: ");
    Serial.print(seconds);

    Serial.print(" | ");
    Serial.print(AP_mode ? "AP " : "");
    Serial.print(WiFi_IP());

    Serial.print(" |  ");
    Serial.print(InfoMessage);
    if (ErrorMessage != "") {
      Serial.print(" | ERR: ");
      Serial.print(ErrorMessage);
    }
    Serial.println();
}
