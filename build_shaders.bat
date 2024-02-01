@echo off
fxc /T vs_5_0 /E vs_main /Fo shaders/VShader.cso src/ColourShader.hlsl -nologo
fxc /T ps_5_0 /E ps_main /Fo shaders/PShader.cso src/ColourShader.hlsl -nologo

fxc /T vs_5_0 /E vs_main /Fo shaders/VShaderFont.cso src/FontRenderBMP.hlsl -nologo
fxc /T ps_5_0 /E ps_main /Fo shaders/PShaderFont.cso src/FontRenderBMP.hlsl -nologo