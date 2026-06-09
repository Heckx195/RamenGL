```shell
cmake --build build/
./build/Debug/task07.exe
```

#line(length: 100%)

= Aufgabe07
- https://docs.gl/
- https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing


== Logik von Percentage Closer Filtering (PCF)
- "Für jedes Fragment innerhalb der Region wird der Schattentest durchgeführt und das Ergebnis (1.0 = Beleuchtet, 0.0 = im Schatten) auf die anderen Schattentests aufsummiert und der Durchschnitt gebildet. Das Ergebnis besagt "zu wie viel Prozent sich das untersuchte Fragment im Schatten befindet."

== 7.2) Performance
#table(
  columns: (auto, auto, auto),
  table.header([*Shadow Map Größe*], [*FPS ohne PCF*], [*FPS mit PCF*]),
  [1024×1024], [1850], [1815],
  [2048×2048], [1880], [1855],
  [4096×4096], [1850], [1850],
  [8192×8192], [1950], [1900],
)

== 7.3) Dokumentation
=== Frage 1.:
Macht es Sinn, Schatten vollständig in Schwarz zu rendern? Wenn ja, warum? Wenn nicht,
  warum nicht?

-> *Lösung*: Nein, wenn die verdeckenden Objekte nicht vollständig lichtundurchlässig sind, verfärben diese den Schatten.

=== Frage 2.:
Wie hat sich die Texturgrösse auf die Performance Ihres Programms ausgewirkt? Hat PCF
  die Performance erhöht oder erniedrigt? Wenn ja, um wie viel Prozent?

-> *Lösung*: Die Texturgröße hat die Performance nicht wirklich beeinflusst. Interessanterweise stieg sie sogar bei 8192x8192 an. PCF hat die Performance leicht erniedrigt, aber nicht signifikant (ca. 30-50 FPS).

=== Frage 3.:
PCF ermöglicht zwar einen weicheren Übergang am Rand des Schattens, lässt aber (mind.) eine physikalische Begebenheit ausser acht. Welche ist das?

-> *Lösung*: PCF berücksichtigt nicht die Lichtstreuung. In der Realität wird das Licht durch die Atmosphäre und andere Partikel gestreut, was zu weicheren Schatten führt. Dieser Effekt wird durch PCF nicht berücksichtigt, da es nur die Tiefenwerte der Schattenkarte vergleicht und nicht die Lichtstreuung. 