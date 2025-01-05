//*******************************************************************************************
// Turn the plug on if glow_time > 0, and decrement glow_time every second
//*******************************************************************************************
void glow_plug() { 
  static long timer;

  if(glow_left < 0)
    glow_time = 0;

  if(last_glow_value!=glow_time)
  {
    glow_left = glow_time;
    last_glow_value = glow_time;

    if(glow_time != 0)
      timer = millis();
  }

  if(glow_time == 0)
    timer = millis();
  
  if(millis() - timer >= 1000)
  {
    timer = millis();
    glow_left -= 1;
  }

  if(glow_left > 0 )
  {
    if(glowplug_on == 0)
      glowplug_on = millis();

   if(glowplug_on!= 0)
   {
      if(millis()-glowplug_on < 5000) //Fades up glow plug over 5 sec
        analogWrite(glow_plug_pin, (millis()-glowplug_on)/10/2);
      else    
        analogWrite(glow_plug_pin, 255);
   }
    glow_plug_on = 1;
  }
  else
  {
    glowplug_on = 0;
    analogWrite(glow_plug_pin, 0);
    glow_plug_on = 0;
    glow_time = 0;
  }
}
