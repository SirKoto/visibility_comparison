# Visibility algorithms comparison

The program, implemented in C++, generates a scene with a set of Armadillos (346k triangles, 173k vertices per model) on a grid. Then the camera follows a route that tries to emulate a lot of possible camera angles. This is used to benchmark the framerate obtained with the following algorithms:
- Unoptimized implementation
- View-frustum culling
- GPU Occlusion queries culling
- CHC++: Frustum culling+occlusion queries ([Link to paper](https://dcgi.fel.cvut.cz/home/bittner/publications/chc++.pdf))

![Program capture](images/capture.PNG)


For more information about compilation, usage, and obtained results, see the [Visibility_Report.pdf](Visibility_Report.pdf) file.

Developed for the Fast Realistic Rendering course of the Master in Innovation and Research in Informatics, at the Barcelona School of Informatics.