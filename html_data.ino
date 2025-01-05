//HTML for the live values, updated every second
const char data_html[] PROGMEM = R"rawliteral(
			<table>
			  <tr bgcolor="#000066"> 
				<td colspan="2"> 
				  <div align="left"><font size="2"><b><font size="3" color="#FFFFFF">Heater 
					Live Data</font></b></font></div>
				</td>
			  </tr>
			  <tr bgcolor="#CCCCCC"> 
				<td colspan="2"> 
				  <div align="left"><font size="2"><b><font size="3">Power:</font></b></font></div>
				</td>
			  </tr>
			  <tr> 
				<td bgcolor="#CCCCCC"> 
				  <div align="left"><font size="2">&nbsp;&nbsp;Hardware On/Off</font></div>
				</td>
				<td> 
				  <div align="left"><font size="2">%PUSHBUTTON%</font></div>
				</td>
			  </tr>
			  <tr> 
				<td bgcolor="#CCCCCC"> 
				  <div align="left"><font size="2">&nbsp;&nbsp;Voltage</font></div>
				</td>
				<td> 
				  <div align="left"><font size="2">%BATT_VOLTS%</font></div>
				</td>
			  </tr>
			  <tr> 
				<td bgcolor="#CCCCCC"> 
				  <div align="left"><font size="2">&nbsp;&nbsp;Current</font></div>
				</td>
				<td> 
				  <div align="left"><font size="2">%BATT_AMPS%</font></div>
				</td>
			  </tr>
			  <tr bgcolor="#CCCCCC"> 
				<td colspan="2"> 
				  <div align="left"><font size="2"><b><font color="#000000" size="3">Temperatures:</font></b></font></div>
				</td>
			  </tr>
			  <tr> 
				<td bgcolor="#CCCCCC"> 
				  <div align="left"><font size="2">&nbsp;&nbsp;Room</font></div>
				</td>
				<td> 
				  <div align="left"><font size="2">%ROOM_TEMP%&deg;C</font></div>
				</td>
			  </tr>
			  <tr> 
				<td bgcolor="#CCCCCC"> 
				  <div align="left"><font size="2">&nbsp;&nbsp;Water/Air</font></div>
				</td>
				<td> 
				  <div align="left"><font size="2">%WATER_TEMP%&deg;C</font></div>
				</td>
			  </tr>
			  <tr> 
				<td bgcolor="#CCCCCC"> 
				  <div align="left"><font size="2">&nbsp;&nbsp;Exhaust</font></div>
				</td>
				<td> 
				  <div align="left"><font size="2">%EXHAUST_TEMP%&deg;C</font></div>
				</td>
			  </tr>
			  <tr> 
				<td bgcolor="#CCCCCC"> 
				  <div align="left"><font size="2">&nbsp;&nbsp;Flame is</font></div>
				</td>
				<td> 
				  <div align="left"><font size="2">%FLAME%</font></div>
				</td>
			  </tr>
			  <tr bgcolor="#CCCCCC"> 
				<td colspan="2"> 
				  <div align="left"><font size="2"><b><font size="3">Outputs:</font></b></font></div>
				</td>
			  </tr>
			  <tr> 
				<td bgcolor="#CCCCCC"> 
				  <div align="left"><font size="2">&nbsp;&nbsp;Water Pump</font></div>
				</td>
				<td> 
				  <div align="left"><font size="2">%WATER_PUMP%</font></div>
				</td>
			  </tr>
			  <tr> 
				<td bgcolor="#CCCCCC"> 
				  <div align="left"><font size="2">&nbsp;&nbsp;Fan</font></div>
				</td>
				<td> 
				  <div align="left"><font size="2">%FAN%</font></div>
				</td>
			  </tr>
			  <tr> 
				<td bgcolor="#CCCCCC"> 
				  <div align="left"><font size="2">&nbsp;&nbsp;Fuel Pump</font></div>
				</td>
				<td> 
				  <div align="left"><font size="2">%FUEL% Hz</font></div>
				</td>
			  </tr>
			  <tr> 
				<td bgcolor="#CCCCCC"> 
				  <div align="left"><font size="2">&nbsp;&nbsp;Glow time remaining</font></div>
				</td>
				<td> 
				  <div align="left"><font size="2">%GLOW_TIME% s</font></div>
				</td>
			  </tr>
			  <tr bgcolor="#CCCCCC"> 
				<td colspan="2"> 
				  <div align="left"><font size="2"><b><font size="3">Info:</font></b></font></div>
				</td>
			  </tr>
			  <tr> 
				<td bgcolor="#CCCCCC"> 
				  <div align="left"><font size="2">&nbsp;&nbsp;Cycle Time</font></div>
				</td>
				<td> 
				  <div align="left"><font size="2">%SECONDS%s</font></div>
				</td>
			  </tr>
			  <tr> 
				<td bgcolor="#CCCCCC"> 
				  <div align="left"><font size="2">&nbsp;&nbsp;Burn Mode</font></div>
				</td>
				<td> 
				  <div align="left"><font size="2">%BURN_MODE%</font></div>
				</td>
			  </tr>
			  <tr> 
				<td bgcolor="#CCCCCC"> 
				  <div align="left"><font size="2">&nbsp;&nbsp;Error(s)</font></div>
				</td>
				<td> 
				  <div align="left"><font size="2">%ERRORMESSAGE%</font></div>
				</td>
			  </tr>
			  <tr> 
				<td bgcolor="#CCCCCC"> 
				  <div align="left"><font size="2">&nbsp;&nbsp;Info</font></div>
				</td>
				<td> 
				  <div align="left"><font size="2">%INFOMESSAGE%</font></div>
				</td>
			  </tr>
			</table>
)rawliteral";


