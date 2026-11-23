//*******************************************************************************************
// The web page.  Static HTML/JS - the values arrive as JSON (see web_server.ino), so there's no
// %PLACEHOLDER% templating any more (and no need to double up % signs in the CSS).
// Layout follows V4: Controls | Live Data | Runtime Settings | Advanced
//*******************************************************************************************

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Webastardo V5</title>
<style>
  body { margin:0; font-family:Arial, sans-serif; font-size:15px; background:#eee; }
  .container { display:flex; flex-wrap:wrap; padding:6px; }
  .column { flex-grow:1; flex-basis:calc(100% / 4); min-width:280px; padding:6px; box-sizing:border-box; }
  @media (max-width:1200px) { .column { flex-basis:calc(100% / 2); } }
  @media (max-width:640px)  { .column { flex-basis:100%; } }
  table { width:100%; border-collapse:collapse; background:#fff; margin-bottom:10px; }
  td, th { border:1px solid #888; padding:6px 8px; text-align:left; }
  .hdr td { background:#000066; color:#fff; font-weight:bold; font-size:16px; }
  .hdr.red td { background:#990000; }
  .hdr.blk td { background:#000; }
  .sub td { background:#ccc; font-weight:bold; }
  td.lbl { background:#ddd; width:45%; }
  .hl td { background:#ffcc33 !important; }
  input[type=number], input[type=text], input[type=password] { width:6em; padding:3px; font-size:15px; }
  input.wide { width:12em; }
  button { font-size:15px; padding:5px 12px; cursor:pointer; }
  .big { font-size:18px; padding:8px 22px; font-weight:bold; }
  .on  { background:#3a3; color:#fff; }
  .off { background:#888; color:#fff; }
  .warn { color:#b00; font-weight:bold; }
  .note { font-size:12px; color:#555; }
  #msg { position:fixed; bottom:12px; left:50%; transform:translateX(-50%); background:#333; color:#fff;
         padding:8px 16px; border-radius:6px; display:none; max-width:90%; }
  #conn { font-size:12px; }
  #errBanner { display:none; background:#b00; color:#fff; padding:10px 14px; font-weight:bold; }
  #errBanner button { margin-left:12px; }
  #errBanner .txt, #d_error { white-space:pre-line; }
  #errBanner .txt { font-weight:normal; margin:6px 0; }
  /* Toggle switch */
  .sw { position:relative; display:inline-block; width:46px; height:24px; vertical-align:middle; }
  .sw input { opacity:0; width:0; height:0; }
  .sl { position:absolute; inset:0; background:#aaa; border-radius:24px; transition:.2s; cursor:pointer; }
  .sl:before { content:""; position:absolute; height:18px; width:18px; left:3px; top:3px; background:#fff;
               border-radius:50%; transition:.2s; }
  .sw input:checked + .sl { background:#3a3; }
  .sw input:checked + .sl:before { transform:translateX(22px); }
</style>
</head>
<body>
<div id="errBanner"></div>
<div class="container">

  <!-- Column 1: Controls -->
  <div class="column">
    <table>
      <tr class="hdr"><td colspan="2">Webastardo V5 <span id="conn" style="float:right">connecting...</span></td></tr>
      <tr><td class="lbl">Heater is</td>
          <td><button id="heatBtn" class="big off" onclick="toggleHeater()">OFF</button></td></tr>
      <tr><td class="lbl">State</td><td id="modeText">-</td></tr>
      <tr><td colspan="2" id="heatNote" class="note"></td></tr>
    </table>

    <table>
      <tr class="hdr"><td colspan="2">Room</td></tr>
      <tr><td class="lbl">Room temp</td><td id="roomNow">-</td></tr>
      <tr><td class="lbl">Target</td>
          <td><input type="number" id="room_target" step="0.5" min="5" max="35" onchange="saveRoom()"> &deg;C</td></tr>
      <tr><td class="lbl">Run to Temp</td>
          <td><label class="sw"><input type="checkbox" id="run_to_temp" onchange="saveRoom()"><span class="sl"></span></label>
              <span class="note">&nbsp;switch off at target</span></td></tr>
      <tr><td colspan="2" id="roomNote" class="note"></td></tr>
    </table>

    <table>
      <tr class="hdr"><td colspan="2">Convection Fan</td></tr>
      <tr><td class="lbl">Manual ON</td>
          <td><label class="sw"><input type="checkbox" id="fanSw" onchange="cmd(this.checked?'fan_on':'fan_off')"><span class="sl"></span></label>
              &nbsp;Fan is <b id="fanState">-</b></td></tr>
      <tr><td colspan="2" class="note">Also runs automatically when the water is above Minimum</td></tr>
    </table>

    <table>
      <tr class="hdr" style="background:#0a0"><td colspan="2" style="background:#060">Fuel Purge</td></tr>
      <tr><td class="lbl">Purge is</td>
          <td><button id="purgeBtn" class="off" onclick="togglePurge()">OFF</button></td></tr>
      <tr><td colspan="2" class="note">Runs the pump fast to bleed air.  Heater must be off.  Stops after 2 minutes.</td></tr>
    </table>

    <table>
      <tr class="hdr"><td colspan="2">Maintenance</td></tr>
      <tr><td class="lbl">Clear Errors</td><td><button onclick="cmd('clear_errors')">Clear</button></td></tr>
      <tr><td class="lbl">System Test</td><td><button onclick="cmd('system_test')">Run</button></td></tr>
      <tr><td class="lbl">Bluetooth phones</td><td><button onclick="if(confirm('Forget all paired phones? They will need the PIN again.'))cmd('forget_phones')">Forget paired</button></td></tr>
      <tr class="hdr red"><td>Restart</td><td><button onclick="if(confirm('Restart the controller?'))cmd('reboot')">Restart</button></td></tr>
      <tr class="hdr blk"><td>Restore defaults</td>
          <td><button onclick="if(confirm('Erase ALL settings (including WiFi) and restart?'))cmd('factory_reset')">Factory Reset</button></td></tr>
    </table>
  </div>

  <!-- Column 2: Live Data -->
  <div class="column">
    <table>
      <tr class="hdr"><td colspan="2">Heater Live Data</td></tr>
      <tr class="sub"><td colspan="2">Power:</td></tr>
      <tr><td class="lbl">&nbsp;&nbsp;Hardware On/Off</td><td id="d_heat_in">-</td></tr>
      <tr><td class="lbl">&nbsp;&nbsp;Battery</td><td id="d_batt">-</td></tr>
      <tr class="sub"><td colspan="2">Temperatures:</td></tr>
      <tr><td class="lbl">&nbsp;&nbsp;Room</td><td id="d_room">-</td></tr>
      <tr><td class="lbl">&nbsp;&nbsp;Water</td><td id="d_water">-</td></tr>
      <tr><td class="lbl">&nbsp;&nbsp;Exhaust</td><td id="d_exhaust">-</td></tr>
      <tr><td class="lbl">&nbsp;&nbsp;Flame is</td><td id="d_flame">-</td></tr>
      <tr class="sub"><td colspan="2">Outputs:</td></tr>
      <tr><td class="lbl">&nbsp;&nbsp;Water Pump</td><td id="d_wpump">-</td></tr>
      <tr><td class="lbl">&nbsp;&nbsp;Combustion Fan</td><td id="d_fan">-</td></tr>
      <tr><td class="lbl">&nbsp;&nbsp;Fuel Pump</td><td id="d_fuel">-</td></tr>
      <tr><td class="lbl">&nbsp;&nbsp;Glow time remaining</td><td id="d_glow">-</td></tr>
      <tr><td class="lbl">&nbsp;&nbsp;Convection Fan</td><td id="d_conv">-</td></tr>
      <tr class="sub"><td colspan="2">Info:</td></tr>
      <tr><td class="lbl">&nbsp;&nbsp;Cycle Time</td><td id="d_secs">-</td></tr>
      <tr><td class="lbl">&nbsp;&nbsp;Burn Mode</td><td id="d_mode">-</td></tr>
      <tr><td class="lbl">&nbsp;&nbsp;Ignition failures</td><td id="d_fails">-</td></tr>
      <tr><td class="lbl">&nbsp;&nbsp;Error(s)</td><td id="d_error" class="warn"></td></tr>
      <tr><td class="lbl">&nbsp;&nbsp;Info</td><td id="d_info">-</td></tr>
      <tr><td class="lbl">&nbsp;&nbsp;Network</td><td id="d_net">-</td></tr>
      <tr><td class="lbl">&nbsp;&nbsp;Up time / last reset</td><td id="d_up">-</td></tr>
    </table>
  </div>

  <!-- Column 3: Runtime Settings -->
  <div class="column">
    <form id="f_run" onsubmit="return saveForm(this)">
    <table>
      <tr class="hdr red"><td colspan="3">Runtime Settings <button type="submit" style="float:right">Save Settings</button></td></tr>
      <tr class="sub"><td>Setting</td><td>Fuel (Hz)</td><td>Fan (%)</td></tr>
      <tr id="lv4"><td class="lbl">HIGH</td><td><input type="number" step="0.01" name="high_fuel"></td><td><input type="number" step="1" name="high_fan"></td></tr>
      <tr id="lv3"><td class="lbl">STEADY</td><td><input type="number" step="0.01" name="steady_fuel"></td><td><input type="number" step="1" name="steady_fan"></td></tr>
      <tr id="lv2"><td class="lbl">LOW</td><td><input type="number" step="0.01" name="low_fuel"></td><td><input type="number" step="1" name="low_fan"></td></tr>
      <tr id="lv1"><td class="lbl">IDLE</td><td><input type="number" step="0.01" name="idle_fuel"></td><td><input type="number" step="1" name="idle_fan"></td></tr>
      <tr class="sub"><td colspan="3">Water Threshold Temperatures:</td></tr>
      <tr><td class="lbl">Minimum</td><td colspan="2"><input type="number" step="1" name="heater_min"> &deg;C <span class="note">restart below</span></td></tr>
      <tr><td class="lbl">Target</td><td colspan="2"><input type="number" step="1" name="heater_target"> &deg;C</td></tr>
      <tr><td class="lbl">High</td><td colspan="2"><input type="number" step="1" name="water_warning"> &deg;C <span class="note">idle above</span></td></tr>
      <tr><td class="lbl">Over</td><td colspan="2"><input type="number" step="1" name="water_overheat"> &deg;C <span class="note">shut down</span></td></tr>
      <tr class="sub"><td colspan="3">Exhaust (PT100):</td></tr>
      <tr><td class="lbl">Flame</td><td colspan="2"><input type="number" step="1" name="flame_thresh"> &deg;C <span class="note">alight above</span></td></tr>
      <tr><td class="lbl">Start rise</td><td colspan="2"><input type="number" step="0.5" name="flame_rise"> &deg;C <span class="note">rise = lit at start-up</span></td></tr>
      <tr><td class="lbl">Overheat</td><td colspan="2"><input type="number" step="5" name="exh_overheat"> &deg;C</td></tr>
      <tr class="sub"><td colspan="3">Fuel Pump:</td></tr>
      <tr><td class="lbl">Pump Size</td><td colspan="2">
        <select name="pump_size"><option value="0">0 (silent)</option><option value="22">22</option>
        <option value="33">33</option><option value="60">60</option></select> ml</td></tr>
    </table>
    </form>
  </div>

  <!-- Column 4: Advanced -->
  <div class="column">
    <form id="f_adv" onsubmit="return saveForm(this)">
    <table>
      <tr class="hdr blk"><td colspan="3">Advanced <button type="submit" style="float:right">Save</button></td></tr>
      <tr class="sub"><td colspan="3">Power:</td></tr>
      <tr><td class="lbl">Glow plug</td><td colspan="2"><input type="number" step="0.1" name="glow_volts"> V RMS</td></tr>
      <tr><td class="lbl">Battery low</td><td colspan="2"><input type="number" step="0.1" name="low_volt"> V</td></tr>
      <tr><td class="lbl">Battery high</td><td colspan="2"><input type="number" step="0.1" name="high_volt"> V</td></tr>
      <tr class="sub"><td colspan="3">Bluetooth (phone app):</td></tr>
      <tr><td class="lbl">Pairing PIN</td><td colspan="2"><input type="number" step="1" min="100000" max="999999" name="ble_pin"> <span class="note">6 digits. Changing it forgets paired phones</span></td></tr>
      <tr class="sub"><td colspan="3">Calibration:</td></tr>
      <tr><td class="lbl">3.3V rail (TP1)</td><td colspan="2"><input type="number" step="0.001" name="rail_3v3"> V</td></tr>
      <tr><td class="lbl">Battery scale</td><td colspan="2"><input type="number" step="0.001" name="vbat_scale"> <span class="note" id="mv_vbat"></span></td></tr>
      <tr><td class="lbl">NTC R25</td><td colspan="2"><input type="number" step="1" name="ntc_r25" class="wide"> &Omega;</td></tr>
      <tr><td class="lbl">NTC Beta</td><td colspan="2"><input type="number" step="1" name="ntc_beta"></td></tr>
      <tr><td class="lbl">Water offset</td><td colspan="2"><input type="number" step="0.1" name="water_offset"> &deg;C <span class="note" id="mv_water"></span></td></tr>
      <tr><td class="lbl">Room offset</td><td colspan="2"><input type="number" step="0.1" name="room_offset"> &deg;C <span class="note" id="mv_room"></span></td></tr>
      <tr><td class="lbl">Exhaust offset</td><td colspan="2"><input type="number" step="0.1" name="exh_offset"> &deg;C <span class="note" id="mv_exh"></span></td></tr>
    </table>
    </form>
    <form id="f_wifi" onsubmit="return saveWiFi(this)">
    <table>
      <tr class="hdr"><td colspan="2">WiFi <button type="submit" style="float:right">Save &amp; Restart</button></td></tr>
      <tr><td class="lbl">Network (SSID)</td><td><input type="text" name="ssid" class="wide" maxlength="32"></td></tr>
      <tr><td class="lbl">Password</td><td><input type="password" name="pass" class="wide" maxlength="63"></td></tr>
      <tr><td colspan="2" class="note">Leave the network blank to always use the built-in Access Point.
        If it can't connect it starts the Access Point <b id="apName"></b> (password Webastardo) at http://192.168.4.1</td></tr>
    </table>
    </form>
  </div>
</div>
<div id="msg"></div>

<script>
let S = {};          // latest status
const $ = id => document.getElementById(id);

function toast(t, ms) {
  const m = $('msg'); m.textContent = t; m.style.display = 'block';
  clearTimeout(m._t); m._t = setTimeout(() => m.style.display = 'none', ms || 3000);
}

async function post(url, data) {
  try {
    const r = await fetch(url, { method:'POST', body:new URLSearchParams(data) });
    const j = await r.json();
    toast(j.msg, j.ok ? 2500 : 6000);
    return j.ok;
  } catch (e) { toast('No reply from heater'); return false; }
}

function cmd(action) { return post('/api/cmd', { action }); }
function toggleHeater() { cmd(S.web_on ? 'heater_off' : 'heater_on'); }
function togglePurge()  { cmd(S.purge ? 'purge_off' : 'purge_on'); }

function saveRoom() {
  post('/api/settings', { room_target:$('room_target').value, run_to_temp:$('run_to_temp').checked ? '1' : '0' });
}

async function saveForm(f) {
  const ok = await post('/api/settings', new FormData(f));
  if (ok) loadSettings();
  return false;
}

async function saveWiFi(f) {
  if (!confirm('Save WiFi settings and restart?')) return false;
  await post('/api/wifi', new FormData(f));
  return false;
}

async function loadSettings() {
  try {
    const s = await (await fetch('/api/settings')).json();
    for (const k in s) {
      document.querySelectorAll('[name="' + k + '"]').forEach(el => el.value = s[k]);
    }
    $('room_target').value = s.room_target;
    $('run_to_temp').checked = s.run_to_temp;
    document.querySelector('#f_wifi [name=ssid]').value = s.wifi_ssid;
    $('apName').textContent = s.ap_ssid;
  } catch (e) { setTimeout(loadSettings, 3000); }
}

const deg = v => v === null ? '-' : v.toFixed(1) + ' &deg;C';

function show(d) {
  S = d;
  const hb = $('heatBtn');
  hb.textContent = d.web_on ? 'ON' : 'OFF';
  hb.className = 'big ' + (d.web_on ? 'on' : 'off');
  $('modeText').innerHTML = d.mode_text + (d.error ? ' <span class="warn">- FAULT</span>' : '');
  let note = '';
  if (d.heat_in) note = 'Hardware HEAT input is ON - it keeps the heater on whatever the web button says.';
  if (d.bench) note = 'No 12V (bench mode) - the heater can\'t start.';
  if (d.batt_lock) note = 'Battery out of range - heater locked out until it recovers.';
  if (d.sim) note += ' SIMULATED SENSORS.';
  $('heatNote').innerHTML = note;

  $('roomNow').innerHTML = d.room_fitted ? (d.room_fault ? '<span class="warn">sensor fault</span>' : deg(d.room)) : 'No sensor fitted';
  $('roomNote').innerHTML = d.target_reached ? '<b>Target reached - heater switched off.</b> Turn it on to restart.'
                          : (!d.room_fitted && d.run_to_temp ? 'Run to Temp needs a room sensor.' : '');

  $('fanSw').checked = d.conv_manual;
  $('fanState').textContent = d.conv_fan ? 'ON' : 'OFF';
  const pb = $('purgeBtn');
  pb.textContent = d.purge ? 'ON' : 'OFF';
  pb.className = d.purge ? 'on' : 'off';

  $('d_heat_in').textContent = d.heat_in ? 'ON' : 'OFF';
  $('d_batt').innerHTML = d.batt.toFixed(2) + ' V' + (d.bench ? ' (no 12V)' : '') + (d.batt_lock ? ' <span class="warn">LOCKOUT</span>' : '');
  $('d_room').innerHTML = d.room_fitted ? deg(d.room) : 'not fitted';
  $('d_water').innerHTML = d.water_fault ? '<span class="warn">sensor fault</span>' : deg(d.water);
  $('d_exhaust').innerHTML = d.exh_fault ? '<span class="warn">sensor fault</span>' : deg(d.exhaust);
  $('d_flame').textContent = d.flame ? 'ALIGHT' : 'EXTINGUISHED';
  $('d_wpump').textContent = d.wpump + ' %';
  $('d_fan').textContent = d.fan.toFixed(1) + ' %';
  $('d_fuel').textContent = d.fuel.toFixed(2) + ' Hz (pump ' + d.pump_hz.toFixed(2) + ' Hz)';
  $('d_glow').textContent = d.glow + ' s';
  $('d_conv').textContent = d.conv_fan ? 'ON' : 'OFF';
  $('d_secs').textContent = d.secs + ' s';
  $('d_mode').textContent = d.mode + ' (' + d.mode_text + ')';
  $('d_fails').textContent = d.fails;
  $('d_error').textContent = d.error;
  $('d_info').textContent = d.info;
  $('d_net').textContent = (d.ap ? 'Access Point ' : 'WiFi ') + d.ip + (d.ap ? '' : ' (' + d.rssi + ' dBm)');
  const up = d.uptime, h = Math.floor(up / 3600), m = Math.floor(up % 3600 / 60);
  $('d_up').textContent = h + 'h ' + m + 'm / ' + d.reset + ' (V' + d.ver + ')';

  for (let i = 1; i <= 4; i++) $('lv' + i).className = (d.mode == 2 && d.level == i) ? 'hl' : '';

  $('mv_vbat').textContent = d.mv.vbat + ' mV';
  $('mv_water').textContent = d.mv.water + ' mV';
  $('mv_room').textContent = d.mv.room + ' mV';
  $('mv_exh').textContent = d.mv.exh + ' mV';

  const eb = $('errBanner');
  if (d.error) {
    eb.innerHTML = 'The heater has stopped because:<div class="txt">' + esc(d.error) + '</div>' +
                   '<button onclick="cmd(&apos;clear_errors&apos;)">Clear Errors</button>';
    eb.style.display = 'block';
  } else eb.style.display = 'none';
}

// Escape text for innerHTML (no quote characters here - they upset the Arduino builder)
const esc = t => { const el = document.createElement('div'); el.textContent = t; return el.innerHTML; };

// Live data: Server-Sent Events, falling back to polling if they aren't available
let lastData = 0;
function connect() {
  const es = new EventSource('/events');
  es.addEventListener('status', e => { lastData = Date.now(); $('conn').textContent = 'live'; show(JSON.parse(e.data)); });
  es.onerror = () => { $('conn').textContent = 'reconnecting...'; };
}
setInterval(async () => {
  if (Date.now() - lastData < 5000) return;
  $('conn').textContent = 'offline';
  try { show(await (await fetch('/api/status')).json()); lastData = Date.now() - 3000; $('conn').textContent = 'polling'; } catch (e) {}
}, 3000);

loadSettings();
connect();
</script>
</body>
</html>
)rawliteral";
