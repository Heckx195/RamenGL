```shell
cmake --build build/
./build/Debug/task08.exe
```

#line(length: 100%)

= Aufgabe08
- https://docs.gl/

#line(length: 100%)

== 8.0) Phong-Blinn Beleuchtungsmodell
Erweitern Sie Ihren Renderer um das Phong-Blinn Beleuchtungsmodell.
Führen Sie die Beleuchtungsberechnungen sowohl *pro Vertex* als
auch *pro Fragment* durch (*Edit: Beleuchtungsrechnung in Vertex-Shader, Anwenden des Schattens in Fragment-Shader*). Implementieren Sie ein UI-Element,
sodass Sie zwischen den beiden Methoden umschalten können.

=== Implementierung
- Vor der main werden die vier Variablen als vec3 deklariert: emissive, ambient, diffuse, specular. Sowie der Shineness-Faktor als float
- In der Render-Loop wird ein neue UI-Element erstellt, worin die vier vec3 verändert werden könnne, sowie Shineness-Faktor und einen Toggle für Phong- vs. Gouraud-Shading.
- Die Werte werden über Uniforms an die beiden Shader gegeben, wobei die Material- und Lichteigenschaften miteinander multipliziert werden, um Multiplikationen pro Vertex oder Fragments in den Shadern zu vermeiden

Vertex-Shader:
- In den Uniforms werden die vier Material/- und Lichteigenschaften und der Shineness-Faktor übergeben
- Für Gouraud-Shading wird im Vertex-Shader die Beleuchtungsberechnung durchgeführt und das Ergebnis als out_color an den Fragment-Shader weitergegeben, welches durch den Rasterizer interpoliert wird

```c
// Gouraud Shading: Normale pro Vertex berechnen
// emissive
vec3 emissive = u_materialEmissive;

// ambient
vec3 ambient = u_materialLightAmbient;

// diffus
vec3 N = normalize(out_WorldNormal);
vec3 L = normalize(u_lightPos - out_WorldPos); // Ziel minus Ursprung
float diffus = max(dot(N, L), 0.0);

// spekular
vec3 V = normalize(u_cameraPos - out_WorldPos); // Ziel minus Ursprung
vec3 H = normalize(L + V);
float spekular = max(dot(H, N), 0.0);
spekular = pow(spekular, u_shininess);

out_color = emissive + ambient + diffus * u_materialLightDiffuse + spekular * u_materialLightSpecular;
    // u_materialLightDiffuse = sorgt dafür das ein rotes Material auch nur rotes Licht reflektiert
```

Fragment-Shader:
- Für Gouraud-Shading wird die berechnete und interpolierte Farbe über die OpenGL-Funktion mix() mit der Farbe des reflektierten Cubemap-Lichts gemischt, um die finale Fragmentfarbe zu berechnen
- Für Phong-Shading wird die Beleuchtungsberechnung im Fragment-Shader durchgeführt, wobei die interpolierten Normalenvektoren verwendet werden. Das Ergebnis wird dann mit der Farbe des reflektierten Cubemap-Lichts gemischt, um die finale Fragmentfarbe zu berechnen. Der gezeigte Code-Block ist identisch mit dem im Vertex-Shader, nur dass die interpolierten Normalenvektoren verwendet werden.
  - Der Vor- und Nachteil von Phong-Shading ist, dass dieser einer detaillierten Beleuchtung ermöglicht, aber auch mehr Rechenleistung benötigt, da die Beleuchtungsberechnung pro Fragment durchgeführt wird.

=== Ergebnis

#figure(
  grid(
    columns: 2,
    column-gutter: 1em,
    row-gutter: 0.5em,
    image("./gouraud.png"), image("./phong.png"),
    align(
      center,
    )[(a) Gouraud-Shading],
    align(center)[(b) Phong-Shading],
  ),
  caption: [Vergleich zwischen Vertex- und Fragment-Shading. An der unterschiedlichen Auflösung des Highlightes ist der Unterschied gut zu erkennen.],
)

#figure(
  grid(
    columns: 2,
    column-gutter: 1em,
    row-gutter: 0.5em,
    image("./spekular.png"), image("./100_emissive.png"),
    align(
      center,
    )[(a) Rote Spekular-Highlights],
    align(center)[(b) 100% weißes Emissive: Verlust der Tiefenwahrnehmung],
  ),
  caption: [Unterschiedliche Auswahl verschiedener Material- Lichteigenschaften.],
)

#figure(
  image("50_reflection.png", height: 40%),
  caption: [50% Reflektionsanteil: Die reflektierte Umgebung ist deutlich sichtbar, gleichzeitig bleiben Material- und Lichteigenschaften noch klar erkennbar.],
)

=== Fragen 1.)
Erläutern Sie, wie sich die Berechnung pro Fragment
im Gegensatz zur Lichtberechnung pro Vertex unterscheidet und wie
sich die beiden Verfahren nennen.
Welche visuellen Unterschiede gibt es und wann sind diese besonders
auffällig? Was ist gerade bei der Berechnung pro Fragment
im Fragmentshader zu beachten?

=== Lösung
Berechnung pro Vertex:
- Gouraud Shading
- Es müssen weniger Berechnungen durchgeführt werden, weil das Ergebnis der Lichtberechnung durch den Rasterizer auf die Fragemente interpoliert werden
- Ein visuellen Unterschied kann man erkennen, da die Auflösung von Vertex schlechter ist als Fragmente, dass klare Kanten zwischen den Vertexes erkannt werden können. Sowie kleinere scharfe Glanzlichter können bei grob tessellierten Objekten zwischen den Vertexes verloren gehen

Berechnung pro Fragment:
- Phong-Shading
- An den Fragment-Shader werden die interpolierten Normalenvektoren weitergegeben, worauf basierend die Lichtberechnung stattfindet
- Dadurch ist die Auflösung der Lichtflächen höher und es enstehen weniger klare Kanten
- Bei der Berechnung im Fragmentshader ist darauf zu achten, dass die Normalenvektoren nach dem Rasterizer nicht mehr normalisiert sein können, und daher im Fragmentshader normalisiert werden müssen

#line(length: 100%)

== 8.1) Normalmapping
Erweitern Sie Ihren Renderer nun um Bumpmapping mithilfe
einer Normalmap.

=== Implementierung
```c
ImGui::Checkbox("Aktiviere PCF", &usePCF);
```


=== Fragen 1.)
Liesse sich die Beleuchtungsberechnung auch im Tangentspace durchführen?
Ist es nötig sowohl Tangent- als auch Bitangentvektoren in den Shader
zu laden, oder reicht es einen von beiden mitzuliefern? Begründen Sie
Ihre Antwort.

#line(length: 100%)






== 7.3) Dokumentation

=== Frage 1.:
Das Phong-Beleuchtungsmodell setzt sich aus vier Anteilen zusammen: emissive, ambient, diffuse, specular. Erkläre kurz, was diese vier Anteile bedeuten und wovan sieh abhängen (von welchen Normale, Lichtrichtung, Blickrichtung)?

-> *Lösung*:
- Emissiv: Selbstrahlend, Objekt besitzt unabhängig einer Lichtquelle eine eigene Leuchtkraft (bspw. Monitor, Ampel). Ein fester Faktor in der Beleuchtungsgleichung
- Ambient: Fester globaler Wert des Hintergrundlichts. Wird in der Beleuchtungsgleichung darauf addiert. Dadurch sind Kernschatten nicht komplett Schwarz. Soll die möglichen Lichtreflektionen eines globalen Beleuchtungsmodell annähern.
  - konstanter Faktor (Materialfarbe x Lichtfarbe), unabhängig von Geometrie/Blickwinkel
- Diffuse: Eigentliche Beleuchtung der Oberfläche basierent auf dem Lambertschen Wert. Berechnet Stärke der Beleuchtung basierent auf dem Winkel der Vertexnormalen zum Lichtrichtungsvektor.
- Spekular: Zuständig für helle Highlights auf den Oberflächen (Glanzlichteffekt), da eine Oberfläche nicht perfekt diffus ist, reflektiviert diese das eintreffende Licht relativ gerichtet in eine Richtung. Man muss die Ausrichtung der Oberfläche zur Kamera berechnen, und wenn der Winkel in etwa mit dem Reflektionswinkel des eintreffenden Lichts auf der Oberfläche ist, kann die Kamera das Highlight sehen. Bei Blinn-Phong wird für die Berechnung der Halfway-Vektor: H = normalize(Lichtvektor + VektorVertixZuKamera) genommen und mit dot(N, H) verrechnet. Über den shininess-Exponent wird die Größe des Highlights bestimmt. Je größer der Exponent, desto kleiner und intensiver das Highlight (Matte Fläche vs. glatte Fläche wie Lack).
  - pow(dot(N,H), shininess)

=== Frage 2.:
Liste auf, welche Vektoren (N, L, V, H, ...) in die Diffuse-Berechnung eingehen und welche in die Specular-Berechnung. Was ist der entscheidende Unterschied in dieser Liste, der erklärt, warum Diffuse kameraunabhängig, Specular aber kameraabhängig ist?

-> *Lösung*:
- Für die Diffuse-Berechnung wird N und L benötigt. Es wird der Kosinus zwischen den beiden gemessen. So kleiner der Winkel ist, desto stärker ist der Vertex beleuchtet.
  - Diffuse ist kameraunabhängig, da der Kameravektor von der Oberfläche zur Kamera V nicht in die Berechnung einfließt.
- Für die Spekular-Berechnung wird zuerst für den Halfway-Vektor V und L benötigt. Dann kann ich mit H und N meine Spekular-Wert berechnen und mit dem Shineniss-Faktor potentieren
  - Ist Kamerabhängig, da der Halfway-Vektor aus V berechnen wird

=== Frage 3.:
Wie würde man die vier Terme (Materialeigenschaften k_e, k_a, k_d, k_s) zu einer finalen Fragmentfarbe zusammensetzen? Welche Terme werden addiert, welche multipliziert, und wo genau kommt der dot(N,L)- bzw. pow(dot(N,H), shininess)-Faktor rein?

-> *Lösung*:
- Die einzelnen Eigenschaften des Materials und der Lichtquelle werden miteinander multipliziert und zusammen addiert. Zum Diffusen-Teil wird die Beleuchtungsrechnung max(dot(N,L), 0) dran multipliziert. Gleiches beim Spekularen-Teil mit pow(max(dot(N,H), 0), shininess)
  - max() wird benötigt, dass man nicht bei negativen Wert die Rückseite einer Oberfläche beleuchtet
- "Multiplikation innerhalb der Phong-Terme (k_d * i_d * dot(N,L)): Dort sind alle drei Faktoren Abschwächungen desselben Lichtpfads – Licht kommt mit Intensität i_d an, wird vom Material anteilig absorbiert/reflektiert (k_d), und der Anteil, der überhaupt auf die Fläche trifft, hängt vom Winkel ab (dot(N,L)). Das ist eine Kette von "wie viel von X kommt durch" – daher Multiplikation."

=== Frage 4.:
Wie kombiniert man die Reflectivity der Cubemap-Umgebung mit der Phong-Blnn-beleuchteten Materialfarbe? Welche Faktoren werden dabei berücksichtigt? Wie berechnet man daraus eine finale Fragmentfarbe?

-> *Lösung*:
- Über die OpenGL Funktion mix() wird dafür benutzt, um die gewichtete Mischung zu erreichen. Eine spiegelnde Fläche sollte weiterhin spiegeln, auch wenn diese im Schatten liegt.

=== Frage 5.:
Wann muss ein Vektor normalisiert werden?

-> *Lösung*:
- Immer dann, wenn man mit Richtungen arbeitet, da sonst die Berechnung der Beleuchtung verfälscht wird. Vor allem nach der Interpolation im Fragmentshader, da die Normalenvektoren nicht mehr normalisiert sein können.
- Wenn man das Skalare Produkt zwischen zwei Vektoren berechnet, da sonst der Winkel zwischen den beiden Vektoren nicht korrekt berechnet werden kann.

=== Frage 6.:
Was ist der Unterschied bei der Berechnung von Richtungsvektoren zwischen:
  - vec3 L = normalize(u_lightPos - out_WorldPos);
  und
  - vec3 L = normalize(out_WorldPos - u_lightPos);

-> *Lösung*:
- Es ist immer "Ziel minus Ursprung" zu rechnen. Im ersten Beispiel wird daher der Lichtvektor berechnet, im zweiten Beispiel der Vektor von der Lichtquelle zum Vertex

#line(length: 100%)

= Ergebnis
// #figure(
//   grid(
//     columns: 2,
//     column-gutter: 1em,
//     row-gutter: 0.5em,
//     image("./task07_result.png"), image("./task07_resultPCF.png"),
//     align(
//       center,
//     )[(a) Schatten ohne PCF],
//     align(center)[(b) Schatten mit PCF],
//   ),
//   caption: [Vergleich zwischen mit und ohne PCF],
// )