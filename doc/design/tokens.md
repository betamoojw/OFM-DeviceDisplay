# Tokens

Feste Werte für alle Widgets, mit der Stelle im Code. Ein Widget nimmt sie über `OpenKNX::Ui` und rechnet sie nicht selbst nach.

| Wert | Bedeutung | Im Code |
|---|---|---|
| y10 | punktierte Linie unter dem Titel, Phase wechselt jede Minute | `Ui::HeaderLine`, `dottedLine()` in `src/DisplayUi.cpp` |
| y14 / 10 px | erste Textzeile, Zeilentakt | `Ui::Row0`, `Ui::Pitch` |
| 8 px | Takt für gleichartige Datenzeilen | `Ui::PitchDense` |
| x116, 12 px breit | Ecke des Managers (Play, Pause, Schloss, BM) | `Ui::CornerW`, `WidgetsManager::drawManagerOverlay()` |
| 6 px, y4, r2 / r1 | Seitenpunkte vor der Ecke, aktive Seite gefüllt | `Ui::pageDots()` |
| 6 px | Zeichenbreite, Wert endet 2 px vor dem Rand | `Ui::CharW`, `Ui::ValueEnd` |
| x12 | Text hinter einem 9×9-Zeilensymbol | `Ui::IconTextX` |
| 9×9 / 13×13 / 32×32 | Zeilensymbol / Plakette / Menübild | `src/Icons/UiIcons.h` (`kRow*`, `kBadge*`, `kMenu32*`) |
| r8 / r10 | Plakette, freier Ring darum | `Ui::BadgeR`, `Ui::BadgeGap` |
| 10 px / 2 px | Balkenhöhe, Innenabstand der Füllung | `Ui::BarH`, `Ui::BarPad` |
| 500 ms | Blinken (ProgMode) | `PROG_MODE_BLINK_DELAY` |
| 30 s | ein angeheftetes Widget kommt nach der letzten Taste zurück | `WidgetsManager::PIN_RETURN_MS` |
| 2,5 s / 5 s | Toast Info und Ok / Warnung; die Fehlerkarte bleibt bis OK oder LEFT | `DisplayToast::ShortMs`, `LongMs` |
| 300 ms | Toast fährt ein und aus | `DisplayToast::SlideMs` |
| 10 s | gleiche erste Zeile zählt hoch („x3“) | `DisplayToast::MergeMs` |
| 60 s | eine zurückgezogene Meldung kommt in dieser Zeit gesehen zurück und weckt nicht | `DisplayToast::ForgetMs` |
| 4 | Plätze der Toast-Warteschlange, einer sichtbar | `DisplayToast::Slots` |
| 21 / 17 / 14 | Zeichen je Textfeld / Toast unten / schmalste Zeile (Karte, Banner neben „xN“) | `DisplayToast::LineLen`, `BottomChars`, `NarrowChars` |
