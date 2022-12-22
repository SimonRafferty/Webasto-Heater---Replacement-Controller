void WiFi_Deliver_Content() {
#ifndef BLYNK_ENABLE
  #ifdef M0_WIFI_ENABLE

    Serial.println("new client");
    // an HTTP request ends with a blank line
    bool currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the HTTP request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard HTTP response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          // output the value of each analog input pin
          client.print("StartFail#: ");
          client.print(Start_Failures);
          client.print(" | B Mode: ");
          if(burn_mode == 0)
            client.print("OFF");
          if(burn_mode == 1)
            client.print("Starting");
          if(burn_mode == 2)
            client.print("Running");
          if(burn_mode == 3)
            client.print("Shuting Down");
      
          //Serial.print(burn_mode);
          client.print(" | W_Tmp: ");
          client.print(water_temp);
          //Serial.print(" | W_Raw: ");
          //Serial.print(rawDataWater);
          client.print(" | E_Tmp: ");
          client.print(exhaust_temp);
          client.print(" | Fan_Speed_%: ");
          client.print(fan_speed);
          client.print(" | Fuel: ");
          client.print(fuel_need);
          client.print(glow_left);
          
          client.print(" | Time: ");
      
      
          
          client.print(seconds);
          client.println("<br />");
          
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);

    // close the connection:
    client.stop();
    //Serial.println("client disconnected");
  #endif
#endif
  }
