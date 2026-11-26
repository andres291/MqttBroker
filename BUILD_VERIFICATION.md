# Build-Nachweis MqttBroker2 1.0.1

Stand: 25.11.2026, erfolgreicher Build am Windows-Rechner.

- Build ueber den mitgelieferten `build.ps1` ausgefuehrt: SUCCESS, Exitcode 0.
- Board: AZ-Delivery ESP-32 Dev Kit C V4, ESP32, 4 MB Flash.
- Plattform: pioarduino 55.03.32, Arduino 3.3.2, ESP-IDF 5.5.1.
- Verwendeter eigener Core: pioarduino Core 6.1.18 in `.pio-core/penv`.
- Effektive generierte `config/sdkconfig.h`: 40 Sockets, 40 aktive TCP-Verbindungen,
  8 Listener, Arduino-Autostart, FreeRTOS 1000 Hz, Optimierung auf Codegroesse.
- `firmware.map` bindet die im Projekt gebaute
  `.pio/build-starter/az-delivery-devkit-v4/esp-idf/lwip/liblwip.a(sockets.c.o)` ein.
  Der Netzwerkstack wurde aus den ESP-IDF-Quellen gebaut, nicht aus der alten
  globalen Arduino-Netzwerkbibliothek uebernommen.
- Windows-Anpassung fuer lange Linker-Befehlszeilen im erfolgreichen Build verwendet.
- Firmware-Binaerdatei: 1.033.872 Byte; gueltige ESP-Image-Kennung 0xE9.
- Programmbelegung laut Linker: 1.033.475 von 1.966.080 Byte (52,6 %).
- Statische RAM-Belegung laut Linker: 41.888 Byte. Dies ist kein Nachweis fuer
  den freien Heap nach WLAN-Start oder bei 30 verbundenen Clients.
- Partitionstabelle aus `partitions.bin` gelesen: app0 bei 0x10000 und app1 bei
  0x1F0000, jeweils 0x1E0000 Byte. Die Binaerdatei passt in beide Partitionen.
- Python-Syntax von Lasttest und Windows-Hilfsskripten geprueft.

SHA256 der erzeugten Firmware:

`4ED9AC82CD4A10BBD5666767199A415C2B8BB6414901E4029B3C4B283BFABEAB`

## Feste IPv4-Adresse ab Version 1.0.1

- Adresse: 192.168.178.120; Netzmaske: 255.255.255.0.
- Gateway und DNS: 192.168.178.1, mit der aktuellen Windows-WLAN-Konfiguration abgeglichen.
- WiFiManager erhaelt dieselbe statische Konfiguration fuer Verbindungen aus dem Portal.
- Vor jedem ausdruecklichen WLAN-Wiederverbindungsversuch wird die statische IP
  erneut gesetzt. Die bisherige Ruecksetzung auf DHCP ist entfernt.
- MQTT-Zugangsdaten wurden nicht geaendert; Tasmotas koennen ihre Brokeradresse behalten.
- Projektstarter verwendet explizit den privaten Python/Core und `.pio/build-starter`.
  Die lokale VS-Code-Konfiguration verwendet ebenfalls den privaten Core.
- Die Firmware wurde kompiliert, aber nicht geflasht; die Adresse auf dem Geraet
  ist daher noch nicht durch einen Verbindungstest bestaetigt.

## Noch am Geraet zu pruefen

Es wurde nichts auf einen ESP32 uebertragen. Die folgenden Punkte sind daher
implementiert, aber noch nicht durch einen Hardwaretest bestaetigt:

- Start und WLAN-Einrichtungsportal auf dem konkreten ESP32.
- 30 reale oder simulierte MQTT-Clients gleichzeitig, Webstatus bei voller
  Belegung, Nachrichtenverteilung, Ping und Wiederverbindung.
- Stromunterbrechung einzelner Clients und WLAN-Ausfall mit Wiederverbindung.
- Passwortgeschuetzter Remote-Upload und Neustart in die andere OTA-Partition.
- Freier/minimaler Heap und Langzeitstabilitaet ueber mindestens 24 Stunden.

Der vorbereitete Lasttest `test/load_test.py` wird erst nach dem USB-Erstflash
gegen die IP des neuen Testbrokers gestartet. Er wurde nicht gegen den alten
Broker ausgefuehrt. MqttBroker und die gemeinsame WiFiRecoveryManager-Bibliothek
wurden fuer dieses Projekt nur gelesen und unveraendert belassen.
