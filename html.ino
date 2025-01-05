//HTML for the largely static portion of the page, including the form-get bits
//Use an HTML Editor to update - copy everything from <!DOCTYPE html> to </html> inclusive

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Webastardo V4.1</title>
    <style>
        body {
            margin: 0;
            font-family: Arial, sans-serif;
        }

        .container {
            display: flex;
            flex-wrap: wrap;
            padding: 10px;
        }

        .column {
            flex-grow: 1;
            flex-basis: calc(100%% / 4); /* Default to 3 columns */
            padding: 10px;
        }

        .column table {
            width: 100%%;
            border-collapse: collapse;
        }

        .column table, .column th, .column td {
            border: 1px solid black;
        }

        .column th, .column td {
            padding: 8px;
            text-align: center;
        }

        /* Adjust number of columns based on the viewport width */
        @media (max-width: 1200px) { /* Fewer columns on smaller screens */
            .column {
                flex-basis: calc(100%% / 3); /* 3 columns */
            }
        }

        @media (max-width: 800px) { /* Fewer columns on smaller screens */
            .column {
                flex-basis: calc(100%% / 2); /* 2 columns */
            }
        }

        @media (max-width: 400px) {
            .column {
                flex-basis: 100%%; /* 1 column */
            }
        }
    </style>
    <script>
      setInterval(function() {
      fetch('/data')  // Replace with your ESP32 data endpoint
      .then(response => response.text())
      .then(data => {
          document.getElementById('live-data').innerHTML = data;  // Update your <div> with new data
      })
      .catch(error => console.error('Error:', error));
      }, 1000); // Refreshes every 1000 milliseconds (1 second)

    </script>
    <script type="text/javascript">
        function sendRebootRequest() {
            fetch("/doReboot")
                .then(response => {
                    if(response.ok) {
                        // Redirect after sending reboot command to avoid refresh issues
                        window.location.href = "/rebooting";
                    }
                })
                .catch(error => console.error('Error:', error));
        }
    </script>
</head>
<body>
	<div class="container">
        <!-- Column 1 -->
        <div class="column">
			
    <table>
      <!-- 3x3 table content -->
      <tr bgcolor="#000066"> 
        <td colspan="3"> 
          <div align="left"><font size="2"><b><font size="3" color="#FFFFFF">Controls</font></b></font></div>
        </td>
      </tr>
      <tr bgcolor="#CCCCCC"> 
        <td colspan="3"> 
          <form action="/get">
            <div align="left"><b><font size="3">Heater is:</font></b>&nbsp;&nbsp;&nbsp;&nbsp; 
              <input name="heater_State" value="%RUNNING%" type="hidden">
              <input value="&nbsp;&nbsp;&nbsp;%RUNNING%&nbsp;&nbsp;&nbsp;" type="submit">
            </div>
          </form>
        </td>
      </tr>
      <form action="/get">
        <tr> 
          <td bgcolor="#CCCCCC"> 
            <div align="left"><font size="2">Set target temp:</font></div>
          </td>
          <td bgcolor="#CCCCCC" colspan="2"> 
            <div align="left"><font size="2">
              <input step="1.0" name="threshold_input" value="%THRESHOLD%" required="" type="number">
              &deg;C</font></div>
          </td>
        </tr>
        <tr> 
          <td bgcolor="#CCCCCC"> 
            <div align="left"><font size="2">Stop at target?</font></div>
          </td>
          <td bgcolor="#CCCCCC"> 
            <div align="left">
              <input name="run_to_temp" type="checkbox"  %RUN_TO_TEMP% />
              <input name="run_to_temp_state" value="%RUN_TO_TEMP%" type="hidden">
            </div>
            
          </td>
          <td bgcolor="#CCCCCC"> 
            <div align="right">
              <input value="Update" type="submit" name="submit">
            </div>
          </td>
        </tr>
      </form>
      <tr bgcolor="#FFFFFF"> 
        <td  colspan="3"> 
          &nbsp;
          <br>
        </td>
      </tr>
      <form action="/get">
        <tr bgcolor="#000066"> 
          <td> 
            <div align="left"> <font size="3" color="#FFFFFF"><b>Clear Errors </b></font></div>           
          </td>
          <td>
            &nbsp;
          </td>
          <td>
      			<div align="right"> 
      				<input name="clearerrors" value="1" type="hidden"> 
            	<input size="6" value="&nbsp;&nbsp;Clear&nbsp;&nbsp;" type="submit"> 
            </div>
          </td>
        </tr>
          <tr bgcolor="#FFFFFF"> 
          <td  colspan="3"> 
          &nbsp;
          <br>
          </td>
        </tr>
      </form>
      <form action="/get">
        <tr bgcolor="#000000"> 
          <td colspan="3"> 
            <div align="left"> <font size="3" color="#FFFFFF"><b>Restore to defaults 
              (then click Restart)</b></font><br>
            </div>
			<div align="right"> 
				<input name="reset" value="1" type="hidden"> 
            	<input size="6" value="Factory Reset" type="submit"> 
            </div>
          </td>
        </tr>
      </form>
      <tr bgcolor="#FFFFFF"> 
        <td  colspan="3"> 
          &nbsp;
          <br>
        </td>
      </tr>
    </form>
        <tr bgcolor="#990000"> 
          <td> 
            <div align="left"> <font size="3" color="#FFFFFF"><b>Restart </b></font></div>           
          </td>
          <td>
            &nbsp;
          </td>
          <td>
      			<div align="right"> 
              <button onclick="sendRebootRequest()">&nbsp;&nbsp;Restart&nbsp;&nbsp;</button>
            </div>
          </td>
        </tr>
    </table>
        </div>
		<!-- Column 2 -->
		<div class="column">
            
      <div id="live-data">
          <!-- Initial content or loading message -->
          Loading live data...
      </div>
  	</div>

    <!-- Column 3 -->
    <div class="column">
    <table>
      <form action="/get">
        <tr bgcolor="#990000"> 
          <td colspan="3"> 
            <div align="left"><font size="2"><b><font size="3" color="#FFFFFF">Runtime 
              Settings </font></b></font></div>
			<div align="right"><input value="Save Settings" type="submit"></div>
          </td>
        </tr>
        <tr bgcolor="#CCCCCC"> 
          <td width="20%%"> 
            <div align="left"><b><font size="3">Setting:</font></b></div>
          </td>
          <td width="30%%"> 
            <div align="left"><b><font size="3">Fuel:</font></b></div>
          </td>
          <td width="30%%"> 
            <div align="left"><b><font size="3">Fan:</font></b></div>
          </td>
        </tr>
        <tr bgcolor="#CCCCCC"> 
          <td width="20%%"> 
            <div align="left"><font size="2">HIGH</font></div>
          </td>
          <td style="background-color: #%HIGHLIGHT1%; width="30%%"> 
            <div align="left"><font size="2"> 
              <input size="10" step="0.01" name="high_fuel_input" value="%HIGH_FUEL%"

                        required="" type="number">
              </font></div>
          </td>
          <td style="background-color: #%HIGHLIGHT1%; width="30%%"> 
            <div align="left"><font size="2"> 
              <input size="10" step="1.0" name="high_fan_input" value="%HIGH_FAN%"

                        required="" type="number">
              </font></div>
          </td>
        </tr>
        <tr bgcolor="#CCCCCC"> 
          <td width="20%%"> 
            <div align="left"><font size="2">STEADY</font></div>
          </td>
          <td style="background-color: #%HIGHLIGHT1%; width="30%%"> 
            <div align="left"><font size="2"> 
              <input size="10" step="0.01" name="steady_fuel_input" value="%STEADY_FUEL%"

                        required="" type="number">
              </font></div>
          </td>
          <td style="background-color: #%HIGHLIGHT1%; width="30%%"> 
            <div align="left"><font size="2"> 
              <input size="10" step="1.0" name="steady_fan_input" value="%STEADY_FAN%"

                        required="" type="number">
              </font></div>
          </td>
        </tr>
        <tr bgcolor="#CCCCCC"> 
          <td width="20%%"> 
            <div align="left"><font size="2">LOW</font></div>
          </td>
          <td style="background-color: #%HIGHLIGHT1%; width="30%%"> 
            <div align="left"><font size="2"> 
              <input size="10" step="0.01" name="low_fuel_input" value="%LOW_FUEL%"

                        required="" type="number">
              </font></div>
          </td>
          <td style="background-color: #%HIGHLIGHT1%; width="30%%"> 
            <div align="left"><font size="2"> 
              <input size="10" step="1.0" name="low_fan_input" value="%LOW_FAN%"

                        required="" type="number">
              </font></div>
          </td>
        </tr>
        <tr bgcolor="#CCCCCC"> 
          <td width="20%%"> 
            <div align="left"><font size="2">IDLE</font></div>
          </td>
          <td style="background-color: #%HIGHLIGHT1%; width="30%%"> 
            <div align="left"><font size="2"> 
              <input size="10" step="0.01" name="idle_fuel_input" value="%IDLE_FUEL%"

                        required="" type="number">
              </font></div>
          </td>
          <td style="background-color: #%HIGHLIGHT1%; width="30%%"> 
            <div align="left"><font size="2"> 
              <input size="10" step="1.0" name="idle_fan_input" value="%IDLE_FAN%"

                        required="" type="number">
              </font></div>
          </td>
        </tr>
        <tr bgcolor="#CCCCCC"> 
          <td colspan="3"> 
            <div align="left"><b><font size="3">Threshold Temperatures:</font></b></div>
            </td>
        </tr>
        <tr bgcolor="#CCCCCC"> 
          <td width="20%%"> 
            <div align="left"><font size="2">Minimum:</font></div>
          </td>
          <td  colspan="2" width="60%%"> 
            <div align="left"><input size="10" step="1.00" name="min_temp_input" value="%MIN_TEMP%"

                        required="" type="number">&deg;C</div>
          </td>
        </tr>
        <tr bgcolor="#CCCCCC"> 
          <td width="20%%"> 
            <div align="left"><font size="2">Target:</font></div>
          </td>
          <td  colspan="2" width="60%%"> 
            <div align="left"><input size="10" step="1.00" name="target_temp_input" value="%TARGET_TEMP%"

                        required="" type="number">&deg;C</div>
          </td>
        </tr>
        <tr bgcolor="#CCCCCC"> 
          <td width="20%%"> 
            <div align="left"><font size="2">High:</font></div>
          </td>
          <td  colspan="2" width="60%%"> 
            <div align="left"><input size="10" step="1.00" name="High_temp_input" value="%HIGH_TEMP%"

                        required="" type="number">&deg;C</div>
          </td>
        </tr>
        <tr bgcolor="#CCCCCC"> 
          <td width="20%%"> 
            <div align="left"><font size="2">Over:</font></div>
          </td>
          <td  colspan="2" width="60%%"> 
            <div align="left"><input size="10" step="1.00" name="over_temp_input" value="%OVER_TEMP%"

                        required="" type="number">&deg;C</div>
          </td>
        </tr>
        <tr bgcolor="#CCCCCC"> 
          <td width="20%%"> 
            <div align="left"><font size="2">Flame:</font></div>
          </td>
          <td  colspan="2" width="60%%"> 
            <div align="left"><input size="10" step="1.00" name="fl_threshold_input" value="%FLAME_TEMP%"

                        required="" type="number">&deg;C</div>
          </td>
        </tr>
        <tr bgcolor="#CCCCCC"> 
          <td colspan="3"> 
            <div align="left"><b><font size="3">Fuel Pump:</font></b></div>
            </td>
        </tr>
        <tr bgcolor="#CCCCCC">
          <td width="20%%" bgcolor="#CCCCCC">
            <div align="left"><font size="2">Pump Size</font></div>
          </td>
          <td  colspan="2" width="60%%"> 
            <div align="left"> 
              <input type="text" name="pump_size_input" maxlength="2" size="6" value="%PUMP_SIZE%" required="">
              ml (0, 22, 33 or 60)</div>
          </td>
        </tr>
        
      </form>
      <form action="/get">
        <tr bgcolor="#FFFFFF"> 
          <td  colspan="3"> 
          &nbsp;
          <br>
          </td>
        </tr>
        <tr bgcolor="#00FF00"> 
          <td colspan="3"> 
            <div align="left"><b><font size="3" color="#000000">Fuel 
              Purge is:</font><font size="3" color="#FFFFFF"> </font></b><input name="Purge_State" value="%PURGE%" type="hidden"> 
                      <input size="6" value="&nbsp;&nbsp;&nbsp;%PURGE%&nbsp;&nbsp;&nbsp;" type="submit">
			      </div>
          </td>
        </tr>
    </table>

  </div>

  </div>
</body>
</html>
)rawliteral";


const char redirect_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta http-equiv="Refresh" content="5; url='/'" />
</head>
  Reloading.....
</html>
)rawliteral";


