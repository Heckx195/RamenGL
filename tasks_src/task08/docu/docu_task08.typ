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
- Vor der main werden die vier Materialeigenschaften als vec3 deklariert: emissive, ambient, diffuse, specular, sowie der Shininess-Faktor als float.
- In der Render-Loop wird ein neues UI-Element erstellt, worin die vier vec3 verändert werden können, sowie der Shininess-Faktor und ein Toggle für Phong- vs. Gouraud-Shading.
- Die Werte werden über Uniforms an die beiden Shader gegeben, wobei die Material- und Lichteigenschaften miteinander multipliziert werden, um Multiplikationen pro Vertex oder Fragment in den Shadern zu vermeiden.

*Vertex-Shader*:
- In den Uniforms werden die vier Material- und Lichteigenschaften sowie der Shininess-Faktor übergeben
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
    // u_materialLightDiffuse = sorgt dafür, dass ein rotes Material auch nur rotes Licht reflektiert
```

*Fragment-Shader*:
- Für Gouraud-Shading wird die berechnete und interpolierte Farbe über die GLSL-Funktion mix() mit der Farbe des reflektierten Cubemap-Lichts gemischt, um die finale Fragmentfarbe zu berechnen
- Für Phong-Shading wird die Beleuchtungsberechnung im Fragment-Shader durchgeführt, wobei die interpolierten Normalenvektoren verwendet werden. Das Ergebnis wird dann mit der Farbe des reflektierten Cubemap-Lichts gemischt, um die finale Fragmentfarbe zu berechnen. Der gezeigte Code-Block ist identisch mit dem im Vertex-Shader, nur dass die interpolierten Normalenvektoren verwendet werden.
  - Der Vor- und Nachteil von Phong-Shading ist, dass es eine detailliertere Beleuchtung ermöglicht, aber auch mehr Rechenleistung benötigt, da die Beleuchtungsberechnung pro Fragment durchgeführt wird.

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
  caption: [Verschiedene Material- und Lichteigenschaften im Vergleich.],
)

#figure(
  image("50_reflection.png", height: 40%),
  caption: [50% Reflektionsanteil: Die reflektierte Umgebung ist deutlich sichtbar, gleichzeitig bleiben Material- und Lichteigenschaften noch klar erkennbar.],
)

=== Frage 1.:
Erläutern Sie, wie sich die Berechnung pro Fragment
im Gegensatz zur Lichtberechnung pro Vertex unterscheidet und wie
sich die beiden Verfahren nennen.
Welche visuellen Unterschiede gibt es und wann sind diese besonders
auffällig? Was ist gerade bei der Berechnung pro Fragment
im Fragmentshader zu beachten?

-> *Lösung*:
Berechnung pro Vertex:
- Gouraud Shading
- Es müssen weniger Berechnungen durchgeführt werden, weil das Ergebnis der Lichtberechnung durch den Rasterizer auf die Fragmente interpoliert wird
- Ein visueller Unterschied ist erkennbar, da die Auflösung auf Vertex-Ebene geringer ist als auf Fragment-Ebene, wodurch klare Kanten zwischen den Vertices sichtbar werden können. Außerdem können kleinere, scharfe Glanzlichter bei grob tessellierten Objekten zwischen den Vertices verloren gehen

Berechnung pro Fragment:
- Phong-Shading
- An den Fragment-Shader werden die interpolierten Normalenvektoren weitergegeben, worauf basierend die Lichtberechnung stattfindet
- Dadurch ist die Auflösung der Lichtflächen höher und es entstehen weniger deutlich sichtbare Kanten
- Bei der Berechnung im Fragmentshader ist darauf zu achten, dass die Normalenvektoren nach dem Rasterizer nicht mehr normalisiert sein können, und daher im Fragmentshader normalisiert werden müssen

#line(length: 100%)

== 8.1) Normalmapping
Erweitern Sie Ihren Renderer nun um Bumpmapping mithilfe
einer Normalmap.

=== Implementierung
- Es wird zuerst die unitplane als Objekt geladen und die dazuliegende Normalmap als Textur geladen.
  - Wichtig dabei ist es, die Normalmap-Textur vertikal zu spiegeln, da OpenGLs Texturkoordinaten Zeile 0 = Bildunterkante erwarten, während stb_image die Bilddaten mit Zeile 0 = Bildoberkante liefert. Dies wird mit der Funktion `stbi_set_flip_vertically_on_load(true)` erreicht (unmittelbar vor dem `Load()`-Aufruf gesetzt und danach wieder zurückgesetzt, da das Flag global für stb_image gilt).
- Die Normalmap-Textur wird mit diesen Parametern initialisiert:
```c
glTextureParameteri(textureHandleNormalMap, GL_TEXTURE_WRAP_S, GL_REPEAT);
glTextureParameteri(textureHandleNormalMap, GL_TEXTURE_WRAP_T, GL_REPEAT);
glTextureParameteri(textureHandleNormalMap, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTextureParameteri(textureHandleNormalMap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
```
  - Es wird `GL_REPEAT` für die Wrap-Parameter verwendet, damit die Textur wiederholt wird, wenn die Texturkoordinaten außerhalb des Bereichs [0, 1] liegen. Dies ist nützlich, wenn die Normalmap auf eine größere Fläche als die Texturgröße angewendet wird.
  - Die Normalmap wird wie jede andere Textur über die Fläche gekachelt.
- In der Render-Loop wird ein neues UI-Element erstellt, um Bumpmapping zu aktivieren oder zu deaktivieren. Der Wert wird über eine Uniform an den Fragment-Shader übergeben, um die Normalmap nur dann zu verwenden, wenn Bumpmapping aktiviert ist.
  - UV-Koordinaten, Tangent- und Bitangentvektoren hängen als Vertex-Attribute am VAO und werden bei jedem Draw-Call automatisch an die Shader übergeben, vorausgesetzt die Location-Nummern stimmen zwischen VAO und Shader exakt überein

*Vertex-Shader*:
- Es werden zusätzlich die UV-Koordinaten, Tangent- und Bitangentvektoren als Attribute an den Vertex-Shader übergeben. Die UV-Koordinaten werden unverändert an den Fragment-Shader weitergegeben, während die Tangent- und Bitangentvektoren davor in Weltkoordinaten transformiert werden.

*Fragment-Shader*:
- Mit den UV-Koordinaten wird die Normalmap-Textur ausgelesen und der daraus resultierende Normalenvektor im Tangentspace wird in den Bereich [-1, 1] transformiert. Anschließend wird aus dem Tangent-, Bitangent- und Normalenvektor des Fragments (alle in Weltkoordinaten) die TBN-Matrix (Tangent, Bitangent, Normal) erstellt und mit dem Normalenvektor im Tangentspace multipliziert, um die Weltraum-Normale zu erhalten. Die neue Weltraum-Normale muss normalisiert werden, bevor sie in der Beleuchtungsberechnung verwendet wird.
- Anschließend wird diese Weltraum-Normale wie gewohnt im Fragment-Shader weiterverwendet. Bei aktiviertem Phong-Shading, wo die Berechnung pro Fragment durchgeführt wird, zeigt sich die Veränderung deutlich: Anstelle der bisherigen (flachen) Normale wird jetzt der aus der Normalmap ausgelesene Normalenvektor verwendet, wodurch die Wand Unebenheiten aufweist und Licht sowie Schatten realistischer auf der Oberfläche verteilt werden.

=== Ergebnis
#figure(
  image("bump.png", height: 40%),
  caption: [Visualisierung der Normalmap auf der unitplane. Die aus der Normalmap ausgelesenen Normalen verändern die Beleuchtungsberechnung, wodurch die eigentlich flache Oberfläche Unebenheiten aufweist.],
)

=== Frage 1.:
Liesse sich die Beleuchtungsberechnung auch im Tangentspace durchführen?

-> *Lösung*:
- Ja: Indem man Lichtposition und Kameraposition mittels der transponierten TBN-Matrix in den Tangentspace transformiert, kann man die komplette Lichtberechnung dort durchführen, ohne die aus der Normalmap gelesenen Normalenvektoren in Weltkoordinaten transformieren zu müssen:
  - Variante 1 (in Weltkoordinaten):
    - Normalenvektoren aus der Normalmap in Weltkoordinaten transformieren (TBN-Matrix)
    - Für jedes einzelne Fragment müssen die Normalenvektoren in Weltkoordinaten transformiert werden, was bei vielen Fragmenten zu einer hohen Rechenlast führen kann
    - Lichtberechnung in Weltkoordinaten durchführen
  - Variante 2 (In Tangent Space): (bevorzugt in der Praxis)
    - Lichtposition und Kameraposition in Tangent Space transformieren (transponierte TBN-Matrix)
    - Das kann einmal pro Vertex durchgeführt werden und dann vom Rasterizer interpoliert werden
    - Lichtberechnung in Tangent Space durchführen

=== Frage 2.:
Ist es nötig sowohl Tangent- als auch Bitangentvektoren in den Shader
zu laden, oder reicht es einen von beiden mitzuliefern? Begründen Sie
Ihre Antwort.

-> *Lösung*:
- Es könnte theoretisch nur der Tangentvektor geladen werden, da der Bitangentvektor über das Kreuzprodukt aus Tangent- und Normalenvektor berechnet werden kann. Allerdings kann es bei den UV-Koordinaten symmetrischer Objekte (gespiegelte Texturkoordinaten) zu einem Wechsel zwischen Rechts- und Linkssystem kommen, wodurch das Kreuzprodukt in die falsche Richtung zeigen würde. Daher ist es sicherer, beide Vektoren direkt zu laden. Noch speichereffizienter wäre es, zusätzlich zum Tangentvektor nur ein einzelnes Vorzeichen-Bit zu laden, das angibt, ob ein Rechts- oder Linkssystem vorliegt, und den Bitangentvektor dann im Shader über das (vorzeichenkorrigierte) Kreuzprodukt zu berechnen.

#line(length: 100%)

== 8.2) Zusatzfragen
In diesem Kapitel sind Fragen zusammengefasst, die sich während der Implementierung ergeben haben (Nützlich für Prüfungsvorbereitung).

=== Frage 1.:
Das Phong-Beleuchtungsmodell setzt sich aus vier Anteilen zusammen: emissive, ambient, diffuse, specular. Erkläre kurz, was diese vier Anteile bedeuten und wovon sie abhängen (z. B. von Normale, Lichtrichtung, Blickrichtung)?

-> *Lösung*:
- Emissiv: Selbstrahlend, Objekt besitzt unabhängig einer Lichtquelle eine eigene Leuchtkraft (bspw. Monitor, Ampel). Ein fester Faktor in der Beleuchtungsgleichung
- Ambient: Fester globaler Wert des Hintergrundlichts. Wird zur Beleuchtungsgleichung addiert. Dadurch sind Kernschatten nicht komplett schwarz. Soll die möglichen Lichtreflexionen eines globalen Beleuchtungsmodells annähern.
  - konstanter Faktor (Materialfarbe x Lichtfarbe), unabhängig von Geometrie/Blickwinkel
- Diffuse: Eigentliche Beleuchtung der Oberfläche basierend auf dem Lambertschen Gesetz. Berechnet die Stärke der Beleuchtung basierend auf dem Winkel der Vertexnormalen zum Lichtrichtungsvektor.
- Spekular: Zuständig für helle Highlights auf der Oberfläche (Glanzlichteffekt), da eine Oberfläche nicht perfekt diffus ist und das eintreffende Licht relativ gerichtet reflektiert. Man muss die Ausrichtung der Oberfläche zur Kamera berechnen, und wenn der Blickwinkel in etwa dem Reflexionswinkel des eintreffenden Lichts entspricht, kann die Kamera das Highlight sehen. Bei Blinn-Phong wird dafür der Halfway-Vektor H = normalize(Lichtvektor + Vektor von Vertex zur Kamera) gebildet und mit dot(N, H) verrechnet. Über den Shininess-Exponent wird die Größe des Highlights bestimmt: Je größer der Exponent, desto kleiner und intensiver das Highlight (matte Fläche vs. glatte Fläche wie Lack).
  - pow(dot(N,H), shininess)

=== Frage 2.:
Liste auf, welche Vektoren (N, L, V, H, ...) in die Diffuse-Berechnung eingehen und welche in die Specular-Berechnung. Was ist der entscheidende Unterschied in dieser Liste, der erklärt, warum Diffuse kameraunabhängig, Specular aber kameraabhängig ist?

-> *Lösung*:
- Für die Diffuse-Berechnung werden N und L benötigt. Es wird der Kosinus zwischen beiden gemessen. Je kleiner der Winkel ist, desto stärker ist der Vertex beleuchtet.
  - Diffuse ist kameraunabhängig, da der Kameravektor von der Oberfläche zur Kamera (V) nicht in die Berechnung einfließt.
- Für die Spekular-Berechnung werden zuerst V und L benötigt, um daraus den Halfway-Vektor H zu bilden. Damit kann man dann mit H und N den Spekular-Wert berechnen und mit dem Shininess-Faktor potenzieren.
  - Ist kameraabhängig, da der Halfway-Vektor unter anderem aus V berechnet wird.

=== Frage 3.:
Wie würde man die vier Terme (Materialeigenschaften k_e, k_a, k_d, k_s) zu einer finalen Fragmentfarbe zusammensetzen? Welche Terme werden addiert, welche multipliziert, und wo genau kommt der dot(N,L)- bzw. pow(dot(N,H), shininess)-Faktor rein?

-> *Lösung*:
- Die einzelnen Eigenschaften des Materials und der Lichtquelle werden jeweils miteinander multipliziert, und die vier resultierenden Terme werden anschließend addiert. Zum Diffuse-Anteil wird zusätzlich der Faktor max(dot(N,L), 0) multipliziert, beim Spekular-Anteil entsprechend pow(max(dot(N,H), 0), shininess).
  - max() wird benötigt, damit man nicht bei negativem Wert die Rückseite einer Oberfläche beleuchtet
- Multiplikation innerhalb der Phong-Terme (z. B. k_d * i_d * dot(N,L))
  - Alle Faktoren sind Abschwächungen desselben Lichtpfads.
  Licht kommt mit Intensität i_d an, wird vom Material anteilig absorbiert/reflektiert (k_d), und der Anteil, der überhaupt auf die Fläche trifft, hängt vom Winkel ab (dot(N,L)). Es ist eine Kette von "wie viel von X kommt durch", daher wird Multiplikation genutzt.

=== Frage 4.:
Wie kombiniert man die Reflectivity der Cubemap-Umgebung mit der Phong-Blinn-beleuchteten Materialfarbe? Welche Faktoren werden dabei berücksichtigt? Wie berechnet man daraus eine finale Fragmentfarbe?

-> *Lösung*:
- Dafür wird die GLSL-Funktion mix(litColor, reflectionColor, reflectivity) benutzt, um eine gewichtete Mischung zu erreichen. Eine spiegelnde Fläche sollte weiterhin die Umgebung spiegeln, auch wenn sie im Schatten liegt.

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
- Es gilt immer "Ziel minus Ursprung". Im ersten Beispiel wird daher der (korrekte) Lichtvektor berechnet, im zweiten Beispiel der (falsche, weil umgekehrte) Vektor von der Lichtquelle zum Vertex.

=== Frage 7.:
- Was speichert eine Normalmap in ihren RGB-Werten, im Gegensatz zu einer normalen Diffuse-/Farbtextur? 

-> *Lösung*:
- Eine Normalmap speichert in den RGB-Werten die XYZ-Komponenten der Normalenvektoren. Die Werte werden dabei in den Bereich [0, 1] transformiert, da Texturen nur positive Werte speichern können. Daher müssen im Shader die Normalenvektoren wieder in den Bereich [-1, 1] zurücktransformiert werden.
  - Kodierung beim Erstellen der Textur: gespeichert = normal \* 0.5 + 0.5
  - Dekodierung im Shader: normal = gespeichert \* 2.0 - 1.0

=== Frage 8.:
Warum kann man den aus der Textur ausgelesenen Vektor nicht einfach direkt als Weltraum-Normale in die Beleuchtungsformel einsetzen? Was würde passieren, wenn man z.B. dieselbe Normalmap auf zwei unterschiedlich rotierte Flächen anwendest (z.B. den Boden und eine Wand) und den ausgelesenen Vektor jeweils direkt als N in Weltkoordinaten benutzt, ohne ihn vorher umzurechnen?

-> *Lösung*:
- Die Normalmap wird generisch relativ zu einer lokalen Ruherichtung gespeichert. Wird sie auf unterschiedlich ausgerichtete Flächen angewendet, ohne umgerechnet zu werden, entstehen dadurch Fehler. Man muss den aus der Textur gelesenen "lokalen" Vektor erst in die tatsächliche Ausrichtung der jeweiligen Fläche im Weltraum überführen. Diese lokale-zu-Welt-Transformation übernimmt die TBN-Matrix (Tangent, Bitangent, Normal): Tangent- und Bitangentvektor werden im Vertex-Shader in Weltkoordinaten transformiert und an den Fragment-Shader weitergegeben. Dort wird der aus der Textur gelesene Vektor mit der TBN-Matrix multipliziert, um die Weltraum-Normale zu erhalten
  - Die Normalmap liegt im Tangent Space
  - Normalmap.xyz stehen für die Ausrichtungen der Normalen im Tangent Space (x = Tangent, y = Bitangent, z = Normal)
  - Der Tangent Space wird durch Tangent (T), Bitangent (B) und Normal (N) aufgespannt. Die Tangent- und Bitangentvektoren werden aus den Texturkoordinaten berechnet. T zeigt in die Richtung der U-Texturkoordinate, B in die Richtung der V-Texturkoordinate. N ist die Oberflächennormale
  - Tangent- und Bitangentvektor werden zusätzlich zur ohnehin vorhandenen Normalen explizit geladen, weil das Berechnen des dritten Vektors über das Kreuzprodukt bei gespiegelten Texturkoordinaten (Wechsel zwischen Rechts- und Linkssystem) zu falschen Ergebnissen führen kann. Ein Bump würde dann in die falsche Richtung zeigen und als Vertiefung statt als Erhebung erscheinen