@echo off
set outputDir=compiled_shaders
set inputDir=raw_shaders

if not exist "%outputDir%" md "%outputDir%"

@echo on
glslc.exe %inputDir%/shader.vert -o %outputDir%/vert.spv
glslc.exe %inputDir%/shader.frag -o %outputDir%/frag.spv

@echo off
echo compiled shaders were put into this directory: %outputDir%
pause