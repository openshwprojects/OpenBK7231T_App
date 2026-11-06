# CLAUDE.md

Dies ist ein lokaler Fork von **OpenBeken** (`openshwprojects/OpenBK7231T_App`), einer Tasmota-ähnlichen Firmware für WLAN-IoT-Chips. Das Repo liegt **innerhalb eines BK7231N-SDK-Baums** (`S:/OpenBeken/OpenBK_SH4/apps/OpenBK7231N_App`, SDK-Wurzel ist `../..`). Der Fork ergänzt einen Treiber für den **Sub-GHz-Funkchip CMT2300A** auf dem Tuya-Modul **SH4** (433/868 MHz, OOK-Empfang und -Senden wie bei einer RF-Bridge).

Für alles andere gelten die Upstream-Dokumente weiterhin: [README.md](README.md), [BUILDING.md](BUILDING.md), [FLASHING.md](FLASHING.md), [docs/](docs/) (Befehle, Treiber, Flags, MQTT-Topics, Skripte).

## Bauen

### Firmware für BK7231N (das Ziel dieser Arbeitskopie)

Aus der SDK-Wurzel (`../..`) in einer bash-Umgebung ausführen (Cygwin, MSYS, WSL oder Docker):

```sh
./b.sh                      # = ./build_app.sh apps/OpenBK7231N_App OpenBK7231N_App 1.0.0
./build_app.sh apps/OpenBK7231N_App OpenBK7231N_App <version>
```

- `build_app.sh` setzt `COMPILE_PREX` und ruft hier `make all` auf. Ist `COMPILE_PREX` gesetzt, nimmt das [Makefile](Makefile) den „HACK“-Zweig ganz oben. Er lädt bei Bedarf mbedtls 2.28.5 nach `output/` und ruft dann das `platforms/bk7231n/toolchain/.../build.sh` des SDK auf.
- Die Ausgabe landet in `output/<version>/`: `OpenBK7231N_App_<ver>.bin`, die Varianten `_QIO_` und `_UA_` (OTA), `.rbl`, `.axf`, `.map` und `.asm`.
- Der SDK-Build legt die **`.o`-Dateien direkt neben den Quellen in `src/`** ab. Sie stehen in `.gitignore`. Nicht als Quellcode behandeln und nicht committen.
- [components.mk](components.mk) wird vom SDK-Build eingebunden. Es fügt die Berry-Quellen und `OBK_VARIANT` hinzu, optional auch MQTT-TLS (`CFG_USE_MQTT_TLS=1`).
- Das Makefile dieses Repos ist lokal geändert (`patch_cmt2300a/makefile_buildenv.patch`): Im „HACK“-Zweig steht beim Aufruf von `build.sh` `bash` statt `sh`.

Der eigenständige Makefile-Weg (ohne `COMPILE_PREX`) kennt `make OpenBK7231N`, `make OpenBL602`, `make OpenESP32` usw. Er holt die SDKs als Submodule nach `sdk/`. `VARIANT=berry|tuyaMCU|powerMetering|sensors|...` wählt `OBK_VARIANT`. `build_scripts/*.bat` und `docker/` bauen auf diesem Weg auf.

### Simulator und Unit-Tests (auf dem PC)

```sh
make -f custom.mk                      # Linux, erzeugt build/win_main (optional ASAN=1 / UBSAN=1)
./build/win_main -runUnitTests 2       # führt src/selftest/* aus
```

Unter Windows: `msbuild openBeken_win32_mvsc2017.vcxproj /p:Configuration=Release` (die CI nutzt das Toolset v143). Die `.vcxproj` listet die Dateien einzeln auf. Neue `.c`-Dateien dort (und in `.filters`) eintragen, wenn sie im MSVC-Simulator mitgebaut werden sollen. `custom.mk` nimmt automatisch jede `src/**/*.c`, außer den Chip-HALs und den Dateien in `EXCLUDED_FILES`.

CI: [.github/workflows/workflow.yaml](.github/workflows/workflow.yaml) baut die Simulatoren, führt die Unit-Tests aus und baut alle Plattformen.

### Web-UI-Dateien

`npm install`, danach `gulp` (siehe [gulpfile.js](gulpfile.js)). Das verkleinert `src/httpserver/script.js` und `style.css`. `npm run getcommands` erzeugt `docs/commands*.md` neu aus den `//cmddetail:`-Kommentaren im Quellcode.

## Aufbau des Codes

| Pfad | Inhalt |
| --- | --- |
| `src/user_main.c` | Start (`Main_Init_*`), `Main_OnEverySecond` und der `QuickTick`-Thread, der `DRV_RunQuickTick`, MQTT und die Pins bedient |
| `src/obk_config.h` | Funktionsschalter je Plattform: `ENABLE_DRIVER_*`, `ENABLE_HTTP_*`, `ENABLE_*`. **Für BK7231N gilt der Block `#elif PLATFORM_BEKEN` (ca. Zeile 278)** |
| `src/driver/` | Treiber. `drv_main.c` enthält die `driver_t`-Tabelle (name, init, onEverySecond, appendHTTP, quickTick, stop, onChannelChanged, onHassDiscovery, loaded). Treiber starten mit `startDriver <Name>` |
| `src/cmnds/` | Befehls-Engine für Konsole und Skripte. `CMD_RegisterCommand(name, fn, ctx)` registriert einen Befehl, `cmd_public.h` ist die öffentliche API |
| `src/hal/<plattform>/` | Plattform-HAL (Pins, HW-Timer, Flash, WLAN, OTA). BK7231N nutzt `src/hal/bk7231/` |
| `src/httpserver/` | Web-Oberfläche, REST/JSON, Home-Assistant-Discovery (`hass.c`) |
| `src/mqtt/` | MQTT-Client und Publish-Hilfsfunktionen |
| `src/new_pins.*`, `src/new_cfg.*` | Pin-Rollen, Kanäle, gespeicherte Konfiguration |
| `src/selftest/` | Unit-Tests, die im Simulator laufen |
| `libraries/berry` | Berry-Skript-Submodul. Das Make-Ziel `berry` erzeugt die Ressourcen |
| `platforms/<CHIP>/` | Build-Anbindung je Plattform (`pre_build.sh`, cmake/mk) |
| `sdk/` | Hersteller-SDKs als Submodule für den eigenständigen Makefile-Weg. Hier meist nicht nötig |

## CMT2300A-/SH4-Treiber (lokale Ergänzung)

Die Dateien sind `src/driver/cmt2300a.c/.h` (Chip-API auf unterster Ebene), `cmt2300a_defs.h` (Register), `cmt2300a_params.h` (Registerbank-Voreinstellungen aus dem RFPDK-Export) und `drv_cmt2300a.c/.h` (der OBK-Treiber, ca. 6000 Zeilen). Die Anwender-Doku mit allen Befehlen, MQTT-Topics und Beispielen für Home Assistant und openHAB steht in [docs/cmt2300a.md](docs/cmt2300a.md). Bei Änderungen an Befehlen oder Topics dort mitpflegen.

- Eingeschaltet wird er mit `#define ENABLE_DRIVER_SH4 1` im Block `PLATFORM_BEKEN` von `obk_config.h`. Der Eintrag in `drv_main.c` hängt an `ENABLE_DRIVER_SH4` (das auskommentierte `ENABLE_DRIVER_CMT2300A` wird nicht benutzt). Der Treibername ist `CMT2300A`, die Init-Funktion heißt `CMT2300A_SH4`. Gestartet wird er also mit `startDriver CMT2300A`, z. B. in der autoexec.
- Die Pins sind in `drv_cmt2300a.c` **fest eingetragen** und werden nicht über OBK-Pin-Rollen eingestellt: SCLK=P14, SDIO=P16, CSB=P6, FCSB=P26, GPIO1/DCLK=P20, GPIO2/DOUT-DIN=P22. Der Treiber nutzt 3-Draht-SPI per Bit-Bang und einen HW-Timer zum Abtasten bei RX/TX.
- Die Schalter zur Compile-Zeit stehen oben in `drv_cmt2300a.c`: `CMT_ENABLE_COMMANDS`, `CMT_ENABLE_MQTT`, `CMT_TX_BAND_LOCK`, `CMT_AUTOCAP_MS` und `CMT_LED_BLINK_MS`.
- Die Konsolenbefehle sind `cmtinfo`, `cmtdump`, `cmtreg`, `cmtfreq`, `cmtvco`, `cmtscan`, `cmtspy`, `cmtstop`, `cmtcap`, `cmtsend`, `cmtcarrier`, `cmtsensor` und `cmtspec`. Ihre Argumente beschreibt der Kopfkommentar in `drv_cmt2300a.c`.
- Empfangene Fernbedienungscodes gehen per MQTT als JSON im RfReceived-Stil raus. Die Spektrum-Ansicht (`cmtspec`) erscheint als SVG auf der Hauptseite und wird nach `<Geraet>/RfSpectrum/get` gesendet.
- **Protokoll-Engine** (aufgebaut wie rtl_433, Bitkonventionen ebenso, damit Decoder aus `rtl_433/src/devices/*.c` direkt übernommen werden können):
  - `cmt_pk_feed()` sammelt Puls/Pause-Paare zu Paketen. Ein Paket endet bei einer Pause ab 25 ms oder nach 40 ms Funkstille (`cmt_pk_tick`). Läuft der Puffer (512 Paare, `CMT_PK_MAX`) vorher voll, schiebt `cmt_pk_slide()` ihn weiter: Die ältesten Paare fallen weg, die letzten 384 (`CMT_PK_KEEP`) bleiben (gleitendes Fenster). Ein Telegramm, dem Rauschen ohne Pause vorausgeht, steht so am Ende des Puffers.
  - `cmt_slice()` zerlegt jedes Paket für jedes eingeschaltete Gerät in `s_bb`-Zeilen (Bitpuffer mit Sync-Zählern). Modulationen: PPM (kurze Pause = 0), PWM (kurzer Puls = 1), PCM (für Manchester-Decoder), DMC und RCS (rc-switch mit Periodenprüfung, Bit = Puls länger als Pause).
  - Bei PWM-Geräten mit `per` (feste Periode) folgt das Bit aus Puls < Pause, und die Verzerrung wird über den symmetrischen Sync geschätzt. Das ist robust gegen gestreckte High-Pulse.
  - Die Gerätetabelle `s_dev[]` enthält 25 Einträge (`CMT_D_*`): Wetterstationen von 32 bis 88 Bit, F007TH/Oregon (Manchester), WT450 (DMC), Somfy RTS, Intertechno/NewKaku, X10 und rc-switch 1/2/5/7.
  - Dazu kommen bis zu 8 eigene Geräte zur Laufzeit (`cmtflex`, `s_flex[]`).
  - Decoder heißen `cmt_d_*`, prüfen Prüfsumme und feste Bits und melden über `rec_*()` und `cmt_emit()`.
- Ausgabe:
  - Sensoren gehen nach MQTT `RfSensor`, `RfSensorCh<N>` und `RfSensor<Feld>`, alles andere nach `RfProtocol` (JSON mit `Protocol`, Feldern, bei Bedarf `Data`/`Bits`).
  - MQTT-Publishes laufen über eine eigene Warteschlange (`cmt_pq_push`, 1 KB). `cmt_pq_tick` sendet höchstens einen Publish je Quick-Tick, denn ein Publish dauert ~100 ms und eine Meldung hat bis zu 16 davon. Ohne MQTT-Verbindung wird nichts gepuffert. Die Zähler stehen in `cmtproto`. Neue Meldungen deshalb immer über `cmt_mqtt_value`/`cmt_pq_push` senden, nie direkt mit `MQTT_Publish*`.
  - Auf der Hauptseite steht eine Tabelle mit den letzten 8 Telegrammen.
  - 24-Bit-Fernbedienungen meldet zusätzlich unverändert der alte Decoder `cmt_dec_feed` als `RfReceived`. `RcSwitch*` überspringt deshalb 24 Bit.
- Senden: `cmt_gen_build()` moduliert Rohbits mit den Zeiten eines Geräts (Felder `pw`, `txsp`, `txsg`, `txgap`, `nsync`, `txrep`). Darauf bauen `cmtsendp`, `cmtsensor` (Encoder in `cmt_vs_encode()`, sendefähig laut `s_stx[]`, Kanal 1..8) und `cmtkaku` auf.
- Weitere Befehle:
  - `cmtproto` listet Protokolle und Zähler und schaltet sie ein oder aus.
  - `cmtspy` schickt jeden Burst durch alle Geräte, ordnet rc-switch zu und gibt Manchester-Bits aus.
- Stack: QuickTick läuft auf BK7231N im FreeRTOS-Timer-Task mit nur 3000 Bytes Stack. Große Puffer (`s_rec`, JSON) sind deshalb statisch. Neue Decoder sollen `s_rec` nutzen und keine großen lokalen Arrays anlegen.
- Rechenzeit: Der Watchdog (10 s) wird in `Main_OnEverySecond` gefüttert, und das läuft im selben Timer-Task. Dauerlast im Quick-Tick führt deshalb zur Bootschleife. Dagegen schützen drei Mechanismen:
  - Ein fertiges Paket wird in einen zweiten Puffer getauscht (`cmt_pk_start`). `cmt_pk_work()` wertet es dann verteilt über mehrere Quick-Ticks aus, je Tick höchstens `CMT_PK_SLICE_MS` (15 ms). Ein Paket mit ~1000 Paaren braucht für alle Geräte ~150–200 ms, weil der Code aus dem Flash läuft. Wird ein Paket fertig, während noch ausgewertet wird, kommt es in einen Ersatzpuffer, wenn es kurz ist (bis 160 Paare, `CMT_PK_SPARE`). Es wird dann direkt danach ausgewertet. Das braucht z. B. WH2: Dessen zweite Zeile kommt 48 ms nach der ersten und ist oft die einzige saubere. Längere Pakete werden verworfen, der Zähler dafür ist `verworfen(belegt)`.
  - Dauerrauschen ohne Pause wird nicht ausgewertet, das Fenster gleitet nur weiter. Ausgewertet wird erst, wenn eine echte Pause kommt. Gekürzte Pakete zählt `cmtproto` getrennt, mit `cmtproto voll off` werden sie zum Vergleich übersprungen. Früher wurden volle Pakete verworfen, und dabei gingen echte Telegramme verloren.
  - Manchester-Geräte beenden die Zeile bei Pulsen über 2T.
  - Ist der letzte Start gescheitert (`g_bootFailures >= 2`), startet die Engine ausgeschaltet (`cmtproto all on`).
  - Zähler dazu stehen in `cmtproto`: verworfen, verteilt, ms und max. ms je Paket, außerdem die Rechenzeit je Gerät. `cmtproto reset` setzt die Zähler zurück.
- OTA: Solange `OTA_GetProgress() != -1` ist, pausiert der Treiber selbst. Der Quick-Tick beendet ein laufendes Senden, hält den Abtast-Timer an, leert die Warteschlangen und tut danach nichts mehr. Früher scheiterte OTA, wenn der Treiber lief. Nach einem gescheiterten OTA läuft der Empfang automatisch weiter.
- RAM: Die statischen Puffer des Treibers belegen ~10 KB. Im Ruhezustand bleiben ~49 KB Heap frei (mit 1.0.10 waren es ~61 KB). Die OBK-Web-App braucht kurzzeitig bis zu 32 KB, und OBK startet neu, wenn länger als 5 s weniger als 25 KB frei sind. Neue statische Puffer deshalb sparsam anlegen.
- IR: Der Upstream-IR-Treiber ([src/driver/drv_ir.cpp](src/driver/drv_ir.cpp)) ist lokal ergänzt. `IREnable RX 0|1` schaltet den Empfänger aus bzw. ein (`gIRRecvDisabled`), Senden geht weiter. Grund: Ein IR-Empfänger, der Fremdlicht abbekommt, belegt im 50-µs-ISR fast die ganze CPU. HTTP, MQTT und OTA werden dann langsam, und der Heap fällt bis unter die Neustart-Schwelle. Diese Änderung gehört mit in den Patch.
- Nur OOK/ASK: FSK-Geräte (viele 868-MHz-Sensoren, Bresser 5in1/6in1/7in1, LaCrosse IT+) brauchen eine andere Konfiguration des Chips und werden nicht empfangen.
- Ein neues Protokoll braucht:
  - einen `CMT_D_*`-Index und einen Eintrag in `s_dev[]` (`CMT_D_N` anpassen),
  - einen Decoder `cmt_d_*`,
  - zum Senden mit `cmtsensor` einen Eintrag in `s_stx[]` und einen Zweig in `cmt_vs_encode()`.
- Kommentare und Log-Texte in diesem Treiber sind **deutsch, ohne Umlaute** (ae/oe/ue). Diesen Stil beim Bearbeiten beibehalten.
- Der Treiber-Quellcode steckt in keinem Plattform-`#if` und nutzt `HAL_HWTimer*`. Nach Änderungen prüfen, ob der Linux-Simulator noch baut (`make -f custom.mk`), oder die Datei dort ausschließen.

`patch_cmt2300a/` enthält den gesamten Fork als Patches auf dem Upstream-Commit `ce1abad8` (`cmt2300a.patch`, `makefile_buildenv.patch`, dazu Kopien der Dateien in `files/`). `git apply patch_cmt2300a/cmt2300a.patch` spielt ihn auf einen frischen Upstream-Stand neu auf. **Nach Änderungen am Treiber die Patches neu erzeugen.**

## Konventionen

- Einrückung mit Tabs (Breite 4). Der Stil steht in `.clang-format` (LLVM-Basis, Klammern in derselben Zeile, keine Zeilenlängengrenze).
- C99 (`gnu99`). Firmware-Code darf nicht viel Speicher anfordern und nicht lange blockieren, weil die Quick-Tick-Handler etwa alle 5 ms laufen. `cmtcap` blockiert absichtlich.
- Neue Treiber brauchen einen Eintrag in `drv_main.c` mit `//drvdetail:`-Kommentaren, einen Schalter `ENABLE_DRIVER_*` in `obk_config.h` und, falls sie unter Windows gebaut werden, einen Eintrag in der `.vcxproj`. Neue Befehle brauchen `//cmddetail:`-Kommentare, damit der Doku-Generator sie findet.
- Geloggt wird mit `ADDLOG_INFO/ERROR/DEBUG(LOG_FEATURE_*, fmt, ...)`.
- git meldet „dubious ownership“, weil das Repo auf einer Netzwerkfreigabe liegt (`\\192.168.6.1\...`). `git -c safe.directory='*' ...` verwenden oder eine safe.directory-Ausnahme eintragen.
- Antworten, Kommentare und Dokumente für dieses Projekt auf Deutsch schreiben.
