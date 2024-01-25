@echo off
fxc /T vs_5_0 /E vs_main /Fo shaders/VShader.cso src/Shader.hlsl -nologo
fxc /T ps_5_0 /E ps_main /Fo shaders/PShader.cso src/Shader.hlsl -nologo