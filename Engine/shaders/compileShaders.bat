@echo off

for %%f in (*.vert) do (
    echo Compiling %%f... ^> %%~nfVert.spv
    C:/VulkanSDK/1.4.328.1/Bin/glslc.exe "%%f" -o "%%~nfVert.spv"
)

for %%f in (*.frag) do (
    echo Compiling %%f... ^> %%~nfFrag.spv
    C:/VulkanSDK/1.4.328.1/Bin/glslc.exe "%%f" -o "%%~nfFrag.spv"
)

pause