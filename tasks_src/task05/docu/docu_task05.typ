```shell
cmake --build build/
./build/Debug/task05.exe
```

#line(length: 100%)

= Aufgabe05
- https://docs.gl/
- https://learnopengl.com/Advanced-OpenGL/Cubemaps

== 5.1.0) Erstellen der Cubemap
Erstellen Sie sich die Vertexdaten für einen Würfel. Achten Sie aber arauf, dass sich der Ursprung des Würfels in dessen Schwerpunktmittelpunkt befindet und die Frontfaces des Würfels nach innen zeigen. Schreiben Sie einen Vertex- und Fragmentshader, der Ihnen den Würfel zeichnet.

=== Implementierung
Wiederverwendung des Würfels aus Aufgabe 03 mit der Änderung, dass die Frontfaces nach innen zeigen. Dafür wurde die Vertex-Wending-Order angepasst, sodass die Vertices im Uhrzeigersinn von außen nach innen definiert werden. Ebenso wurden die Normalenvektoren angepasst, damit sie nach innen zum Ursprung zeigen.

== 5.1.1) Erstellen der 3D Textur
Erzeugen Sie nun eine neue OpenGL Textur (`glCreateTextures`). Diesmal handelt es sich aber um den Typ: `GL_TEXTURE_CUBEMAP`.Diese Texturart erlaubt es Ihnen im Shader mit einem 3D-Richtungsvektor von den sechs Seiten der Cubemap zu samplen. Den Texturspeicher legen Sie mit `glTextureStorage2D` an. Nun können Sie die sechs geladenen Bilder mithilfe der Funktion `glTextureSubImage3D` in den angelegten Speicher der Grafikkarte laden. Nehmen Sie in Ihre Dokumentation mit auf für was die Parameter von `glTextureSubImage3D` stehen und wie Sie vorgegangen sind. 

=== Implementierung


=== Frage 1.):
Spielt die Reihenfolge, mit der Sie die einzelnen Bilder hochladen, eine Rolle?

=== Lösung
- Die Reihenfolge beim Hochladen der sechs Bilder spielt in dem Sinne nur eine Rolle, dass sie zum richtigen Enum zugeordnert werden müssen
 - i=0 -> GL_TEXTURE_CUBE_MAP_POSITIVE_X
 - i=1 -> GL_TEXTURE_CUBE_MAP_NEGATIVE_X
 - i=2 -> GL_TEXTURE_CUBE_MAP_POSITIVE_Y
 - i=3 -> GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
 - i=4 -> GL_TEXTURE_CUBE_MAP_POSITIVE_Z
 - i=5 -> GL_TEXTURE_CUBE_MAP_NEGATIVE_Z

#line(length: 100%)

```c
void glTextureSubImage3D(
  GLuint texture,
  GLint level,
  GLint xoffset,
  GLint yoffset,
  GLint zoffset,
  GLsizei width,
  GLsizei height,
  GLsizei depth,
  GLenum format,
  GLenum type,
  const void *pixels
);
```

- texture: Name/ ID vom Textur-Objekt.
- level: Mipmap-Level der Textur, die aktualisiert werden soll (0 für die Basis-Textur).
- xoffset, yoffset, zoffset: Offset in Pixeln, der angibt, wo die Aktualisierung innerhalb der Textur beginnen soll.
- width, height, depth: Breite, Höhe und Tiefe des Bereichs, der aktualisiert werden soll, in Pixeln.
- format: Format der Pixel-Daten (z.B. GL_RGBA, GL_RGB).
- type: Typ der Pixel-Daten (z.B. GL_UNSIGNED_BYTE).
- \*pixels: Zeiger auf die Pixel-Daten.


#line(length: 100%)
*Kopie aus Docu04:*

```c
void glTextureParameteri(
  GLuint texture,
  GLenum pname,
  GLint param
);
```

- texture: Name/ ID vom Textur-Objekt.
- pname: Parameter, der gesetzt werden soll (z.B. GL_TEXTURE_WRAP_S, GL_TEXTURE_MIN_FILTER).
- param: Wert, der für den angegebenen Parameter gesetzt werden soll (z.B. GL_CLAMP_TO_EDGE, GL_NEAREST).

Wird verwendet, um Parameter für ein bereits existierendes Textur-Objekt zu setzen.

glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
- Wrap-S -> S steht für die horizontale UV-Koordinate.
- Definiert den Fall, wenn UV-Koordinaten außerhalb des Bereichs [0, 1] liegen. GL_CLAMP_TO_EDGE sorgt dafür, dass für alle UV-Koordinaten außerhalb des Bereichs die äußerste Randfarbe der Textur verwendet wird.

glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
- Wrap-T -> T steht für die vertikale UV-Koordinate.
- Definiert den Fall, wenn UV-Koordinaten außerhalb des Bereichs [0, 1] liegen. GL_CLAMP_TO_EDGE sorgt dafür, dass für alle UV-Koordinaten außerhalb des Bereichs die äußerste Randfarbe der Textur verwendet wird.

glTextureParameteri(textureHandle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
- Min-Filter -> Definiert die Filtermethode, die verwendet wird, wenn die Textur verkleinert dargestellt wird (d.h. wenn mehrere Texel auf einen Pixel abgebildet werden). GL_NEAREST sorgt dafür, dass der nächstgelegene Texel-Wert verwendet wird.

glTextureParameteri(textureHandle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
- Mag-Filter -> Definiert die Filtermethode, die verwendet wird, wenn die Textur vergrößert dargestellt wird (d.h. wenn ein Texel auf mehrere Pixel abgebildet wird). GL_NEAREST sorgt dafür, dass der nächstgelegene Texel-Wert verwendet wird.

#line(length: 100%)

== 5.3) FPS Camera
Ändern Sie nun das Verhalten der Cubemap so,
dass Sie diese nicht mehr verlassen können! Dies soll den Eindruck
eines unendlich entfernt liegenden Horizonts simulieren, wie
es auch in vielen Videospielen der Fall ist. 

```c
glDepthMask(GL_FALSE);
```

- Deaktiviert das Schreiben in den Tiefenpuffer. Dadurch wird verhindert, dass die Cubemap-Geometrie die Tiefeninformationen überschreibt, was wichtig ist, um sicherzustellen, dass andere Objekte korrekt *vor* der Cubemap gerendert werden können.