SVO Workbench
=============

Workbench for Raytracing of Sparse Voxel Octrees

This is a workbench I created to help me with research for my master thesis, [Efficient Ray Tracing of Sparse Voxel Octrees on an FPGA](http://brage.bibsys.no/xmlui/handle/11250/255856).

See this repository for the Verilog/FPGA-code for the same thesis: https://github.com/skyfex/svo-raycaster

The code is very messy in places, as it's not intented to be an actual product or library. But some of the code here may be useful to others doing similar work.

If you want to render the pologyn models in this worbench you need to fetch the original PLY-models from here: 
https://graphics.stanford.edu/data/3Dscanrep/

Put the models in data/ and rename them or update the awakeFromNib function in OctreeView.mm to point at the correct file. The models come in different resolutions, and any of them should be fine.

The included octree models (the .bin files in data/) was generated from those PLY models.

This project features:
* A software implementation of ray tracing of sparse voxel octree
** The latest version is written as a state machine that models the Verilog-version
** There is a copy of an older version which is more straigth-forward and should be easier to understand.
* Modeling of caching for hardware raytracing
* Demonstrating SW ray tracing and GPU rasterization rendering into the same framebuffer
* Export data to FPGA-version
* Push data to FPGA-version through fpgalink

![Screenshot](https://raw.githubusercontent.com/skyfex/svo-workbench/master/screenshot.png)
