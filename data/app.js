// --- DOM ID mapper (v6.5: Presence and Calendar + Ping) ---
[
  'hyst','learn','lang','gmj','gft',
  't0','t1','t2','t3','t4','curT','modeBtn',
  'b_save_gas','b_save_sch','b_wifi','b_clear','b_csv',
  'devinfo','gasMJ','whum','wpress','wtemp','wwind','wd_m','we_m',
  'l_learnslope',
  'advSchedToggle', 'advEnabledToggle',
  // ÚJ ID-k a v6.5-höz
  'awayBadge', 'h_ctrl_sys', 'h_smart',
  'h_presence', 'l_presence_ips', 'pres_ips', 'p_presence_desc', 
  'l_presence_mins', 'pres_mins', 'l_presence_temp', 'pres_temp', 'b_save_presence',
  'h_calendar', 'l_cal_mode', 'calModeToggle', 'p_cal_desc',
  // ÚJ ID-k a Presence Ping-hez
  'l_presence_ping', 'presenceVal', 'presenceFill'
].forEach(id => {
  // Make sure the element exists before adding it to window
  const el = document.getElementById(id);
  if (el) window[id] = el;
});

// ---------- Dark mode ----------
let dark = (localStorage.getItem('dark') === '1');
function toggleDark(){
  dark = !dark;
  document.body.classList.toggle('dark', dark);
  localStorage.setItem('dark', dark ? '1' : '0');
  document.querySelector('button.icon[title="Day / Night"]').textContent = dark ? '🌙' : '☀️';
  buildChart(); // Rebuild chart for dark mode
}
(function(){
  document.body.classList.toggle('dark', dark);
  document.querySelector('button.icon[title="Day / Night"]').textContent = dark ? '🌙' : '☀️';
})();

// ---------- Theme Cycler (NEW) ----------
const THEMES = ['apple', 'siemens', 'nest'];
let currentTheme = localStorage.getItem('theme') || 'apple';
document.documentElement.dataset.theme = currentTheme;

function cycleTheme(){
  let idx = THEMES.indexOf(currentTheme);
  idx = (idx + 1) % THEMES.length;
  currentTheme = THEMES[idx];
  document.documentElement.dataset.theme = currentTheme;
  localStorage.setItem('theme', currentTheme);
  buildChart(); // Rebuild chart for new theme
}

// ---------- Tabs ----------
function showTab(i){
  for(let j = 0; j <= 4; j++){ // Now 5 tabs (0-4)
    const tab = document.getElementById('t' + j);
    const view = document.getElementById('v' + j);
    if(tab) tab.classList.toggle('active', j === i);
    if(view) view.classList.toggle('hidden', j !== i);
  }
}

// ---------- Clock with day/evening/night icon ----------
function updateClock(){
  const d = new Date();
  let h = d.getHours(), m = String(d.getMinutes()).padStart(2,'0');
  const icon = (h >= 6 && h < 18) ? '☀️' : ((h >= 18 && h < 22) ? '🌇' : '🌙');
  document.querySelector('.clock').textContent = `${icon} ${String(h).padStart(2,'0')}:${m}`;
}
setInterval(updateClock, 60000); updateClock();

// ---------- Helpers ----------
async function GET(u){ try{ const r = await fetch(u);
return await r.text(); } catch(e){ return null; } }
async function GETJ(u){ try{ const r = await fetch(u);
return await r.json(); } catch(e){ return null; } }
// Epoch to HH:MM helper
function epochToHHMM(epoch) {
  if (!epoch || epoch < 10000) return '--:--';
  const d = new Date(epoch * 1000);
  return `${String(d.getHours()).padStart(2,'0')}:${String(d.getMinutes()).padStart(2,'0')}`;
}
// Minutes to HH:MM helper
function minsToHHMM(mins) {
  const h = Math.floor(mins / 60);
  const m = mins % 60;
  return `${String(h).padStart(2,'0')}:${String(m).padStart(2,'0')}`;
}

// ---------- I18N (Standard Case) ----------
const I18N_DAYS = {
  hu: ["Vasárnap", "Hétfő", "Kedd", "Szerda", "Csütörtök", "Péntek", "Szombat"],
  en: ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"]
};

const I18N = {
  hu: { t0: "Termosztát", t1: "Rendszer", t2: "Időjárás", t3: "Diagnosztika", t4: "Ütemterv",
        ctl: "Fűtés Vezérlés", cur: "Aktuális Hőmérséklet", mode: "Mód",
        sch: "Ütemezés (hétköznap / hétvége)", wdm: "Hétköznap reggel", wde: "Hétköznap este", wem: "Hétvége reggel", wee: "Hétvége este",
        save_sch: "Mentés", sys: "Rendszer & Stat",
        ctrl_sys: "Szabályozás", hyst: "Hiszterézis", learn: "Tanuló mód", lang: "Nyelv",
        wifi: "Wi-Fi újraindítás", wifi_btn: "Portál", gas: "Gáz", g_mj: "Fűtőérték", g_ft: "Ár", today: "Ma", save_gas: "Mentés",
        log: "Hőmérséklet Napló (48h/15m)", loghint: "CSV letöltés, napló törlés.", csv: "CSV", clear: "Törlés",
        dev: "Eszközinfo", weather: "Időjárás", city: "Város", key: "API kulcs", show: "Mutat", save_w: "Mentés",
        nodata: "Nincs adat. Ellenőrizd az API kulcsot.", on: "BE", off: "KI", auto: "Auto", manual: "Kézi", 
        learnslope: "Tanulási meredekség (°C/óra)",
        // v6.4
        h_adv_notice: "Haladó ütemterv aktív.", p_adv_notice: "Szerkesztés az 'Ütemterv' fülön.",
        h_adv_sched: "Haladó Ütemterv (7 napos)", p_adv_desc: "Max 21 pont adható hozzá. A termosztát a legutóbbi múltbeli eseményt használja.",
        h_adv_add: "Új pont felvétele", l_adv_day: "Hét napja", l_adv_time: "Időpont", l_adv_temp: "Célhőm.", l_adv_enabled: "Engedélyezve",
        b_adv_save: "Új pont mentése", h_adv_points: "Jelenlegi pontok", l_adv_sched_toggle: "Haladó ütemterv",
        h_learn_diag: "Okos Tanulás Diagnosztika", l_gain_rate: "Kazán Felfűtési Sebesség", l_loss_coeff: "Épület Hatékonysága",
        l_loss_unit: "(Hőveszt. együttható)", p_learn_desc: "A 'Felfűtési Sebesség' a kazánod ereje. A 'Hatékonyság' a ház hűlési sebessége (alacsonyabb a jobb).",
        del_point: "Időpont törlése", del_confirm: "Biztosan törlöd ezt az időpontot?",
        preheat: "ELŐFŰTÉS",
        // v6.5 (Presence & Calendar)
        away: "TÁVOLLÉT",
        h_smart: "Okos Funkciók",
        h_presence: "Jelenlét Érzékelés (Away Mode)",
        l_presence_ips: "Figyelt IP Címek",
        p_presence_desc: "Vesszővel elválasztva (pl. 192.168.1.10). Ha egyik sem elérhető, a 'Távollét' mód bekapcsol.",
        l_presence_mins: "Távollét időtúllépés (perc)",
        l_presence_temp: "Távollét hőfok (°C)",
        b_save_presence: "Jelenlét Mentése",
        h_calendar: "Naptár Integráció",
        l_cal_mode: "Google Naptár mód",
        p_cal_desc: "Engedélyezi a Google Naptár felülbírálást. A Google Scriptet be kell állítani!",
        // Presence Ping
        l_presence_ping: "Jelenlét Ping",
        ping_ok: "Látva",
        ping_fail: "Nincs látva"
      },
  en: { t0: "Thermostat", t1: "System", t2: "Weather", t3: "Diagnostics", t4: "Schedule",
        ctl: "Heating Control", cur: "Current Temperature", mode: "Mode",
        sch: "Schedule (weekday / weekend)", wdm: "Weekday morning", wde: "Weekday evening", wem: "Weekend morning", wee: "Weekend evening",
        save_sch: "Save", sys: "System & Stats",
        ctrl_sys: "Control", hyst: "Hysteresis", learn: "Smart learning", lang: "Language",
        wifi: "Wi-Fi reconfigure", wifi_btn: "Portal", gas: "Gas", g_mj: "Heating value", g_ft: "Cost", today: "Today", save_gas: "Save",
        log: "Temperature Log (48h/15m)", loghint: "You can download CSV or clear log.", csv: "CSV", clear: "Clear",
        dev: "Device info", weather: "Weather", city: "City", key: "API key", show: "Show", save_w: "Save",
        nodata: "No data. Check API key.", on: "ON", off: "OFF", auto: "Auto", manual: "Manual", 
        learnslope: "Learning Slope (°C/h)",
        // v6.4
        h_adv_notice: "Advanced Scheduling is Active.", p_adv_notice: "Edit points on the 'Schedule' tab.",
        h_adv_sched: "Advanced Schedule (7-day)", p_adv_desc: "Add up to 21 schedule points. The thermostat will use the target from the last event that has passed.",
        h_adv_add: "Add New Point", l_adv_day: "Day of Week", l_adv_time: "Time", l_adv_temp: "Target Temp", l_adv_enabled: "Enabled",
        b_adv_save: "Save New Point", h_adv_points: "Current Points", l_adv_sched_toggle: "Advanced Schedule",
        h_learn_diag: "Smart Learning Diagnostics", l_gain_rate: "Boiler Heat-Up Rate", l_loss_coeff: "Building Efficiency",
        l_loss_unit: "(Loss Coeff)", p_learn_desc: "'Heat-Up Rate' is your boiler's power. 'Efficiency' shows how fast your building loses heat (lower is better).",
        del_point: "Delete Point", del_confirm: "Are you sure you want to delete this schedule point?",
        preheat: "PRE-HEAT",
        // v6.5 (Presence & Calendar)
        away: "AWAY",
        h_smart: "Smart Features",
        h_presence: "Smart Presence (Away Mode)",
        l_presence_ips: "Monitored IP Addresses",
        p_presence_desc: "Comma-separated (e.g., 192.168.1.10). If none are reachable, 'Away Mode' activates.",
        l_presence_mins: "Away Timeout (minutes)",
        l_presence_temp: "Away Temperature (°C)",
        b_save_presence: "Save Presence",
        h_calendar: "Calendar Integration",
        l_cal_mode: "Google Calendar Mode",
        p_cal_desc: "Enables Google Calendar override. Requires Google Script setup!",
        // NEW: Presence Ping
        l_presence_ping: "Presence Ping",
        ping_ok: "Seen",
        ping_fail: "Not Seen"
      }
};

function applyLang(L){
  const t = I18N[L] || I18N.en;
  t0.textContent = t.t0; t1.textContent = t.t1; t2.textContent = t.t2;
  if(window.t3){ t3.textContent = t.t3; }
  if(window.t4){ t4.textContent = t.t4; } 
  h_ctl.textContent = t.ctl;
  h_sch.textContent = t.sch;
  l_wdm.textContent = t.wdm; l_wde.textContent = t.wde; l_wem.textContent = t.wem; l_wee.textContent = t.wee;
  b_save_sch.textContent = t.save_sch;
  h_sys.textContent = t.sys;
  h_ctrl_sys.textContent = t.ctrl_sys; l_hyst.textContent = t.hyst; 
  l_learn.textContent = t.learn; l_lang.textContent = t.lang; l_wifi.textContent = t.wifi; b_wifi.textContent = t.wifi_btn;
  h_gas.textContent = t.gas; l_gmj.textContent = t.g_mj; l_gft.textContent = t.g_ft; l_today.textContent = t.today; b_save_gas.textContent = t.save_gas;
  h_log.textContent = t.log;
  p_loghint.textContent = t.loghint; b_csv.textContent = t.csv; b_clear.textContent = t.clear;
  h_dev.textContent = t.dev; h_weather.textContent = t.weather; l_city.textContent = t.city;
  l_key.textContent = t.key; a_show.textContent = t.show; b_save_w.textContent = t.save_w; p_nodata.textContent = t.nodata;
  if(window.l_learnslope) { l_learnslope.textContent = t.learnslope; }
  
  // v6.4
  if(document.getElementById('h_adv_notice')) {
    document.getElementById('h_adv_notice').textContent = t.h_adv_notice;
    document.getElementById('p_adv_notice').textContent = t.p_adv_notice;
    document.getElementById('h_adv_sched').textContent = t.h_adv_sched;
    document.getElementById('p_adv_desc').textContent = t.p_adv_desc;
    document.getElementById('h_adv_add').textContent = t.h_adv_add;
    document.getElementById('l_adv_day').textContent = t.l_adv_day;
    document.getElementById('l_adv_time').textContent = t.l_adv_time;
    document.getElementById('l_adv_temp').textContent = t.l_adv_temp;
    document.getElementById('l_adv_enabled').textContent = t.l_adv_enabled;
    document.getElementById('b_adv_save').textContent = t.b_adv_save;
    document.getElementById('h_adv_points').textContent = t.h_adv_points;
    document.getElementById('l_adv_sched_toggle').textContent = t.l_adv_sched_toggle;
    document.getElementById('h_learn_diag').textContent = t.h_learn_diag;
    document.getElementById('l_gain_rate').textContent = t.l_gain_rate;
    document.getElementById('l_loss_coeff').textContent = t.l_loss_coeff;
    document.getElementById('l_loss_unit').textContent = t.l_loss_unit;
    document.getElementById('p_learn_desc').textContent = t.p_learn_desc;
  }
  
  // v6.5 (Presence & Calendar)
  if(window.h_smart) {
    h_smart.textContent = t.h_smart;
    h_presence.textContent = t.h_presence;
    l_presence_ips.textContent = t.l_presence_ips;
    p_presence_desc.textContent = t.p_presence_desc;
    l_presence_mins.textContent = t.l_presence_mins;
    l_presence_temp.textContent = t.l_presence_temp;
    b_save_presence.textContent = t.b_save_presence;
    h_calendar.textContent = t.h_calendar;
    l_cal_mode.textContent = t.l_cal_mode;
    p_cal_desc.textContent = t.p_cal_desc;
  }

  // Presence Ping
  if(window.l_presence_ping) {
    l_presence_ping.textContent = t.l_presence_ping;
  }
}
async function setLang(L){
  localStorage.setItem('lang', L);
  document.documentElement.lang = L;
  applyLang(L);
  await GET('/save_lang?lang=' + encodeURIComponent(L));
  await refresh();
  await buildChart();
}

// --- Animated Weather Icons ---
function iconSVG(kind){
  const d = kind && kind.endsWith('d');
  const baseSun   = `<div class="sun"></div>`;
  const baseCloud = `<div class="cloud float"></div>`;
  const rain = `${[...Array(6)].map((_, i) => `<div class='drop' style='left:${30 + i*8}px; animation-delay:${i*0.12}s'></div>`).join('')}`;
  const snow = `${[...Array(6)].map((_, i) => `<div class='flake' style='left:${28 + i*12}px; top:64px; animation-delay:${i*0.15}s'></div>`).join('')}`;
  const fog  = `${[1,2,3].map(() => `<div class='bar'></div>`).join('')}`;
  const bolt = `<div class='lightning'></div>`;
  switch(kind ? kind.slice(0,2) : '01'){
    case '01': return d ? baseSun : `${baseCloud}`;
    case '02': return `${baseSun}${baseCloud}`;
    case '03': return `${baseCloud}`;
    case '04': return `${baseCloud}${baseCloud}`;
    case '09': return `${baseCloud}<div class='rain'>${rain}</div>`;
    case '10': return d ? `${baseSun}${baseCloud}<div class='rain'>${rain}</div>` : `${baseCloud}<div class='rain'>${rain}</div>`;
    case '11': return `${baseCloud}${bolt}`;
    case '13': return `${baseCloud}<div class='snow'>${snow}</div>`;
    case '50': return `${fog}`;
    default:   return `${baseSun}`;
  }
}

// ---------- Weather helpers (UPDATED) ----------
function renderWeatherIcon(code){
  const w = document.getElementById('wiconWrap');
  if(w) w.innerHTML = iconSVG(code || '01d');
}
function fillWeather(w){
  const ok = w && w.ok;
  wcity.textContent = w?.city || '---';
  wtemp.textContent = (ok ? w.temp : 0).toFixed(1);
  wdesc.textContent = ok ? (w.desc || 'N/A') : (document.documentElement.lang === 'hu' ? I18N.hu.nodata : I18N.en.nodata);
  document.getElementById('wfeels').textContent = ok ? (w.feels || 0).toFixed(1) : '--';
  document.getElementById('whum').textContent = ok ? (w.hum || 0) : '--';
  document.getElementById('wpress').textContent = ok ? (w.press || 0) : '--';
  document.getElementById('wwind').textContent = ok ? (w.wind || 0) : '--';
  document.getElementById('wminmax').textContent = (ok ? `${(w.temp_min||0).toFixed(1)} / ${(w.temp_max||0).toFixed(1)}` : '-- / --');
  document.getElementById('wsunrise').textContent = epochToHHMM(w.sunrise);
  document.getElementById('wsunset').textContent = epochToHHMM(w.sunset);
  renderWeatherIcon(ok ? w.icon : '01d');
}
async function saveWeather(){
  await GET('/weathersave?city=' + encodeURIComponent(city.value) + '&key=' + encodeURIComponent(apikey.value));
  const w = await GETJ('/weather'); if(w) fillWeather(w);
}
function toggleKey(){ const i = apikey;
  i.type = (i.type === 'password' ? 'text' : 'password');
}

// ... (Pro Controls helpers unchanged) ...
function setBadges(p){
  const bb = document.getElementById('boostBadge');
  const eb = document.getElementById('ecoBadge');
  if(p.boost === true){ bb.classList.remove('hidden'); } else { bb.classList.add('hidden'); }
  if(p.eco === true && p.isNight === true){ eb.classList.remove('hidden'); } else { eb.classList.add('hidden'); }
}
function setBoostLeft(sec){
  const el = document.getElementById('boostLeft');
  if(sec > 0){ el.classList.remove('hidden'); el.textContent = `BOOST LEFT: ${Math.floor(sec/60)}M ${sec % 60}S`; }
  else { el.classList.add('hidden'); }
}

// ---------- Status & forms ----------
async function refresh(){
  const s = await GETJ('/status');
  if(!s) return;

  // Sensor error
  const sensorErrorEl = document.getElementById('sensorError');
  if (s.temp === null) {
    curT.textContent = '---';
    if (sensorErrorEl) sensorErrorEl.classList.remove('hidden');
  } else {
    curT.textContent = (s.temp || 0).toFixed(1);
    if (sensorErrorEl) sensorErrorEl.classList.add('hidden');
  }

  const L = localStorage.getItem('lang') || 'hu';
  const t = I18N[L] || I18N.en;
  
  // Heating badge
  const b = document.getElementById('heatBadge');
  b.textContent = s.heat ? t.on : t.off;
  b.className = 'badge ' + (s.heat ? 'on' : 'off');
  
  // Heating Glow
  document.querySelector('.main-display').classList.toggle('heating-active', s.heat);
  modeBtn.textContent = s.manual ? t.manual : t.auto;
  
  // Active target line
  if(s && typeof s.tgt !== 'undefined' && s.prog){
    const Lc = (localStorage.getItem('lang') || document.documentElement.lang || 'en');
    const targetEl = document.getElementById('stat_target_main');
    const programEl = document.getElementById('stat_program_main');
    
    if(targetEl) targetEl.textContent = Number(s.tgt).toFixed(1);
    
    if(programEl) {
      let progText = s.prog;
      if (Lc === 'hu') {
        progText = s.prog.replace('Weekday','Hétköznap').replace('Weekend','Hétvége').replace('morning','reggel').replace('evening','este')
                         .replace('Manual (Eco)', 'Kézi (Eco)')
                         .replace('Manual', 'Kézi')
                         .replace('Boost (Eco)', 'Boost (Eco)')
                         .replace('Boost', 'Boost')
                         .replace('Advanced', 'Haladó')
                         .replace('Away Mode', t.away || 'Távollét')
                         .replace('Calendar', t.h_calendar || 'Naptár');
      } else {
        // Angol fordítás (ha a backend más nevet adna)
        progText = s.prog.replace('Away Mode', t.away || 'Away Mode')
                         .replace('Calendar', t.h_calendar || 'Calendar');
      }
      programEl.textContent = progText;
    }
    
    // Pre-Heat Badge Management
    const preheatBadge = document.getElementById('preheatBadge');
    if(preheatBadge) {
      preheatBadge.classList.toggle('hidden', s.preheat !== true);
      preheatBadge.textContent = t.preheat || 'PRE-HEAT';
    }
    
    // Away Badge Management
    const awayBadge = document.getElementById('awayBadge');
    if(awayBadge) {
      awayBadge.classList.toggle('hidden', s.away !== true);
      awayBadge.textContent = t.away || 'AWAY';
    }
  }

  // badges
  setBadges({ boost: s.boost, eco: s.eco, isNight: s.isNight });
  setBoostLeft(s.boostLeft || 0);
  
  // manual block visibility
  const mb = document.getElementById('manualBlock');
  if(s.manual){
    mb.classList.remove('hidden');
    mtSlider.value = (s.mt !== undefined && s.mt !== null) ? s.mt : mtSlider.value;
    document.getElementById('mtVal').textContent = Number(s.mt || 0).toFixed(1);
  } else {
    mb.classList.add('hidden');
  }

  // --- Initial Load (runs only once) ---
  if(!refresh._init){
    const sc = await GETJ('/sched');
    if(sc){
      wd_m.value = sc.wd_m || '06:00'; wd_e.value = sc.wd_e || '20:00';
      we_m.value = sc.we_m || '07:00'; we_e.value = sc.we_e || '22:00';
      wd_tm.value = Number(sc.wd_tm || 20).toFixed(1); wd_te.value = Number(sc.wd_te || 20).toFixed(1);
      we_tm.value = Number(sc.we_tm || 20).toFixed(1); we_te.value = Number(sc.we_te || 20).toFixed(1);
    }
    const st = await GETJ('/settings');
    if(st){
      hyst.value = Number(st.hyst || 0.3).toFixed(1);
      learn.value = st.learnMode ? '1' : '0';
      const stored = localStorage.getItem('lang') || st.lang || 'hu';
      document.documentElement.lang = stored; applyLang(stored);
      document.getElementById('langToggle').classList.toggle('on', stored === 'en');
      g_mj.value = Number(st.gmj || 35.91).toFixed(2);
      g_ft.value = Number(st.gft || 2.865).toFixed(3);
      
      // Set Advanced Schedule toggle and UI state
      toggleAdvSchedUI(st.advSchedEn);
      document.getElementById('advSchedToggle').classList.toggle('on', st.advSchedEn);
      
      // ÚJ: Presence & Calendar beállítások betöltése
      pres_ips.value = st.pres_ips || '';
      pres_mins.value = st.pres_mins || 30;
      pres_temp.value = Number(st.pres_temp || 16).toFixed(1);
      calModeToggle.classList.toggle('on', st.cal_mode);
    }
    const di = await GET('/devinfo'); if(di) devinfo.innerHTML = di;
    const eg = await GETJ('/energy');
    if(eg){
      gasMJ.textContent = Number(eg.mj || 0).toFixed(1);
      if(document.getElementById('gasM3')) document.getElementById('gasM3').textContent = Number(eg.m3 || 0).toFixed(2);
      gasFt.textContent = Number(eg.cost || 0).toFixed(0);
    }
    const w = await GETJ('/weather');
    if(w){ fillWeather(w);
      city.value = w.city || 'Bekescsaba'; if(w.keyset) apikey.placeholder = '********';
    }
    
    await loadAdvSched(); // Load the advanced schedule list
    await buildChart();
    refresh._init = true;
  }
}

// ---------- Language toggle ----------
document.getElementById('langToggle').addEventListener('click', async () => {
  const on = document.getElementById('langToggle').classList.toggle('on');
  const L = on ? 'en' : 'hu';
  await setLang(L);
});
async function toggleMode(){ await GET('/toggle'); refresh(); }
heatBadge.addEventListener('click', async () => {
  const s = await GETJ('/status'); if(!s || !s.manual) return;
  await GET('/manual_heat?on=' + (s.heat ? 0 : 1)); refresh();
});
async function saveSched(){
  const q = new URLSearchParams({
    wd_m: wd_m.value, wd_e: wd_e.value, we_m: we_m.value, we_e: we_e.value,
    wd_tm: wd_tm.value, wd_te: wd_te.value, we_tm: we_tm.value, we_te: we_te.value
  }).toString();
  await GET('/save_sched?' + q);
}
hyst.addEventListener('change', async e => { await GET('/save?hyst=' + encodeURIComponent(e.target.value)); });
learn.addEventListener('change', async e => { await GET('/save?learn=' + (e.target.value === '1' ? 1 : 0)); });
// Manual target slider
const mtSlider = document.getElementById('mtSlider');
mtSlider?.addEventListener('input', (e) => {
  document.getElementById('mtVal').textContent = Number(e.target.value).toFixed(1);
});
mtSlider?.addEventListener('change', async (e) => { // Only send on release
  await GET('/pro_set?mt=' + encodeURIComponent(e.target.value));
});

async function toggleBoost(){
  const s = await GETJ('/status'); if(s && s.boost){ await GET('/boost?on=0');
  } else { await GET('/boost?on=1'); }
  refresh();
}
async function toggleEco(){
  const s = await GETJ('/status'); if(s && s.eco){ await GET('/eco?on=0');
  } else { await GET('/eco?on=1'); }
  refresh();
}

// ---------- Wi-Fi / Gas / Log ----------
async function wifiReset(){ await GET('/wifi_reset'); alert('Rebooting to Wi-Fi portal...');
}
async function clearLog(){ await GET('/clearlog'); await buildChart(); }
async function saveGas(){ await GET('/gassave?mj=' + g_mj.value + '&ft=' + g_ft.value);
}

// ---------- Chart.js (Theme Aware & FIXED) ----------
let chart;
async function buildChart(){
  const data = await GETJ('/logdata') || [];
  if(!data || data.length === 0){ 
    if(chart) chart.destroy();
    return;
  }
  
  const labels = data.map(p => {
    if(!p.x || p.x < 1577836800) return null;
    return new Date(p.x * 1000);
  }).filter(v => v);
  const series = data.map(p => p.t);
  
  if(chart){ chart.destroy(); }
  const ctx = document.getElementById('tempChart').getContext('2d');
  const locale = document.documentElement.lang || 'hu';
  const timeLabel = locale === 'hu' ? 'Idő (15 perces minták)' : 'Time (15 min samples)';
  
  const style = getComputedStyle(document.documentElement);
  
  const isDark = document.body.classList.contains('dark');
  const tickColor = isDark ? '#e0e0e0' : '#333';
  const gridColor = style.getPropertyValue('--chart-grid-color').trim();

  Chart.defaults.font.family = style.getPropertyValue('--chart-font').trim();
  Chart.defaults.font.size = parseInt(style.getPropertyValue('--chart-font-size').trim());
  Chart.defaults.color = tickColor;

  const chartBorderColor = style.getPropertyValue('--chart-line-color').trim();
  const chartTension = parseFloat(style.getPropertyValue('--chart-tension').trim());
  const chartStepped = (style.getPropertyValue('--chart-stepped').trim() === 'true');

  chart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: labels,
      datasets: [{
        label: '(°C)',
        data: series,
        tension: chartTension,
        pointRadius: 0,
        borderWidth: 2,
        borderColor: chartBorderColor,
        backgroundColor: chartBorderColor,
        stepped: chartStepped,
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      interaction: { mode: 'index', intersect: false },
      plugins: { legend: { display: false } },
      scales: {
        x: {
          type: 'time',
          time: {
            unit: 'minute',
            stepSize: 15,
            tooltipFormat: 'yyyy-MM-dd HH:mm:ss',
            displayFormats: { minute: 'HH:mm', hour: 'HH:mm' }
          },
          ticks: { autoSkip: true, maxTicksLimit: 8, color: tickColor },
          title: { display: true, text: timeLabel, color: tickColor },
          grid: { color: gridColor }
        },
        y: {
          beginAtZero: false,
          title: { display: true, text: '(°C)', color: tickColor },
          ticks: { color: tickColor },
          grid: { color: gridColor }
        }
      }
    }
  });
}

// --- IMPROVED REFRESH LOGIC ---
setInterval(refresh, 5000);
setInterval(buildChart, 60000);
setInterval(async () => {
  const s = await GETJ('/status');
  if(s) { setBadges({ boost: s.boost, eco: s.eco, isNight: s.isNight }); setBoostLeft(s.boostLeft || 0); }
}, 1000);
setInterval(fetchDiag, 10000);

// ---------- Advanced Scheduler UI Logic ----------
document.getElementById('advSchedToggle').addEventListener('click', async (e) => {
  const on = e.currentTarget.classList.toggle('on');
  await GET('/adv_sched_enable?enable=' + (on ? '1' : '0'));
  toggleAdvSchedUI(on);
});

document.getElementById('advEnabledToggle').addEventListener('click', (e) => {
  e.currentTarget.classList.toggle('on');
});

function toggleAdvSchedUI(isAdvancedOn) {
  document.getElementById('t4').classList.toggle('hidden', !isAdvancedOn);
  document.getElementById('simpleSchedBlock').classList.toggle('hidden', isAdvancedOn);
  document.getElementById('advancedSchedNotice').classList.toggle('hidden', !isAdvancedOn);
  // If we hide the active tab, go back to tab 0
  if (isAdvancedOn === false && document.getElementById('t4').classList.contains('active')) {
    showTab(0);
  }
}

async function loadAdvSched() {
  const listEl = document.getElementById('advSchedList');
  if (!listEl) return;
  listEl.innerHTML = ''; // Clear list
  
  const data = await GETJ('/adv_sched_get');
  if (!data) return;
  
  // Sort data by day, then time
  data.sort((a, b) => {
    if (a.day !== b.day) return a.day - b.day;
    return a.time - b.time;
  });
  
  const L = localStorage.getItem('lang') || 'hu';
  const t = I18N[L] || I18N.en;
  
  data.forEach(p => {
    const dayName = I18N_DAYS[L][p.day] || '---';
    const timeStr = minsToHHMM(p.time);
    
    const item = document.createElement('div');
    item.className = 'sched-item';
    item.innerHTML = `
      <div>
        <div class="sched-item-time">${timeStr}</div>
        <div class="sched-item-day">${dayName}</div>
      </div>
      <div class="sched-item-temp">${p.temp.toFixed(1)} °C</div>
      <div class="sched-item-controls">
        <div class="toggle ${p.enabled ? 'on' : ''}" onclick="toggleAdvPoint(this, ${p.idx})"><div class="knob"></div></div>
        <button class="btn-delete" onclick="deleteAdvPoint(event, ${p.idx})" title="${t.del_point || 'Delete Point'}">🗑️</button>
      </div>
    `;
    listEl.appendChild(item);
  });
}

// This function is called by the toggle's onclick
async function toggleAdvPoint(el, idx) {
  const on = el.classList.toggle('on');
  const data = await GETJ('/adv_sched_get'); // Re-fetch to ensure data is fresh
  if (!data) return;
  const item = data.find(p => p.idx === idx);
  if (!item) return;
  
  const q = new URLSearchParams({
    idx: item.idx,
    day: item.day,
    time: item.time,
    temp: item.temp,
    enabled: (on ? 'true' : 'false')
  }).toString();
  await GET('/adv_sched_save?' + q);
}

//  Delete Point function
async function deleteAdvPoint(e, idx) {
  e.stopPropagation(); // Stop click from bubbling up to the toggle
  const L = localStorage.getItem('lang') || 'hu';
  const t = I18N[L] || I18N.en;
  const confirmText = t.del_confirm || "Are you sure you want to delete this schedule point?";
  
  if (!confirm(confirmText)) {
    return;
  }
  await GET('/adv_sched_delete?idx=' + idx);
  await loadAdvSched(); // Refresh the list
}


async function saveAdvPoint() {
  const day = document.getElementById('advDay').value;
  const time = document.getElementById('advTime').value.split(':');
  const timeMins = parseInt(time[0]) * 60 + parseInt(time[1]);
  const temp = document.getElementById('advTemp').value;
  const enabled = document.getElementById('advEnabledToggle').classList.contains('on');
  
  const q = new URLSearchParams({
    day: day,
    time: timeMins,
    temp: temp,
    enabled: (enabled ? 'true' : 'false')
  }).toString();
  
  await GET('/adv_sched_save?' + q);
  await loadAdvSched(); // Reload the list
}

// --- Presence and Calendar save functions ---
async function savePresence() {
  const q = new URLSearchParams({
    ips: pres_ips.value,
    mins: pres_mins.value,
    temp: pres_temp.value
  }).toString();
  await GET('/save_presence?' + q);
  alert('Presence settings saved!');
}

async function toggleCalendarMode(e) {
  const on = e.currentTarget.classList.toggle('on');
  await GET('/save_calendar?enable=' + (on ? '1' : '0'));
  await refresh(); // Refresh to show new status
}

// ---------- Diagnostics polling (UPDATED for Gauges AND Presence Ping) ----------
async function fetchDiag(){
  const d = await GETJ('/diag.json');
  if(!d) return;
  
  // Device + uptime
  if(d.device) devName.textContent = d.device;
  const s = d.uptime_s || 0;
  const hh = Math.floor(s / 3600), mm = Math.floor((s % 3600) / 60), ss = s % 60;
  uptime.textContent = `${hh}h ${mm}m ${ss}s`;

  // RSSI
  const r = d.rssi || -100;
  const rPct = Math.max(0, Math.min(100, Math.round((r + 100) * 100 / 70)));
  rssiPct.textContent = rPct + '%';
  rssiFill.style.width = rPct + '%';

  // NTP
  ntpVal.textContent = d.ntp_sync ? 'OK' : 'NO';
  ntpFill.style.width = d.ntp_sync ? '100%' : '10%';

  // --- Presence Ping Status ---
  const L = localStorage.getItem('lang') || 'hu';
  const t = I18N[L] || I18N.en;
  const last_s = d.presence_last_s; // Get new value from C++
  
  if (typeof last_s !== 'undefined' && window.presenceVal) {
    // Get the timeout setting from the input field
    const awayTimeoutMins = document.getElementById('pres_mins')?.value || 30;
    const awayTimeoutSecs = awayTimeoutMins * 60;

    if (last_s < (awayTimeoutSecs + 60)) { // Show "OK" status up to 1 min past timeout
      // Bar goes from 100% (0s) down to 10% (at timeout)
      const pct = Math.max(10, 100 - (last_s / awayTimeoutSecs) * 90);
      presenceFill.style.width = pct + '%';
      presenceFill.style.background = ''; // Use default CSS gradient
      
      if (last_s < 120) { // < 2 perc
        presenceVal.textContent = `${t.ping_ok} ${last_s}s`;
      } else { // > 2 perc
        presenceVal.textContent = `${t.ping_ok} ${Math.floor(last_s / 60)}m`;
      }
    } else {
      // We haven't seen the device in a long time
      presenceFill.style.width = '10%';
      presenceFill.style.background = '#d90429'; // Override to red
      presenceVal.textContent = `${t.ping_fail} ${Math.floor(last_s / 60)}m`;
    }
  }
  // --- END: Presence Ping ---

  // CPU + Heap
  const cpu = Math.max(0, Math.min(100, Math.round(d.cpu_pct || 0)));
  cpuVal.textContent = cpu + '%';
  cpuFill.style.width = cpu + '%';
  const heap = Math.max(0, Math.min(100, Math.round(d.heap_used_pct || 0)));
  heapVal.textContent = heap + '%';
  heapFill.style.width = heap + '%';
  
  // --- Visual Gauges ---
  const gain = d.learnGainRate || 0;
  const loss = d.learnLossCoeff || 0;
  
  // Show new gauges if data is valid, hide old bar
  if (!isNaN(gain) && !isNaN(loss) && gain > 0) {
    document.getElementById('diagLearnOld').classList.add('hidden');
    document.getElementById('diagLearnNew').classList.remove('hidden');
    
    // Update Gain Gauge (0 to 2.5 °C/h)
    // Map 0-180 degrees
    const gainPct = Math.max(0, Math.min(100, (gain / 2.5) * 100));
    const gainDeg = gainPct * 1.8;
    document.getElementById('gaugeFillGain').style.transform = `rotate(${gainDeg}deg)`;
    document.getElementById('gaugeValGain').textContent = gain.toFixed(2);
    
    // Update Loss Gauge (0 to 0.3 Coeff) - 0.3 is very bad insulation
    // Map 0-180 degrees
    const lossPct = Math.max(0, Math.min(100, (loss / 0.3) * 100));
    const lossDeg = lossPct * 1.8;
    document.getElementById('gaugeFillLoss').style.transform = `rotate(${lossDeg}deg)`;
    document.getElementById('gaugeValLoss').textContent = loss.toFixed(4);
    
  } else {
    // Show old bar if no learning data yet
    document.getElementById('diagLearnOld').classList.remove('hidden');
    document.getElementById('diagLearnNew').classList.add('hidden');
  }
}

// Initial load
(async () => {
  await refresh();
  await fetchDiag();
  await buildChart();
})();

// ---------- Safe Reboot (Ctrl + Click) ----------
document.getElementById('rebootBtn')?.addEventListener('click', async (e) => {
  if(!e.ctrlKey){ alert('Hold CTRL while clicking to confirm reboot.'); return; }
  try {
    const x = new XMLHttpRequest();
    x.open('GET', '/reboot', true);
    x.setRequestHeader('X-Confirm', 'ctrl');
    x.onreadystatechange = () => {};
    x.send();
    alert('Rebooting...');
  } catch(err){ console.error(err); }
});