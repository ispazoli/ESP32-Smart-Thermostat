[Magyar (HU)](#magyar-verzió) | [English (EN)](#english-version)

# ESP32 Smart Thermostat Pro (v6.5+)

Ez egy fejlett, hálózatra kapcsolt okostermosztát firmware, amely egy ESP32-es mikrokontrolleren fut. Egy különálló, Tasmota-alapú (ESP8266/ESP32) relé- és szenzoregységet vezérel HTTP-n keresztül.

A projekt célja egy kereskedelmi forgalomban kapható okostermosztátok (pl. Nest, Siemens) tudásával vetekedő, teljes mértékben testreszabható, nyílt forráskódú alternatíva biztosítása, amely helyi hálózaton fut, de képes felhő-szolgáltatások (Google Naptár, Google Sheets) integrálására is.

***

<a id="magyar-verzió"></a>

#  Magyar Verzió (HU)

## 🌟 Főbb Funkciók

A rendszer lelke egy ESP32, amely egy központi "agyként" funkcionál. Minden vezérlési logika, ütemezés és a webes felület itt fut.

* **Modern Webes Felület:** Teljesen reszponzív, témázható (Apple, Siemens, Nest stílusú) mobilbarát weboldal a termosztát vezérléséhez és beállításához.
* **Zárt láncú Tasmota vezérlés:** HTTP parancsokkal vezérel egy távoli Tasmota relét. A parancs kiadása után ellenőrzi a relé tényleges állapotát a `setRelay` funkcióban.
* **Fejlett TPI Vezérlés:** "Time-Proportional Integral" (TPI) algoritmust használ a fűtés precíz, impulzusszélesség-moduláción alapuló vezérléséhez (a V55 modul alapján).
* **"Optimális Indítás" (Smart Learning):** Képes megtanulni az épület hőgyarapodási és hőveszteségi együtthatóját a külső hőmérséklet függvényében. A fűtést korábban elindítja, hogy a kívánt hőfokot *pont* az ütemezett időpontra érje el.
* **Jelenlét Érzékelés (Away Mode):** Figyeli a helyi hálózaton lévő eszközöket (pl. telefonok) PING segítségével. Ha senki sincs otthon egy beállított ideig, automatikusan "Távollét" módba kapcsol, csökkentve a hőmérsékletet.
* **Google Naptár Integráció:** Képes beolvasni egy Google Naptárból az eseményeket, és felülbírálni a fűtési ütemtervet (pl. "Home Office" vagy "Nyaralás" események alapján).
* **Google Sheets Naplózás:** Automatikusan naplózhatja a hőmérsékleti adatokat és a gázfogyasztást egy privát Google Sheets táblázatba.
* **Fejlett 7 Napos Ütemező:** A hagyományos (hétköznap/hétvége) ütemezés mellett egy 21 pontból álló, napokra lebontott ütemező is rendelkezésre áll.
* **Stabilitási Funkciók (Watchdog):**
    * **Tasmota Watchdog:** Figyeli a Tasmota egység válaszait. Ha a Tasmota lefagy (nem ad hőmérsékleti adatot), a termosztát biztonsági okokból lekapcsolja a fűtést, és újraindítási parancsot küld a Tasmotának.
    * **Wi-Fi Öngyógyítás:** Automatikusan megpróbál újracsatlakozni a Wi-Fi hálózatra, ha a kapcsolat megszakad.

## 🛠️ Hardverkövetelmények

1.  **Központi Egység (Agy):** 1 db ESP32 (pl. ESP32 WROOM 32).
2.  **Relé/Szenzor Egység:** 1 db ESP8266 (pl. Wemos D1 Mini) vagy ESP32, amely Tasmota firmware-t futtat.
3.  **Szenzor:** 1 db DS18B20 hőmérséklet-érzékelő, amely a Tasmota egységre van kötve.
4.  **Kapcsolás:** 1 db 5V-os relé modul, amelyet a Tasmota egység vezérel.

## ⚙️ Szoftveres Beüzemelés

A rendszer három fő komponensből áll, amelyeket be kell állítani.

### 1. Lépés: Tasmota Relé/Szenzor Egység

Ez az egység felel a fizikai mérésért és kapcsolásért.

1.  Telepítsd a Tasmota firmware-t az ESP8266/ESP32 eszközre.
2.  Állítsd be a Tasmota felületén a DS18B20 szenzort (pl. a D4-es GPIO-n).
3.  Állítsd be a relét (pl. a D1-es GPIO-n).
4.  Győződj meg róla, hogy az egység fix IP címet kap a routereden (pl. `192.168.1.193`).
5.  Ellenőrizd, hogy a `http://<IP>/cm?cmnd=Status%208` parancsra JSON választ kapsz, ami tartalmazza a hőmérsékleti adatot.
6.  Ellenőrizd, hogy a `http://<IP>/cm?cmnd=Power%20On` és `Power%20Off` parancsokkal tudod kapcsolni a relét.

### 2. Lépés: Google Script (Opcionális, de ajánlott)

A Google Naptár és a Google Sheets naplózás funkciókhoz szükséged van egy Google Apps Scriptre.

1.  Hozz létre egy új Google Sheets táblázatot.
2.  Menj az `Eszközök > Parancsfájl-szerkesztő` menübe.
3.  Illessz be egy parancsfájlt, amely képes fogadni a GET kéréseket (pl. `?temp=21.5` vagy `?action=get_calendar`).
4.  Telepítsd a szkriptet "Internetes alkalmazásként" (Web App), és adj neki "Bárki" (akár névtelenül is) hozzáférést.
5.  Másold ki a kapott "Web App URL"-t (pl. `https://script.google.com/macros/s/..../exec`).

### 3. Lépés: ESP32 Termosztát Firmware

Ez a projekt fő firmware-e.

1.  **Könyvtárak:** Telepítsd az összes szükséges könyvtárat az Arduino IDE-ben (a `globals.h` alapján):
    * `WebServer` (beépített)
    * `WiFiManager` (kell telepíteni)
    * `Preferences` (beépített)
    * `HTTPClient` (beépített)
    * `ArduinoJson` (kell telepíteni)
    * `ESPping` (kell telepíteni)
    * ...és az egyéb beépített könyvtárak (WiFi, ESPmDNS, ArduinoOTA stb.).

2.  **Konfiguráció:**
    * **`config.h`:** Itt kell beillesztened a 2. Lépésben kapott Google Script URL-t a `G_SCRIPT_URL` makróba.
    * **`ESP_Thermostat_Pro_v6_5_1_Calendar_Backend.ino`:** A globális változók között állítsd be a Tasmota egységed IP címét és parancsait:
        ```cpp
        String remoteHost = "[http://192.168.1.193](http://192.168.1.193)"; // <-- Cseréld le a Tasmota IP-jére
        String relayOn = "/cm?cmnd=Power%20On";
        String relayOff = "/cm?cmnd=Power%20Off";
        ```

3.  **SPIFFS Fájlrendszer feltöltése:**
    * A program webes felülete 3 fájlból áll: `index.html`, `style.css`, `app.js`.
    * Telepítsd az "ESP32 filesystem uploader" bővítményt az Arduino IDE-be.
    * Hozd létre a `data` mappát a projekt gyökerében, másold bele ezt a 3 fájlt.
    * Az Arduino IDE `Tools` (Eszközök) menüjéből válaszd az "ESP32 Sketch Data Upload" opciót a fájlok feltöltéséhez.

4.  **Fordítás és Feltöltés:** Fordítsd le és töltsd fel a programot az ESP32-re.

### 4. Lépés: Első Indítás (WiFiManager)

1.  Az első indításkor az ESP32 nem fog tudni csatlakozni a Wi-Fi-hez.
2.  AP (Access Point) módba lép, és létrehoz egy `ESP_Thermostat_Setup` nevű Wi-Fi hálózatot.
3.  Csatlakozz ehhez a hálózathoz a telefonoddal. Egy felugró portál fogad.
4.  Válaszd ki az otthoni Wi-Fi hálózatodat, és add meg a jelszót.
5.  Az ESP32 elmenti a beállításokat, és újraindul, csatlakozva a hálózatodhoz. Az IP címét a soros monitoron láthatod, vagy keresd meg a routeredben `esp32thermostat.local` néven.

## 📖 Felhasználói Útmutató (Webes Felület)

Nyisd meg a termosztát IP címét (vagy a `http://esp32thermostat.local` címet) egy böngészőben.

### Főoldal (Termosztát)



* **Fő kijelző:** Itt láthatod az aktuális hőmérsékletet, a célhőfokot és az aktív programot (pl. "Hétköznap reggel", "Naptár", "Távollét").
* **Jelvények (Badges):**
    * `TÁVOLLÉT` (Away): Akkor jelenik meg, ha a jelenlét-érzékelés aktív.
    * `ELŐFŰTÉS` (Pre-Heat): Akkor jelenik meg, ha az "Optimális Indítás" aktív, és a rendszer épp fűt az ütemezés *előtt*.
    * `BOOST`: Manuális fűtés +1°C-kal 30 percre.
    * `ECO`: Éjszakai (22:00-06:00) hőmérséklet-csökkentés aktív.
* **Gombok:**
    * `Auto/Kézi`: Váltás az ütemezett és a manuális mód között.
    * `BOOST`: Boost mód aktiválása.
    * `ECO`: Eco mód be/kikapcsolása.
* **Egyszerű Ütemezés:** Ha a "Haladó Ütemterv" ki van kapcsolva, itt állíthatod be a Hétköznap/Hétvége reggeli és esti időpontjait és hőfokait.
* **Hőmérséklet Napló:** Az elmúlt 48 óra hőmérsékleti grafikonja.

### Ütemterv (Schedule)

Ez a fül csak akkor látható, ha a "Haladó Ütemterv" be van kapcsolva a Rendszer fülön.

* Itt vehetsz fel új, időponthoz és naphoz kötött fűtési pontokat (max. 21 db).
* Beállíthatod a napot, időpontot, célhőfokot, és engedélyezheted/letilthatod az adott pontot.
* A meglévő pontokat egy kattintással tilthatod, vagy törölheted.

### Rendszer & Statisztika (System)

Itt találhatók a fő beállítások.

* **Szabályozás (Control):**
    * `Hiszterézis`: A kapcsolási érzékenység (csak kézi/egyszerű módban).
    * `Tanuló mód`: Az "Optimális Indítás" be/kikapcsolása.
    * `Haladó ütemterv`: Váltás az egyszerű és a 7 napos ütemező között.
* **Gáz (Gas):** Add meg a gáz fűtőértékét (MJ/m³) és árát (Ft/MJ) a becsült fogyasztás és költség kiszámításához.
* **Okos Funkciók (Smart Features):**
    * **Jelenlét Érzékelés:** Itt add meg a figyelt telefonok IP címeit (vesszővel elválasztva), a türelmi időt (amíg távol lehetsz, pl. 30 perc), és a "Távollét" hőfokot (pl. 16°C).
    * **Naptár Integráció:** Engedélyezi a Google Naptár felülbírálást.

### Időjárás (Weather)

* A termosztát az OpenWeatherMap API-t használja a külső hőmérséklet lekérdezéséhez, ami kulcsfontosságú az "Optimális Indítás" tanulásához.
* Itt kell megadnod az ingyenes OWM API kulcsodat és a városodat.

### Diagnosztika (Diagnostics)

A rendszer belső állapotjelzője, hibakereséshez.

* `Wi-Fi RSSI`: A Wi-Fi jelerőssége.
* `NTP Sync`: Sikerült-e szinkronizálni az időt.
* `Presence Ping`: **(Élő adat)** Azt mutatja, mikor látta utoljára a rendszer a figyelt IP címek valamelyikét. Segít a Jelenlét Érzékelés tesztelésében.
* `CPU Load` / `Heap Memory`: Az ESP32 terheltsége.
* **Okos Tanulás Diagnosztika:**
    * `Kazán Felfűtési Sebesség`: A tanult érték, °C/óra.
    * `Épület Hatékonysága`: A tanult hővesztési együttható (minél alacsonyabb, annál jobb).
* `Eszközinfo`: Nyers adatok az IP-ről, SSID-ről, stb.

***

<a id="english-version"></a>

# English Version (EN)

## 🌟 Core Features

The system's "brain" is an ESP32, which runs all control logic, scheduling, and the web interface.

* **Modern Web UI:** A fully responsive, theme-able (Apple, Siemens, Nest styles), mobile-friendly web interface for controlling and configuring the thermostat.
* **Closed-Loop Tasmota Control:** Manages a remote Tasmota relay via HTTP commands. It verifies the relay's actual state after sending a command using the `setRelay` function.
* **Advanced TPI Control:** Uses a Time-Proportional Integral (TPI) algorithm for precise, pulse-width-modulated heating control (based on the V55 module).
* **"Optimal Start" (Smart Learning):** Learns the building's heat-up and heat-loss coefficients based on outdoor temperature. It starts heating early to reach the target temperature *exactly* at the scheduled time.
* **Presence Detection (Away Mode):** Monitors devices on the local network (e.g., phones) via PING. If no one is home for a set duration, it automatically switches to "Away Mode," lowering the temperature.
* **Google Calendar Integration:** Can read events from a Google Calendar to override the heating schedule (e.g., for "Home Office" or "Vacation" events).
* **Google Sheets Logging:** Can automatically log temperature data and gas consumption metrics to a private Google Sheet.
* **Advanced 7-Day Scheduler:** In addition to the simple (weekday/weekend) schedule, a 21-point, 7-day advanced scheduler is available.
* **Stability Features (Watchdog):**
    * **Tasmota Watchdog:** Monitors responses from the Tasmota unit. If the Tasmota unit freezes (stops sending temperature data), the thermostat performs a failsafe (turns heating OFF) and sends a reboot command to the Tasmota unit.
    * **Wi-Fi Self-Healing:** Automatically attempts to reconnect to the Wi-Fi network if the connection is lost.

## 🛠️ Hardware Requirements

1.  **Central Unit (Brain):** 1x ESP32 (e.g., ESP32 WROOM 32).
2.  **Relay/Sensor Unit:** 1x ESP8266 (e.g., Wemos D1 Mini) or ESP32, running the Tasmota firmware.
3.  **Sensor:** 1x DS18B20 temperature sensor, connected to the Tasmota unit.
4.  **Switching:** 1x 5V Relay Module, controlled by the Tasmota unit.

## ⚙️ Software Setup

The system consists of three main components that must be configured.

### Step 1: Tasmota Relay/Sensor Unit

This unit is responsible for the physical measurements and switching.

1.  Flash the Tasmota firmware onto your ESP8266/ESP32 device.
2.  In the Tasmota web UI, configure the DS18B20 sensor (e.g., on GPIO D4).
3.  Configure the Relay (e.g., on GPIO D1).
4.  Ensure this unit has a static IP address on your router (e.g., `192.168.1.193`).
5.  Verify that `http://<IP>/cm?cmnd=Status%208` returns a JSON response containing the temperature.
6.  Verify that `http://<IP>/cm?cmnd=Power%20On` and `Power%20Off` successfully toggle the relay.

### Step 2: Google Script (Optional, but Recommended)

For Google Calendar and Google Sheets logging, you need a Google Apps Script.

1.  Create a new Google Sheet.
2.  Go to `Extensions > Apps Script`.
3.  Paste in a script that can handle GET requests (e.g., `?temp=21.5` or `?action=get_calendar`).
4.  Deploy the script as a "Web App" and set access to "Anyone (even anonymous)".
5.  Copy the resulting "Web App URL" (e.g., `https://script.google.com/macros/s/..../exec`).

### Step 3: ESP32 Thermostat Firmware

This is the main firmware for the project.

1.  **Libraries:** Install all necessary libraries in the Arduino IDE (based on `globals.h`):
    * `WebServer` (built-in)
    * `WiFiManager` (must be installed)
    * `Preferences` (built-in)
    * `HTTPClient` (built-in)
    * `ArduinoJson` (must be installed)
    * `ESPping` (must be installed)
    * ...and other built-in libraries (WiFi, ESPmDNS, ArduinoOTA, etc.).

2.  **Configuration:**
    * **`config.h`:** Paste your Google Script URL from Step 2 into the `G_SCRIPT_URL` macro.
    * **`ESP_Thermostat_Pro_v6_5_1_Calendar_Backend.ino`:** In the global variables section, set the IP address and commands for your Tasmota unit:
        ```cpp
        String remoteHost = "[http://192.168.1.193](http://192.168.1.193)"; // <-- Change to your Tasmota's IP
        String relayOn = "/cm?cmnd=Power%20On";
        String relayOff = "/cm?cmnd=Power%20Off";
        ```

3.  **Upload SPIFFS Filesystem:**
    * The web UI consists of 3 files: `index.html`, `style.css`, `app.js`.
    * Install the "ESP32 filesystem uploader" plugin in your Arduino IDE.
    * Create a `data` folder in the project's root directory and place these 3 files inside it.
    * From the Arduino IDE `Tools` menu, select "ESP32 Sketch Data Upload" to flash the files.

4.  **Compile and Upload:** Compile and upload the main sketch to your ESP32.

### Step 4: First Boot (WiFiManager)

1.  On first boot, the ESP32 will fail to connect to Wi-Fi.
2.  It will enter AP (Access Point) mode and create a Wi-Fi network named `ESP_Thermostat_Setup`.
3.  Connect to this network with your phone. A captive portal should appear.
4.  Select your home Wi-Fi network (SSID) and enter its password.
5.  The ESP32 will save the credentials, reboot, and connect to your network. You can find its IP in the Serial Monitor or look for `esp32thermostat.local` on your router.

## 📖 User Guide (Web Interface)

Open the thermostat's IP address (or `http://esp32thermostat.local`) in a browser.

### Main Tab (Thermostat)



* **Main Display:** Shows the current temperature, target temperature, and active program (e.g., "Weekday morning", "Calendar", "Away Mode").
* **Badges:**
    * `AWAY`: Appears when Presence Detection is active.
    * `PRE-HEAT`: Appears when "Optimal Start" is active and the system is heating *before* a scheduled change.
    * `BOOST`: Manual heating +1°C for 30 minutes.
    * `ECO`: Nighttime (22:00-06:00) temperature setback is active.
* **Buttons:**
    * `Auto/Manual`: Toggles between scheduled and manual modes.
    * `BOOST`: Activates Boost mode.
    * `ECO`: Toggles Eco mode.
* **Simple Schedule:** If "Advanced Schedule" is off, you can set Weekday/Weekend morning/evening times and temperatures here.
* **Temperature Log:** A 48-hour chart of your temperature history.

### Schedule Tab

This tab is only visible if "Advanced Schedule" is enabled on the System tab.

* Here, you can add new time- and day-specific heating setpoints (up to 21).
* You can set the day, time, target temperature, and enable/disable the point.
* Existing points can be toggled or deleted with one click.

### System & Stats Tab

This is the main configuration page.

* **Control:**
    * `Hysteresis`: The switching sensitivity (used in manual/simple modes).
    * `Smart learning`: Toggles the "Optimal Start" feature.
    * `Advanced Schedule`: Toggles between the Simple and 7-day schedulers.
* **Gas:** Enter your gas heating value (MJ/m³) and cost (e.g., EUR/MJ) to calculate estimated consumption and cost.
* **Smart Features:**
    * **Smart Presence:** Enter the IP addresses of phones to monitor (comma-separated), the "Away Timeout" (e.g., 30 minutes), and the "Away Temperature" (e.g., 16°C).
    * **Calendar Integration:** Toggles the Google Calendar override feature.

### Weather Tab

* The thermostat uses the OpenWeatherMap API to get the current outdoor temperature, which is critical for the "Optimal Start" learning.
* You must enter your free OWM API key and city name here.

### Diagnostics Tab

This is the internal health dashboard, useful for debugging.

* `Wi-Fi RSSI`: Wi-Fi signal strength.
* `NTP Sync`: Shows if the time has been synchronized.
* `Presence Ping`: **(Live Data)** Shows how long ago the system last saw one of the monitored IPs. Helps you test the Presence Detection setup.
* `CPU Load` / `Heap Memory`: The ESP32's resource usage.
* **Smart Learning Diagnostics:**
    * `Boiler Heat-Up Rate`: The learned value in °C/hour.
    * `Building Efficiency`: The learned heat-loss coefficient (lower is better).
* `Device info`: Raw data about IP, SSID, etc.

***

## ⚖️ Licenc (License)

Ez a projekt az MIT Licenc alatt van közzétéve. / This project is licensed under the MIT License.

## 🙏 Elismerések (Acknowledgements)

* Köszönet a projekt eredeti ötletgazdájának, **Ispa**-nak.
* A firmware-t és a dokumentációt az **Ispa** és a **Google AI** (Gemini) közösen fejlesztette.