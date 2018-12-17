RayTra
===

Raytra is a raytracer written in C++

![shot.png](shot.png)

### Features
- Supports a simple scene description language ([sample scene](https://github.com/prakhar1989/raytra/blob/master/scenes/multiple_lights.scn))
- Renders an image in the [OpenEXR](http://www.openexr.com/) format.
- Ray acceleration tests using Bounded Volume Heirarchy (BVH) trees.
- Reflections and shadows
- Reading in geometry via Obj files
- Antialiasing via Monte Carlo sampling

### Run
```
$ mkdir build && cd build
$ cmake .. && make
$ raytra ../scenes/sample.scn image.exr
```
