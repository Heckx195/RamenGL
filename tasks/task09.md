# Task 09 - Precomputed Raytraced Diffuse Global Illumination via Compute Shaders

Ziel dieser Aufgabe ist es eine Lightmap zu generieren, welche als Cache für den diffusen, globalen
Beleuchtungsanteil der Szene dienen wird. Dabei werden alle Dreiecke der Szene in gleichgroße
kleinere Flächen, sogenannte **Patches** unterteilt. Jedes Patch wird einem Pixel in der
Lightmap zugeordnet. Die Generierung der Patches und die Zuordnung der UV Koordinaten ist bereits
für Sie implementiert.

Nutzen Sie das Programm ```task09.cpp``` als Startercode.

Innerhalb des Computeshaders (```computeshader.comp```) können Sie einfach auf ein einzelnes
Patch zugreifen, da genau so viele work-items gestartet werden, wie es Patches gibt.
Die Generierung für randomisierte Rays, die uniform über die Hemisphere eines Patches
verteilt sind, ist ebenfalls schon implementiert.

Realisieren Sie nun innerhalb des Computeshaders ein einfaches Monte-Carlo Programm, welches
Ihnen die Lambert BRDF für jedes Patch annähert. Dazu müssen Sie für jedes Ray durch
sämtliche Dreiecke iterieren und prüfen, welches davon das Ray zuerst (kürzeste Distanz) schneidet.
Abhängig von der Anzahl der **Bounces**, wiederholen Sie diesen Vorgang solange, bis entweder
kein Schnittpunkt mehr gefunden wird (geben Sie eine vordefinierte **Skycolor** zurück, z.B. vec3(1.0f))
oder Sie die maximale Anzahl an erlaubten Bounces erreicht haben. Achten Sie darauf, dass Sie
dem Patch nur dann eine Radiance zugewiesen wird, wenn auch wirklich eine Lichtquelle involviert war.
Die Lichtquelle ist in unserem Beispiel der Himmel, also, wenn das Ray der Levelgeometrie "entkommen"
konnte. Natürlich wäre es auch denkbar Vertexattributen ein Flag mitzugeben, dass es als
"Strahler" deklariert. Gerne können Sie dies implementieren, es ist jedoch nicht gefordert.

Achten Sie darauf, dass rekursive Funktionsaufrufe in Shadern **nicht** möglich sind.
Implementieren Sie Ihr Raytracing daher iterativ.
Damit Monte-Carlo Raytracing konvergiert, sind viele, viel Samples pro Patch notwendig. Das Starterprogramm
ist so aufgesetzt, dass **ein** Aufruf des Computeshaders (```glDispatchCompute(...)```)
**einem** Sample pro Patch entspricht. Der Grund dafür ist, dass das Betriebssystem den Prozess
abschießt, wenn dieser innerhalb eines gewissen Zeitfensters nicht aus dem Computeshader
zurückkehrt. Der Intersectiontest zwischen Dreieck und Ray ist relativ teuer. Wenn Sie also
mehrere Bounces pro Sample und viele Samples innerhalb eines Computeshader Dispatchcalls
durführen, kann es je nach GPU und Betriebssystem zu Problemen kommen.

Tipp zum Vorgehen: Implementieren Sie zunächst den Ray-Triangle Intersection Algorithmus.
Testen Sie diesen mit einem einzigen Ray und geben z.B. die Farbe rot zurück, wenn ein
das Ray einen Schnittpunkt gefunden hat und die Farbe blau (Himmel), wenn das Ray
ins leere gegangen ist. Damit sollten Sie gut sehen, ob Ihr Ray-Intersection Algorithmus
korrekt funktioniert.