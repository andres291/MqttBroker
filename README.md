# MqttBroker2

Eigenstaendiger ESP32-MQTT-Broker fuer das AZ-Delivery DevKit V4 mit 4 MB Flash.
MqttBroker und die gemeinsame WiFiRecoveryManager-Bibliothek werden nicht veraendert.

## Funktionsumfang

- Ziel: 30 gleichzeitig verbundene MQTT-Clients; insgesamt 40 lwIP-Sockets und
  40 aktive TCP-Verbindungen. Die Clientgrenze enthaelt Tasmota, Loxone und MQTTX.
- Arduino 3.3.2 als Komponente unter ESP-IDF 5.5.1, festgelegte pioarduino-Plattform
  55.03.32. Der Netzwerkstack wird mit den Projekteinstellungen neu kompiliert.
- Alle Plattform- und Framework-Pakete liegen im eigenen Ordner `.pio-core`.
- Nur benoetigte Arduino-Funktionen werden ausgewaehlt; optionale Cloud-, Audio-
  und andere Registry-Komponenten sind fuer diesen Broker ausgeschlossen.
- WLAN-Daten im nichtfluechtigen Speicher; automatischer Wiederverbindungsversuch
  alle 30 Sekunden. Nach 60 Sekunden Ausfall oeffnet sich zusaetzlich das
  Einrichtungsportal. Bei erfolgreicher Verbindung wird es geschlossen.
- Port 1883 fuer MQTT; Weboberflaeche und passwortgeschuetztes Remote-Update auf
  Port 80. Diagnose unter `/status` und alle 60 Sekunden als retained Nachricht
  auf `mqttbroker2/status`, ohne zusaetzliche MQTT-Verbindung.
- Zwei Firmware-Partitionen fuer Remote-Updates; kein automatischer taeglicher Neustart.

## Windows-Buildstarter

`build.ps1` verwendet die zum festgelegten ESP32-Paket gehoerende private
PlatformIO-Version (pioarduino Core 6.1.18), nicht die globale Installation.
Die auf diesem Rechner vorhandene globale Core-Version 6.2.0 hat mit diesem
Plattformpaket einen SCons-Versionskonflikt. Deshalb fuer dieses Projekt den
Starter oder die mitgelieferten VS-Code-Aufgaben `MqttBroker2: Build`,
`MqttBroker2: USB Upload` und `MqttBroker2: Monitor` verwenden. Der Starter
installiert bei einem neuen Checkout zuerst die benoetigten Projektwerkzeuge.
Der Starter verwendet ein eigenes Build-Verzeichnis `.pio/build-starter`,
damit IDE-Hintergrundaktionen den laufenden Build nicht loeschen. Die lokale
VS-Code-Konfiguration verweist ebenfalls auf den eigenen Core.
Optional laesst sich der serielle Anschluss mit `-Port COM6` festlegen.
Die Skripte unter `scripts/` umgehen zudem die Windows-Grenze fuer lange
Befehlszeilen beim Erzeugen der Linkerdatei, ohne Framework-Dateien zu patchen.

## Feste IP im Heimnetz

Der Broker verwendet fest **192.168.178.120**, mit Netzmaske **255.255.255.0**
und Gateway/DNS **192.168.178.1**. Diese Werte wurden mit der aktuellen
WLAN-Konfiguration des Rechners abgeglichen. Die Adresse wird beim Start,
beim Verbinden ueber das Einrichtungsportal und bei WLAN-Wiederverbindungen
verwendet. Das Einrichtungsportal bleibt unter `192.168.4.1` erreichbar.
Die Einstellungen stehen in `include/config.h`.

Vor der Inbetriebnahme den alten Broker ausschalten. Beide Geraete duerfen
nicht gleichzeitig 192.168.178.120 verwenden. Die Adresse im Router fuer den
neuen ESP32 freihalten. Die Tasmotas koennen ihre bisherige Brokeradresse und
ihre MQTT-Zugangsdaten behalten. Update-Seite nach dem Start:
`http://192.168.178.120/update`.

## Zugangsdaten

`include/credentials.h` enthaelt die aus MqttBroker uebernommenen MQTT-Zugangsdaten
und einen neu erzeugten Updatezugang (Benutzer `admin`, individuelles Passwort).
Diese Datei ist von Git ausgeschlossen. Bei einem neuen Checkout die Datei aus
`credentials.example.h` erstellen und die Werte setzen.

Einrichtungs-WLAN: **MqttBroker2 Setup**, Passwort **mqttbroker2**.
Nach der Verbindung im Browser `http://192.168.4.1` oeffnen und das Heim-WLAN auswaehlen.
Name und AP-Passwort lassen sich in `include/config.h` aendern.
MQTT/HTTP sind fuer das vertraute lokale Netzwerk gedacht; keine Portfreigabe ins Internet.

## Bauen und erstmalig starten

1. Diesen Ordner als eigenes PlatformIO-Projekt oeffnen.
2. `powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1` baut die Firmware. Der erste Build laedt eine eigene Toolchain und
   Frameworks herunter und benoetigt mehrere GB freien Speicher sowie Zeit.
3. Erster Flash per USB: `powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Mode Upload`. Es wird nicht automatisch geflasht.
   Die neue Partitionstabelle erfordert diesen USB-Erstflash; die alte Update-Seite
   soll fuer den Wechsel auf MqttBroker2 nicht verwendet werden.
4. Serielle Ausgabe mit `powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Mode Monitor`, 115200 Baud. Dort erscheint die IP.
5. Falls erforderlich ueber das Einrichtungs-WLAN das Heim-WLAN konfigurieren.
6. MQTTX mit MQTT 3.1.1, Port 1883 und den bisherigen MQTT-Zugangsdaten verbinden.

WLAN-Daten koennen bei einem Flash auf denselben ESP32 erhalten bleiben. Den alten Broker vor dem Start des neuen ausschalten: Diese Firmware verwendet
bewusst dieselbe feste IP 192.168.178.120. Fuer einen parallelen Test vorher
die Testadresse in `include/config.h` aendern und neu bauen.

## Remote-Update

`http://<ESP-IP>/update` oeffnen, mit den Update-Zugangsdaten aus `credentials.h`
anmelden und die neue `.pio/build-starter/az-delivery-devkit-v4/firmware.bin` hochladen.
Die Datei muss zu diesem Board und dieser Partitionstabelle passen. Nach Erfolg
startet der ESP32 neu; MQTT-Clients verbinden sich danach erneut. Retained
Nachrichten liegen im RAM und gehen bei einem Neustart verloren.

## Gepruefter Build

Der Windows-Build ueber `build.ps1` war am 25.11.2026 erfolgreich. Die fertige
Firmware liegt unter `firmware/MqttBroker2-1.0.1.bin` (Kopie der Build-Ausgabe).
Sie ist 1.033.872 Byte gross und passt in beide 1.966.080-Byte-OTA-Partitionen.
Der Linker meldet 41.888 Byte statisch belegtes RAM; freier Heap im Betrieb und
Stabilitaet mit 30 realen Clients sind erst am Geraet zu messen.
Nachweise und Pruefgrenzen stehen in `BUILD_VERIFICATION.md`.

## Pruefung auf dem Geraet

Der Build allein beweist noch keine Stabilitaet mit 30 realen Clients. Zum Test
mit einem separaten ESP32 starten, waehrend keine anderen Clients diesen Broker
belegen. Der Test verwendet nur die Python-Standardbibliothek und liest die
MQTT-Zugangsdaten aus `credentials.h`, ohne sie auszugeben:

```powershell
python test/load_test.py <ESP-IP> --clients 30 --seconds 600
```

Er prueft Anmeldung, Subscribe/Publish an alle Clients, Ping, Webstatus bei voller
Belegung, Ablehnung eines 31. Clients und den Ersatz von drei getrennten Clients. Anschliessend mit den echten Tasmota-Geraeten testen:
WLAN fuer mindestens 90 Sekunden unterbrechen, Wiederverbindung und Portal pruefen,
OTA unter Last pruefen und mindestens 24 Stunden Clientzahl/Heap beobachten.
Ein erfolgreicher Test muss 30 stabile Clients, funktionierendes Webinterface und
keinen fortlaufenden Speicherverlust zeigen. Ein separater PC/MQTTX zaehlt bereits
zu diesen 30 Clients.

## Lokale Bibliotheksanpassungen

Die sMQTTBroker-Basis wurde aus dem bisherigen Projekt kopiert (Versionsangabe
0.1.8; Lizenz im Bibliotheksordner). Nur diese Kopie wurde angepasst:

- Clientbereinigung ohne ungueltige Iteratoren nach `vector::erase()`.
- Begrenzung auf 30 TCP-Sitzungen, Clientzaehler und Kapazitaetsablehnungen.
- Beim WLAN-Ausfall alle Clients/Subskriptionen schliessen, Listener neu starten;
  retained Nachrichten bleiben waehrend des WLAN-Ausfalls erhalten.
- Keepalive-Vergleich ueber den `millis()`-Ueberlauf hinweg korrigiert.
- Keine Passwortausgabe; Befehle vor erfolgreicher MQTT-Anmeldung ablehnen.
- MQTT-Paketgroesse auf 8192 Byte begrenzt.

Der MQTT-Umfang der Basisbibliothek bleibt begrenzt: MQTT 3.1.1, fuer diesen
Einsatz QoS 0 verwenden; keine dauerhaften Sessions oder persistente Speicherung.
Last-Will-Nachrichten werden von dieser Basis nicht veroeffentlicht. MQTT 5 und
TLS sind nicht enthalten. Ein vollstaendiger MQTT-Konformitaetstest ist nicht
Bestandteil dieses Projekts.
