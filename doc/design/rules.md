# Regeln

Alle Vorgaben aus dem Handbuch an einem Ort. Die Kürzel (L1, T4, …) sind dieselben wie in `index.html`.

## Raster

- **L1 Kopfzeile y0–9**: Titel bei x0 y0 in Größe 1, darunter bei y10 eine gepunktete Linie über die volle Breite (jedes zweite Pixel, die Punkte wechseln jede Minute die Seite gegen Einbrennen). Immer über `Ui::header(d, title, pages, page)`. Der Titel hat höchstens `(116 − 6·n − 2) / 6` Zeichen, bei 5 Seiten also 14; längere enden auf „...“. Keine Webadresse, kein Doppelpunkt, keine Großbuchstaben als Schmuck. Ausnahme: die QR-Seite des Setup-AP setzt „WiFi“ ab x68 in die rechte Spalte, darunter eine durchgezogene Linie ab x66, weil der QR links die volle Höhe von 64 px braucht.
- **L2 Manager-Ecke x116–127, y0–8**: Ein 12×9-Feld, das dem WidgetsManager gehört, solange ein Rotations-Widget läuft. Es zeigt das Play-Dreieck (Widgets rotieren) oder die Pause-Balken (Rotation angehalten), auf einem angehefteten Widget ein Schloss. Im Busmonitor blinkt im Wechsel ein inverses „BM“. Widgets zeichnen dort nichts.
- **L3 Seitenpunkte**: Ab zwei Seiten, rechtsbündig vor der Ecke: `x = 116 − 6·n + 6·i + 2`, y4. Aktive Seite als Scheibe r2, die anderen als Ring r1.
- **L4 Zwei Takte**: Text im 10-px-Takt auf y14, 24, 34, 44, 54 (fünf Zeilen). Gleichartige Datenzeilen (IP-Listen, Konten) dürfen im 8-px-Takt stehen. Andere Abstände sind nicht erlaubt.
- **L5 Graph-Zone y44–63**: Ein Verlauf belegt unten 20 px, damit oben drei Textzeilen bleiben. Ein Vollbild-Graph nutzt y12–63. Die Grundlinie liegt auf der letzten Pixelzeile des Graphen.
- **L6 Vollbild-Zustände**: ProgMode, OTA und die Haltegeste verwenden dieselbe Kopfzeile wie alle anderen. Darunter ein Wort oder Wert in Größe 2 ab y14, ein Balken, wo es einen Fortschritt gibt (OTA, Geste), und Textzeilen im 10-px-Takt bis y54.
- **L7 Ränder**: Text beginnt bei x0, Menüzeilen bei x2. Rechtsbündige Werte enden 2 px vor dem Rand. Rahmen sind 1 px breit. Unter y56 beginnt keine Textzeile, die letzte liegt auf y54–61.

## Schrift und Text

- **T1 Englisch, kurz**: Alle Texte auf dem Display, im Menü und in den zugehörigen Logzeilen sind englisch. Jedes Zeichen zählt: „Ready“, „3 queued“, „12 min ago“.
- **T2 Größe 2 nur für den einen Wert**: Höchstens ein Text in Größe 2 pro Seite: der Wert, wegen dem die Seite existiert. Größe 3 nur für die Uhrzeit.
- **T3 Schreibweise**: Titel, Bezeichnungen und Aktionen mit großem Anfangsbuchstaben („Network“, „KNX Bus“, „Restart“). Großbuchstaben nur für Abkürzungen (KNX, IP, PA, OTA, BM).
- **T4 Bezeichnung und Wert**: Die Bezeichnung steht links und ist auf eine feste Breite aufgefüllt, mindestens ein Leerzeichen trennt sie vom Wert. In einer Spalte haben alle Bezeichnungen dieselbe Breite.
- **T5 Kürzen**: Gekürzt wird die Bezeichnung, nie der Wert. Abgeschnittenes endet auf „...“ (`Ui::printCut`).
- **T6 Zahlen und Einheiten**: Zähler über `humanCount`, Bytes über `humanBytes`, Dauer als „12 min ago“ oder „3 h left“. Jedes Widget formatiert gleich.
- **T7 Zustand selbst setzen**: Vor dem ersten Zeichnen setzt jedes Widget `setTextSize`, `setTextColor` und `setTextWrap(false)`. Es übernimmt nichts vom Widget davor.

## Menü

- **M1 Auswahl ist invers**: Die gewählte Zeile ist eine weiße Fläche mit schwarzer Schrift. Andere Markierungen (Rahmen, Pfeil, Stern) gibt es nicht.
- **M2 Was die Zeile tut, steht rechts**: „>“ öffnet ein Untermenü, „< v >“ blättert mit OK durch Werte, „v >“ öffnet einen Editor, „<“ geht zurück. Schalter sind Werte mit zwei Stufen: „On“ / „Off“, gewählt „< On >“.
- **M3 Ort und Menge zeigen**: Listen zeigen den Menünamen als Titel und die Position „n/m“ in der Kopfzeile. Mehr als fünf Zeilen bekommen eine Bildlaufleiste bei x126–127.
- **M4 Zerstörendes fragt nach**: Reset, Werkseinstellung und Neustart gehen über die Abfrage. Vorgewählt ist immer „No“.

## Icons

- **I1 9×9 Zeilensymbol**: Steht vor einer Textzeile bei x0, y = Zeile − 1. Der Text beginnt bei x12. Strich 1 px, Füllung nur für Fehler und Warnung.
- **I2 32×32 Menübild**: Das Icon-Karussell zeichnet jeden Hauptmenüeintrag als 32×32-Bild bei x48 y13, die Nachbarn halb so groß links und rechts. Das Bild kommt aus `src/Icons/UiIcons.h`, gesucht nach dem Namen des Eintrags (`OpenKNX::Ui::menuIcon32`); ein neues Modul trägt sein Bild dort ein, sonst bleibt die Stelle leer.
- **I3 17er Plakette**: Gefüllter Kreis r8, darin ein 13×13-Symbol in Schwarz (Notify-Muster). Sie zeigt den Zustand eines ganzen Widgets. Ein Ring r10 in Schwarz trennt sie von der Zeichnung darunter.
- **I4 Bedeutung bleibt gleich**: Haken heißt erfolgreich, Ausrufezeichen heißt Fehler, Uhr heißt wartet, Schloss heißt gesperrt. Ein Symbol hat in allen Modulen dieselbe Bedeutung.

## Graphen

- **G1 Neu ist rechts**: Die Zeit läuft nach rechts, der jüngste Wert steht am rechten Rand. Ein Pixel ist ein Messwert. Der Zeitraum steht rechts oben über dem Graphen („100 s“, „24 h“).
- **G2 Skala**: Prozent und feste Grenzen haben eine feste Skala. Alles andere skaliert auf das laufende Maximum, und der Spitzenwert steht links oben auf einer schwarzen Fläche (NetSpeed-Muster).
- **G3 Linien**: Die Grundlinie ist durchgezogen, das Hilfsraster punktiert im 3-px-Abstand, eine Grenze gestrichelt 2 an, 2 aus. Höchstens zwei Reihen, unterschieden durch Richtung (Spiegel) oder Muster.
- **G4 Leer und Fehler**: Vor dem ersten Messwert steht „-“ statt einer Null. Fehlt der Messwert (z. B. kein Zähler im Build), steht „n/a“ und kein leerer Graph.
- **G5 Balken mit Innenabstand**: Fortschritt und Pegel: Rahmen 1 px, 10 px hoch, Füllung 2 px innen. So unterscheiden sich 0 %, 100 % und „kein Wert“.

## Einfach und Experte

- **V1 Einfach ist Standard**: Eine Karte pro Modul im Notify-Muster: Symbol oben mittig, Plakette unten rechts am Symbol, Zustandswort groß, eine Detailzeile.
- **V2 Das Wort sagt, ob etwas zu tun ist**: „Ready“, „Online“, „Bus OK“ oder „No LAN“, „Bus lost“. Die Zahl steht klein darunter.
- **V3 Experte ergänzt**: Expert-Seiten folgen als weitere Seiten desselben Widgets. Zeilen mit einem Zustand (gut, Warnung, Fehler) bekommen ein Symbol, reine Werte stehen als Bezeichnung und Wert. Mit Auto paging aus blättert man sie mit ↑ ↓.
- **V4 Umschalten**: `Display → Auto paging`: aus (Standard) zeigt die Rotation nur Seite 1 jedes Widgets, die Unterseiten blättert man mit ↑ ↓ und bekommt dafür die volle Anzeigezeit. An: die Unterseiten laufen in der Rotation mit.

## Toast

- **1 Screenshot saved**: Beim Halten von → steht „Capture“ / „Screen“ auf dem Display. Danach „Screenshot saved“ mit dem Dateinamen, ohne Karte „No SD card“.
- **2 Display reset, Panel saved**: Rückmeldung im Menü, als Banner.
- **3 LAN down / LAN up, WiFi lost / WiFi up**: Ausfall als Karte, die Rückkehr mit der Adresse zieht sie zurück. Ein Ausfall, den das Gerät selbst auslöst, zeigt bis zu seiner Grenze nichts.
- **4 Bus lost / Bus connected**: Aus BCU-Zustand und `busOperational()`. Verlust als Karte, „Bus connected“ zieht sie zurück.
- **5 Tunnel und Busmonitor**: „Tunnel open“ mit PA und IP, die IP allein, wenn beides nicht passt; „Tunnel closed“ mit der PA. Am IP-Interface dazu „Busmonitor on“ / „off“.
- **6 SD card in / removed**: Info mit freiem Platz, Entnahme als Warnung.
- **N1 Eine Meldung zur Zeit**: Höchstens ein Toast sichtbar, bis zu 3 warten, Fehler stehen vorn.
- **N2 Gleiches fassen**: Dieselbe erste Zeile innerhalb von 10 s erhöht nur „x3“. Ist die Ursache weg, zieht der Absender die Meldung mit `retractToast()` zurück; kommt sie innerhalb von 60 s wieder, zählt sie weiter und weckt nicht erneut.
- **N3 Tasten**: Toast und Banner nehmen keine Taste weg. Die Karte nimmt OK und ←, auch gehalten, solange keine Haltegeste anläuft: OK ab 1 s, ← auf der Startseite ab 1 s, wenn ← eine Geste belegt hat (Standard: Display off). Dann bleibt die Karte; Loslassen im Balken bricht die Geste ab. Gibt der Absender ein Ziel mit, führt OK es aus (heute gibt keiner eins).
- **N4 Verwerfen statt stauen**: Was die Lage nicht zeigt, wird verworfen (siehe Tabelle). Bei voller Schlange fällt eine neue Meldung weg; ein Fehler nimmt den Platz der letzten wartenden, außer alle wartenden sind frische Fehler.
- **N5 Wenig I2C**: Der Toast ändert nur die unteren drei Pages, das Banner die oberen zwei. Ohne Warteschlange (ESP32) schiebt `i2cDisplay` pro `loop()` ein Stück von höchstens 32 Spalten und wechselt danach zur nächsten geänderten Page.

## Fachmodule

- **D1 Grad als Ring**: Das Gradzeichen wird als Kreis r1 gezeichnet, nicht aus dem Zeichensatz geholt. Welches Zeichen dort steht, hängt von `cp437()` ab. Das ist gemeinsamer Zustand: About und Doom schalten es ein und nicht wieder aus.
- **D2 Viele Kanäle als Raster, wenige als Zeilen**: Ab etwa 8 Kanälen ein Kästchen pro Kanal (gefüllt = aktiv, Rahmen = ruhig, Buchstabe = Sonderzustand). Bis 5 Kanäle je eine Zeile mit Symbol und Wert.
- **D3 Grenzen kommen aus dem Modul**: Die Luftgüte-Stufen sind die des Sensormoduls (CO2 401 / 701 / 1001 / 1401 / 2001 ppm), keine eigenen. Das Display zeigt die Bewertung, es bewertet nicht selbst.

## Abläufe: Setup-QR und Texteingabe

- **Q1 Anheften**: Solange der Setup-AP läuft und kein Handy verbunden ist, bleibt der QR stehen (angeheftet im WidgetsManager, Schloss in der Ecke). ← → wechseln trotzdem, nach 30 s ohne Taste kommt der QR zurück.
- **Q2 Toast und Wecken**: „WiFi setup on“ kommt als Warnung, wenn der AP über die Prog-Taste oder die Konsole startet oder später von selbst, nachdem die Startprüfung ohne AP endete; der AP beim Booten meldet sich nicht. Die Warnung weckt ein gedimmtes Display, Schoner oder Sleep und erscheint im Menü als Banner.
- **Q3 Restzeit zeigen**: Ring und Restzeit („4:59“) nur, wenn der AP eine Frist hat: 5 min beim Rückfall, 10 min aus ProgMode oder Konsole. Ohne gespeicherte WiFi-Daten schließt der AP nie, dann ohne Ring.
- **Q4 Handy da: Seite 2**: Sobald ein Handy verbunden ist und das Display an ist, wechselt das Widget einmal selbst auf die Adresse `10.42.0.1/wifi` und gibt die Rotation frei. Geht das letzte Handy, wird der QR wieder angeheftet.
- **W1 Handy zuerst**: Netz und Passwort gibt man über den Setup-QR am Handy ein. Am Gerät ist das nicht gebaut: dafür fehlen die Netzliste, 63 Zeichen (der Editor hat höchstens 15) und eine Maske für das Passwort.
- **W2 Zeichenrad**: Der Text steht in Größe 2 (zehn Zellen sichtbar), darunter die Nachbarzeichen und die vier Gruppen abc, ABC, 123, #+=. ↑ ↓ wechseln das Zeichen; ↑ halten springt beim Loslassen zur nächsten Gruppe, ↓ halten zur vorigen. Ein Eintrag begrenzt Länge (`ip[0]`, höchstens 15) und Zeichen (`dropdownOptions[0]`), heute für den Namen der SD-Karte.
- **W3 Kein langes OK**: OK halten schaltet ProgMode. OK kurz übernimmt, ← an der ersten Stelle bricht ab, → hinter dem Ende hängt eine Stelle an.
- **W4 Länge zeigen**: Rechts oben steht, wie viele Zeichen OK speichern würde, und das Maximum („6/11“); Leerzeichen am Ende zählen nicht.
