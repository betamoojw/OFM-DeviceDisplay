# Toast

Kurze Meldungen über dem laufenden Bild. Aufruf: `openknxDisplayModule.toast(level, icon9, line1, line2)`, `toastText(level, "Zeile 1\nZeile 2")`, `retractToast(line1)`, wenn die Ursache weg ist. Konsole: `ddc toast <i|o|w|e> text|zeile2`.

## Wo ein Toast erscheint

Aus `act()` und `DisplayToast::update()` in `src/Toast.cpp`. „wartet“: die Meldung bleibt in der Warteschlange, bis die Szene sie zeigt. „entfällt“: sie wird verworfen.

| Stufe | Rotation | Menü | Editor, About, Geste | Schoner | gedimmt | Sleep | aus (Geste) | ProgMode | Update |
|---|---|---|---|---|---|---|---|---|---|
| Info | unten, 2,5 s | Banner | wartet | unten | unten | entfällt | entfällt | entfällt | entfällt |
| Ok | unten, 2,5 s | Banner | wartet | unten | unten | entfällt | entfällt | entfällt | entfällt |
| Warnung | unten, 5 s | Banner | wartet | weckt, dann unten | weckt, dann unten | weckt, dann unten | wartet | wartet | entfällt |
| Fehler | Karte | Karte | wartet | weckt, dann Karte | weckt, dann Karte | weckt, dann Karte | wartet | wartet | wartet |

- „Editor, About, Geste“ umfasst auch andere Status-Widgets und das Startlogo, „Sleep“ auch ein Panel, das nach Zeit ausgegangen ist; „aus (Geste)“ ist nur das per Geste ausgeschaltete Panel.
- Info und Ok erscheinen über dem Schoner und auf dem gedimmten Panel, ohne es zu wecken; der Schoner läuft weiter.
- Geweckt wird nur beim ersten Erscheinen. Eine Meldung, die schon zu sehen war, weckt nicht noch einmal.
- Die Fehlerkarte bleibt, bis OK oder LEFT sie schließt. Das Loslassen dieser Taste löst danach nichts anderes aus.
- Reihenfolge: Fehler zuerst, sonst in der Folge des Eingangs. Dieselbe erste Zeile innerhalb von 10 s zählt hoch („x3“).
- Eine zurückgezogene Meldung, die innerhalb von 60 s wiederkommt, zählt weiter und gilt als gesehen; ein flatternder Kontakt weckt das Panel also nicht in jedem Zyklus.

## Meldungen

| Erste Zeile | Stufe | Zweite Zeile | Zurückgezogen durch | Quelle |
|---|---|---|---|---|
| LAN down | Fehler | Check cable | LAN up | OFM-Network |
| LAN up | Ok | Adresse, dazu DHCP oder FIX, wenn beides passt | – | OFM-Network |
| WiFi lost | Fehler | Netzname | WiFi up | OFM-Network |
| WiFi up | Ok | Adresse, dazu DHCP oder FIX, wenn beides passt | – | OFM-Network |
| WiFi setup on | Warnung | Name des Setup-AP | – | OFM-Network |
| Stored, restarting / No change / Not stored… | Ok / Info / Warnung | Grund | – | OFM-Network, Menü Network |
| Bus lost | Fehler | Check KNX line | Bus connected | IP-Interface, IP-Router |
| Bus connected | Ok | – | – | IP-Interface, IP-Router |
| Busmonitor on / off | Info | – | – | IP-Interface |
| Tunnel open | Info | PA und IP, sonst die IP | – | IP-Interface, IP-Router |
| Tunnel closed | Info | PA | – | IP-Interface, IP-Router |
| SD card in | Info | freier Platz | – | OFM-SDCard |
| SD card removed | Warnung | – | – | OFM-SDCard |
| Formatieren läuft / fertig / fehlgeschlagen | Info / Ok / Warnung | Text der Formatierung, ab „\n“ als zweite Zeile | – | OFM-SDCard |
| Screenshot saved | Ok | Dateiname | – | OFM-DeviceDisplay |
| No SD card / No SD module / Screenshot failed | Warnung | – | – | OFM-DeviceDisplay |
| Display reset / Panel saved | Ok | – | – | OFM-DeviceDisplay |
| Screen may burn in | Warnung | – | – | OFM-DeviceDisplay, wenn Dimmen, Schoner oder Sleep ausgeschaltet wird |

Ein Verlust des Links, den das Gerät selbst auslöst (Neustart des PHY, Wechsel der Betriebsart, Stufe der LAN-Leiter, neuer WiFi-Beitritt), zeigt bis zu seiner Grenze nichts. Nach dem Start meldet der Bus erst etwas, wenn die erste Verbindung 3 s gehalten hat; danach kommen „Bus lost“ und „Bus connected“ bei jedem Wechsel.
