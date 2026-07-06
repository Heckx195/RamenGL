```shell
cmake --build build/
./build/Debug/task09.exe
```

#line(length: 100%)

= Aufgabe09
- https://docs.gl/

#line(length: 100%)

== 9.0) 


=== Implementierung
- Keine Rekursion im Compute-Shader, da sonst ein Call-Stack benötigt wird, der auf Grafikkarten, wo Millionen von Threads gleichzeitig laufen, nicht praktikabel ist. Stattdessen wird die Rekursion in eine Schleife umgewandelt, die die Strahlen (Rays) solange verfolgt, bis sie auf eine Oberfläche treffen oder die maximale Anzahl an Bounces erreicht ist.

Im Compute-Shader:
- RayTriangleIntersect():
  - Schritt 1. (Ebenen-Schnittpunkt): Schaut ob der Strahl die Ebene des Dreiecks schneidet. Wenn nicht, wird sofort abgebrochen (Early Exit).
  - Schritt 2. (Baryzentrischer Test): Wenn die Ebene getroffen wird, wird überprüft, ob der Schnittpunkt innerhalb des Dreiecks liegt. Wenn nicht, wird ebenfalls abgebrochen (Early Exit).
- FindClosestHit():
  - Iteriert über alle Dreiecke der Szene und ruft RayTriangleIntersect() auf, um den nächsten Schnittpunkt zu finden. Interpoliert die Informationen des nächsten Trefferpunkts basierend auf den baryzentrischen Koordinaten (Position, Normale, Farbe) und speichert die Informationen in einer HitInfo-Struktur.

```c

```

=== Ergebnis
#figure(
  image("50_reflection.png", height: 40%),
  caption: [50% Reflektionsanteil: Die reflektierte Umgebung ist deutlich sichtbar, gleichzeitig bleiben Material- und Lichteigenschaften noch klar erkennbar.],
)

#line(length: 100%)

== 9.1) 


=== Implementierung

=== Ergebnis
#figure(
  image("bump.png", height: 40%),
  caption: [Visualisierung der Normalmap auf der unitplane. Die aus der Normalmap ausgelesenen Normalen verändern die Beleuchtungsberechnung, wodurch die eigentlich flache Oberfläche Unebenheiten aufweist.],
)


#line(length: 100%)

== 9.x) Zusatzfragen
In diesem Kapitel sind Fragen zusammengefasst, die sich während der Implementierung ergeben haben (Nützlich für Prüfungsvorbereitung).

=== Frage 1.:
Was sind Radiance-Werte?

-> *Lösung*:
- Intensität des Lichts pro Patch. Schätzwert, wie viel indirektes (durch Bounces gesammeltes) Licht dieses Flächenstück abbekommt und selbst wieder reflektiert)
- Entsteht durch Mone-Carlo-Samples
- Wird in die LightMap geschrieben
- Im Rendern wird der Wert aus der LightMap gelesen und mit der Oberflächenfarbe der Geometrien/ Objekte verrechnet
  - Sichtbar an dem "Color Bleeding" Effekt, der entsteht, wenn die Farbe der Oberfläche in die indirekte Beleuchtung einfließt.

=== Frage 2.:
Was ist eine LightMap?

-> *Lösung*:
- Eine Textur, die die indirekte Beleuchtung (Radiance-Werte) für jedes Flächenstück speichert. Sie wird während des Precomputings (Compute Shader) berechnet und im Renderprozess verwendet, um die indirekte Beleuchtung auf die Szene anzuwenden.
- Wenn sich Geometrien oder Lichtquellen verschieben, muss diese aufwendig neuberechnet werden.

=== Frage 3.:
Wie wird die LightMap berechnet?

-> *Lösung*:
- Sie wird nicht aus der Perspektive der Kamera erstellt!
- Stattdessen wird die Berechnung aus der Perspektive jedes einzelnen Patches selbst durchgeführt: Jedes Patch sitzt an einer festen Position auf der Oberfläche, schaut mit seiner eigenen Normalen in seine eigene Hemisphere hinaus und fragt "wie viel Licht kommt bei mir an?"
  - Unabhängig davon, wo später die Kamera in der Szene steht

=== Frage 4.:
Wie werden die Radiance-Werte von einer 3D-Fläche in einer 2D-Textur (der LightMap) gespeichert?

-> *Lösung*:
- Dies übernimmt die xatlas-Bibliothek. Darin wird die 3D-Geometrie in 2D-UV-Koordinaten "abgewickelt" (unwrapped), sodass jedes Patch der Oberfläche eine eindeutige Position in der LightMap-Textur erhält. Die Radiance-Werte werden dann entsprechend dieser UV-Koordinaten in der LightMap gespeichert.

=== Frage 5.:
Was ist die Monte-Carlo-Idee?

-> *Lösung*:
- Man nähert ein schwer zu berechnendes Integral durch viele zufällige Samples an.
- Bei Raytracing wird dies genutzt, weil man nicht alle Richtungen der Hemisphere einer Oberfläche abtasten kann. Stattdessen werden zufällige Richtungen gewählt, und die Ergebnisse dieser Samples werden gemittelt, um eine Schätzung des Gesamtergebnisses zu erhalten.

=== Frage 6.:

-> *Lösung*:

=== Frage 7.:

-> *Lösung*:

=== Frage 8.:

-> *Lösung*:

== 9.x) Dokumentation
=== Ray-Tracing
- Von jeder Oberfläche (Patch) werden Strahlen (Rays) in zufällige Richtungen innerhalb der Hemisphäre um die Oberflächennormale ausgesendet. Diese Strahlen treffen auf andere Oberflächen und sammeln deren Radiance-Werte, die dann zur Berechnung der indirekten Beleuchtung verwendet werden.
- Wir müssten pro Strahl die gesamte Szene abtasten, um zu sehen, auf welche Oberflächen er trifft. Dies ist jedoch sehr rechenintensiv. Eine Möglichkeit den Rechenaufwand zu verringern, ist mithilfe einer Bounding Volume Hierarchy (BVH) die Anzahl der zu prüfenden Oberflächen zu reduzieren. Die BVH ist eine Baumstruktur, die die Szene in hierarchische Volumina unterteilt, sodass Strahlen schnell entscheiden können, welche Oberflächen sie überhaupt treffen könnten.

=== Ray-Triangle-Intersection
- Verwendet den Möller-Trumbore-Algorithmus, um zu bestimmen, ob ein Strahl (Ray) ein Dreieck in 3D-Raum schneidet. Dabei werden baryzentrische Koordinaten verwendet, um die Position des Schnittpunkts innerhalb des Dreiecks zu berechnen.

=== Baryzentrische Koordinaten
- Baryzentrische Koordinaten sind ein Koordinatensystem, das die Position eines Punktes innerhalb eines Dreiecks durch Gewichte (w0, w1, w2) der drei Eckpunkte des Dreiecks beschreibt. Diese Gewichte geben an, wie stark jeder Eckpunkt zur Position des Punktes beiträgt.
- Zusatzfunktion: Mit den baryzentrischen Koordinaten kann man interpolierte Werte innerhalb des Dreiecks berechnen, z. B. Texturkoordinaten, Normalen oder Farben.

=== Möller-Trumbore-Algorithmus
- Schritt 1. (Ebenen-Schnittpunkt/ Early Exit): Der Strahl (O + t*D) wird mit der unendlichen Ebene geschnitten, in der das Dreieck liegt. Der Algorithmus berechnet dafür `edge1 = v1 - v0`, `edge2 = v2 - v0` und `h = cross(D, edge2)`. Ist `dot(edge1, h) ≈ 0`, verläuft der Strahl parallel zur Ebene (kein Schnittpunkt möglich). Anders als bei der klassischen Ebenengleichung wird die Ebene dabei nie explizit (Normalenvektor + Punkt) aufgestellt – die Kreuz- und Skalarprodukte liefern implizit sowohl die Schnitt-Distanz `t` als auch die Werte für Schritt 2 in einem Rechenweg.
- Schritt 2. (Baryzentrischer Test): Selbst wenn der Strahl die Ebene trifft, kann der Schnittpunkt außerhalb der eigentlichen Dreiecksfläche liegen (die Ebene ist unendlich groß, das Dreieck nicht). Deshalb werden die baryzentrischen Koordinaten `u` und `v` des Schnittpunkts berechnet (die dritte, `w0 = 1 - u - v`, ergibt sich automatisch). Nur wenn `u >= 0`, `v >= 0` und `u + v <= 1` gilt, liegt der Schnittpunkt tatsächlich innerhalb des Dreiecks – sonst wird der Treffer verworfen, obwohl die Ebene geschnitten wurde.
- Erst wenn beide Schritte erfolgreich sind (Ebene getroffen UND Punkt innerhalb des Dreiecks UND `t > 0`, also vor dem Strahlursprung), zählt das als gültiger Treffer.

