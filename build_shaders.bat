@echo off
if  not exist shaders mkdir shaders
fxc /T vs_5_0 /E vs_main /Fo shaders/VShader.cso src/ColourShader.hlsl -nologo
fxc /T ps_5_0 /E ps_main /Fo shaders/PShader.cso src/ColourShader.hlsl -nologo

fxc /T vs_5_0 /E vs_main /Fo shaders/VShaderFont.cso src/TexRender.hlsl -nologo
fxc /T ps_5_0 /E ps_main /Fo shaders/PShaderFont.cso src/TexRender.hlsl -nologo