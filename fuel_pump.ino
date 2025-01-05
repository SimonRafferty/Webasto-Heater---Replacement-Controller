// Hardware timer
hw_timer_t * timer = NULL;

//Timer Callback - this bit actually switches the pin on & off
void IRAM_ATTR onTimer() {


  // Toggle the pin state
  if(pulseisHigh && pulseEnable){
    digitalWrite(fuel_pump_pin, 255);
  } else {
    digitalWrite(fuel_pump_pin, 0);  }
  pulseisHigh = !pulseisHigh;

  // If the pin is set high, set the timer to the pulse length
  // Otherwise, set it to the period minus the pulse length
  if(pulseEnable){
    if(pulseisHigh) {
      timerAlarmWrite(timer, pulseLength * 1000, true);      
    } else if(pulseFrequency > 0) {
      timerAlarmWrite(timer, (1000000 / pulseFrequency) - (pulseLength * 1000), true);
    }   
  }
}


//*******************************************************************************************
// Set the fuel pump rate - this uses timer driven, slow PWM in the background
// It doesn't need calling other than to change the delivery rate
//*******************************************************************************************
void Fuel_Rate(int pumpsize, float fuelneed){
double pulse_length;
static double Last_Pump_Hz = 0;
static double Last_Pump_Duty = 0;

  //Serial.print("Fuel Need = "); Serial.println(fuelneed);

  fuel_need = fuelneed; //Global used elsewhere

  //Set the pulse length required by the size of pump
  if(pumpsize == 0)
    pulse_length = 2;  //This is a special case for James's silent fuel pump jamesbrowningsmith@yahoo.co.uk
  else if(pumpsize == 22)
    pulse_length = 9.00;
  else if(pumpsize == 33)
    pulse_length = 18.00;
  else if(pumpsize == 60)
    pulse_length = 36.00;
    
  //A bigger pump delivers more fuel per pulse, adjust Pump_Hz so a smaller pump will deliver the same amount as bigger  
  double pump_division = 60.00/pumpsize;
  //Calculate Pump_Hz for the pump size
  double Pump_Hz = fuelneed/pump_division;

  //Only update if it's changed
  if(Last_Pump_Hz != Pump_Hz){
    Last_Pump_Hz = Pump_Hz;

    //Serial.print("Pump_Hz = "); Serial.println(Pump_Hz);

    setupPulse(fuel_pump_pin, Pump_Hz, pulse_length);
  }
}

//*******************************************************************************************
// Generic routing for timer based pulsing
//*******************************************************************************************
//Generic routing for pulsing
void setupPulse(int pulsePin, int frequency, int length) {
  pulseFrequency = frequency;
  pulseLength = length;

  //Wait until current pulse is finished
  while(pulseisHigh){
    vTaskDelay(1);
  }

  // Disable timer if frequency or pulse length is 0
  if(frequency == 0 || length == 0) {
    pulseEnable = false;
    digitalWrite(pulsePin, LOW);
    return;
  } else {
    pulseEnable = true;
  }

  // Check if the timer needs to be re-initialized
  if (timer == NULL) {
    // Initialize the hardware timer
    timer = timerBegin(1, 80, true); // Timer 0, prescaler 80, count up

    // Attach the handler function
    timerAttachInterrupt(timer, &onTimer, true);
  }

  // Set the timer with the specified period
  timerAlarmWrite(timer, 1000000 / frequency, true);
  timerAlarmEnable(timer);
}










/*
void Fuel_Rate(int pumpsize, float fuelneed){
double pulse_length;
static double Last_Pump_Hz = 0;
static double Last_Pump_Duty = 0;

  //Serial.print("Fuel Need = "); Serial.println(fuelneed);

  fuel_need = fuelneed; //Global used elsewhere
  //Stop the pump by disabling the timer interrupt
  if((fuelneed==0) && channelNum >= 0){
    Fuel_Pump.deleteChannel((uint8_t) channelNum);
    channelNum = -1;
    return;
  } else if(fuelneed==0) {
    //Channel has not been set - just return as pump not running
    return;
  }


  if(pumpsize == 0)
    pulse_length = 2;  //This is a special case for James's silent fuel pump jamesbrowningsmith@yahoo.co.uk
  else if(pumpsize == 22)
    pulse_length = 9.00;
  else if(pumpsize == 33)
    pulse_length = 18.00;
  else if(pumpsize == 60)
    pulse_length = 36.00;
    
  double pump_division = 60.00/pumpsize;

  double Pump_period = 1000.0/fuelneed/pump_division; //Changing this to HZ fuel need, adding a pump volume switch also to adjust for different pumps

  double Pump_Hz = 1000.0 / Pump_period;
  double Pump_Duty = pulse_length / Pump_period * 100.0;

  //Only update if it's changed
  if((Last_Pump_Duty != Pump_Duty) || (Last_Pump_Hz != Pump_Hz)){
    Last_Pump_Duty = Pump_Duty;
    Last_Pump_Hz = Pump_Hz;

    Serial.print("Pump_Hz = "); Serial.println(Pump_Hz);
    Serial.print("Pump_Duty = "); Serial.println(Pump_Duty);
    Serial.print("fuelneed = "); Serial.println(fuelneed);
    Serial.print("channelNum = "); Serial.println(channelNum);


    if(channelNum<0){
      channelNum = Fuel_Pump.setPWM(fuel_pump_pin, Pump_Hz, Pump_Duty);
      Serial.print("New channelNum = "); Serial.println(channelNum);
    }
    Fuel_Pump.modifyPWMChannel(channelNum, fuel_pump_pin, Pump_Hz, Pump_Duty);
    //Serial.print("channelNum = "); Serial.println(channelNum);
  }
}
*/

/*
void fuel_pump() {
  static unsigned long next_pulse_timer;
  static unsigned long pulse_started_off_at;
  
  if(fuel_need == 0)
  {
    next_pulse_timer = millis()+50;
    pulse_started_off_at = millis()-50;
  }

 
  int pulse_length = 9;

  if(pump_size == 0)
    pulse_length = 2;  //This is a special case for James's silent fuel pump jamesbrowningsmith@yahoo.co.uk
  else if(pump_size == 22)
    pulse_length = 9;
  else if(pump_size == 33)
    pulse_length = 18;
  else if(pump_size == 60)
    pulse_length = 36;
    
  double pump_division = 60.00/pump_size;

  delayed_period = 1000/fuel_need/pump_division; //Changing this to HZ fuel need, adding a pump volume switch also to adjust for different pumps

  if(pulse_started_off_at<=millis())
    digitalWrite(fuel_pump_pin, LOW);
  
  if(next_pulse_timer<=millis())
  {
      next_pulse_timer = delayed_period + pulse_length + millis();
      digitalWrite(fuel_pump_pin, HIGH);
      pulse_started_off_at = millis()+pulse_length;
  }
      
}
*/
