# MySVG
Library for parsing and editing svg files.  

C++14 required

**TODO:**
- `ClipPaths`
- `CSS Style`
- `Animations`
- `Text`

## Examples
Building svg document from file

```cpp
#include <MySVG/Parser.h>
...
Svg::Document doc;
Svg::Parser<char>::Create()
   .SetDocument(&doc)
   .Parse("file.svg");
```

Building svg document from source

```cpp
#include <MySVG/Elements.h>
...
Svg::Document doc;
doc.svg->width  = 400;
doc.svg->height = 400;

auto rect = doc.svg->Make<Svg::RectElement>();
rect->x = 100;
rect->y = 100;
rect->width  = 300;
rect->height = 300;

auto color = doc.refs.Make<ColorElement>(255, 0, 0);
rect->GetStyle()->fill.data = color;
```

The code above is equivalent to this svg file

```svg
<svg width="400" height="400"
    xmlns="http://www.w3.org/2000/svg">
    <rect x="100" y="100" width="300" height="300"
        fill="rgb(255, 0, 0)"/>
</svg>
```

## Rendering the document
The following renderers are currently supported:
- `Blend2d`

They are in the **bindings/Renderers/** folder  
An example can be found in the **examples/** folder
