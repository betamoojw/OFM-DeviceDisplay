# Display-Design

Die Richtlinie für die OLED-Widgets aller OpenKNX-Module: Katalog, Regeln, Bausteine und Code zum Kopieren. Wer ein neues Widget baut, fängt hier an.

## Was wo liegt

| Datei / Ordner | Inhalt |
|---|---|
| [`index.html`](index.html) | Das Handbuch zum Durchklicken, offline im Browser. Zuerst der **Katalog**: jedes Widget mit seinen Seiten, ein Klick zeigt den Code. Dann Regeln, Raster, Schrift, Bausteine, Menü, Icons, Graphen, Toast und der Widget-Pool. Schalter oben rechts: Panel-Farbe (Weiß, Blau, Grün) und Pixelraster. |
| [`code/handbook.js`](code/handbook.js) | Die Ansichten des Handbuchs, Icon- und Graph-Pool, Toast. Er treibt `index.html`. |
| [`rules.md`](rules.md) | Alle Regeln (L Raster, T Schrift, M Menü, I Icons, G Graphen, V Ansichten, D Module, N Toast, Q Setup-QR, W WiFi-Eingabe). |
| [`tokens.md`](tokens.md) | Die festen Werte mit ihrer Stelle im Code. |
| [`toast.md`](toast.md) | Wann ein Toast erscheint und welche Meldungen es gibt. |
| [`template/`](template/) | `WidgetMyModule.h/.cpp`: ein Widget mit Karte, Werten und Verlauf als Vorlage. |

## Wiederverwenden

Die Bausteine liegen in `src/DisplayUi.h` (`OpenKNX::Ui`), die Symbole in `src/Icons/UiIcons.h`. Eine Seite mit Kopfzeile und zwei Zeilen:

```cpp
#include "DisplayUi.h"
#include "Icons/UiIcons.h"

OpenKNX::Ui::begin(*d);
OpenKNX::Ui::header(*d, "My module", getPageCount(), _page);
OpenKNX::Ui::rowIcon(*d, 0, OpenKNX::Ui::kRowOk, "State", "Ready");
OpenKNX::Ui::row(*d, 1, "Count", "12");
```

Ein neues Widget: `template/` kopieren, Namen ersetzen, im Modul registrieren. Ein neues Menübild: ein 32×32-Feld in `src/Icons/UiIcons.h` und eine Zeile in der Tabelle von `menuIcon32()` mit dem Namen des Menüeintrags.

## Grenzen

Die Bilder im Handbuch sind mit denselben Adafruit-GFX-Algorithmen und dem Zeichensatz `glcdfont.c` nachgerechnet wie auf dem Gerät. Die Zahlen darauf sind Beispieldaten.
