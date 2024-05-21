# PencilAnnealing
Basic conversion of photos to pen and ink sketches with simulated annealing.

Drawing|Optimization
--|--
![drawing](https://github.com/theoreticalphysicsftw/PencilAnnealing/assets/10967184/08a17c0c-fc4c-4186-a160-b2a373edfbaa) | ![optimization](https://github.com/theoreticalphysicsftw/PencilAnnealing/assets/10967184/ce8a649b-73ad-4454-865f-d17acd492e46)

## Building and Usage

Build with:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config=Release
```

To use just put a WebP picture named `in.webp` inside the executable folder and start it. I till produce `optimization.ogv` video with the optimization process `out.svg` vector graphics output of the final result and `out.ogv` drawing the picture.

**WIP: Make better UI.**

## Showcase

https://github.com/theoreticalphysicsftw/PencilAnnealing/assets/10967184/30027fa5-61a7-4b18-a925-fe5b55fa8031


