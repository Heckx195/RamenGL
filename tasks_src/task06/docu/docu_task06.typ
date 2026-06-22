```shell
cmake --build build/
./build/Debug/task06.exe
```

#line(length: 100%)

= Aufgabe06
- https://docs.gl/
- https://learnopengl.com/Getting-started/Textures


== Renderingartefakte beim Schattenwurf
- Tiefenwerte in der Schattenkarte haben eine begrenzte Präzision, was zu stufenweiser Schattenbildung führen kann (sogenanntes "Shadow Acne").
  - Um dies zu verhindern, kann man die Präzision der Tiefenwerte erhöhen

- Hinter der LightCamera liegt nur Schatten
  - if (projCoords.z <= 1.0) { Schatten berechnen } else { kein Schatten }

- Shadow Acne: Objekt ist gleichzeitig Schattenwerfend (im Shadow Pass) und Schattenempfänger (im Hauptpass)
  - Lösung: Bias hinzufügen, um die Tiefenwerte der Schattenkarte leicht zu erhöhen, damit sie nicht mit den Tiefenwerten der Szene kollidieren.