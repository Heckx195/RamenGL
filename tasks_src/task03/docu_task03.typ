```shell
cmake --build build/
./build/Debug/task03.exe
```

#line(length: 100%)

= Aufgabe03

== 3.4) Debugging der Normalenvektoren
Wie testen Sie, ob Ihre Normalenvektoren korrekt sind und auch korrekt
im Shader sind? Nutzen Sie mindestens zwei Möglichkeiten.
Dokumentieren Sie Ihr Vorgehen.

-> *Lösung:*
1. Möglichkeit: Visualisierung der Normalenvektoren
  - Normalenvektoren als Linien in der Szene visualisieren. Dazu wird für jeden Vertex eine Linie gezeichnet zwischen seinem Position und einem Punkt auf seiner Normalen. Die Farbe wird basierend auf seiner Ausrichtung berechnet. Wenn die Normalen korrekt sind, sollten sie in der Szene sichtbar sein und die Farben sollten entsprechend der Ausrichtung variieren.
  - Farbkennzeichnung:
    +X (rechts)	1.0	0.5	0.5	Rot
    -X (links)	0.0	0.5	0.5	Cyan
    +Y (oben)	0.5	1.0	0.5	Grün
    -Y (unten)	0.5	0.0	0.5	Magenta
    +Z (vorne)	0.5	0.5	1.0	Blau
    -Z (hinten)	0.5	0.5	0.0	Gelb-Braun

2. Möglichkeit: Verwendung von RenderDoc
  - In RenderDoc die gerenderten Frames analysieren. In RenderDoc können die Normalenvektoren im Mesh Viewer unter VS Input (Vertex-Shader) inspiziert werden. Zudem kann überprüft werden, ob diese korrekt an den Fragment-Shader weitergegen wird (VS Out)
  - "out_Normal = in_Normal;" zu Vertex-Shader hinzugefügt, um die Normalen direkt an den Fragement-Shader weiterzugeben, und in RenderDoc überprüft, ob die Werte korrekt sind.

== 3.5) Beleuchtung
Recherchieren Sie hierzu das *Lambert cosine law*.
Gibt es bei der Transformation von Modellen hinsichtlich
der Normalenvektoren etwas zu beachten? Falls ja,
erleutern Sie was das Problem ist und leiten Sie
eine geeignete mathematische Lösung her.

-> *Lösung:*
- Das *Lambert cosine law* besagt, dass die Intensität des von einer Oberfläche reflektierten Lichts proportional zum Kosinus des Winkels zwischen der Normalen der Oberfläche und der Richtung des einfallenden Lichts ist. So größer der Winkel zwischen Normalen und Lichtquelle, desto schwächer die Beleuchtung.
- Bei der Transformation von Modellen bei nicht-uniformen Skalierungen können die Normalenvektoren nicht einfach mit der Modellmatrix transformiert werden, da dies zu verzerrten Normalen führt. Das Problem ist, dass die Normalenvektoren orthogonal zur Oberfläche bleiben müssen, aber eine nicht-uniforme Skalierung kann diese Orthogonalität zerstören. Wenn die Oberfläche t mit positiver x-Komponente wird eine x-Streckung das t nach rechts drehen -> -18 Grad. Der Normalenvektor n mit negativer x-Komponente wird durch die gleiche x-Streckung nach rechts gedreht -> +18 Grad. Das Ergebnis ist, dass n nicht mehr orthogonal zu t ist, was zu falschen Beleuchtungsergebnissen führt.
- Deshalb muss die Transformation der Normalenvektoren mit der Inversen Transponierten der Modellmatrix erfolgen, um sicherzustellen, dass die Normalen korrekt transformiert werden und ihre Orthogonalität zur Oberfläche beibehalten wird.
- Die trigonometrische Funktion Cosinus ist 1 bei 0° und 0 bei 90°
- diffuse = max(0, dot(N, L)) \* lightColor \* objectColor
  - Mit max(0, ..) werden nur positive Werte berücksichtigt - keine negativen Beleuchtungswerte
  - dot(N, L) berechnet den Winkel zwischen Normalenvektor N und Lichtvektor L
  - lightColor ist die Farbe des Lichts

== 3.6) Bewegung der Kamera
Erweitern Sie das Programm, sodass Sie die Kamera mit 
der Tastatur steuern können. Sie sollten in der Lage sein,
die Kamera entlang ihrer Achsen zu verschieben und um einen Winkel zu rotieren.
Nutzen Sie für die Tastaturabfrage das Gerüst in der Eventloop und
die Funktionalität von SDL3.
Dokumentieren Sie welches Winkelvorzeichen die Kamera in welche Richtung
rotieren bzw. verschiebt. Erklären Sie warum die Kamera sich so verhält. 

-> *Lösung:*
-> Rechte-Hand-Regel (x Daumen zeigt in positive Richtung, y Zeigefinger zeigt in positive Richtung, z Mittelfinger zeigt in positive Richtung)
- Pitch Up: positive Roation um die X-Achse
- Pitch Down: negative Rotation um die X-Achse
  -> Läuft man ins positive an der X-Achse neigt sich es nach oben 
- Yaw Left: positive Rotation um die Y-Achse
  -> Läuft man ins positive an der Y-Achse dreht man sich nach links
- Yaw Right: negative Rotation um die Y-Achse