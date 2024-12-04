# ⚡Taranis engine 💫

[![Build Status](https://github.com/PierreEVEN/Taranis/actions/workflows/build.yml/badge.svg)](https://github.com/PierreEVEN/Taranis/actions/workflows/build.yml)

Taranis is a custom game engine focused on the rendering of multi-planetary worlds.

This engine is built around vulkan. Other backends may be implemented later if necessary. The main goal for now is to handle all the features required to build a openworld game, solar system sized.

![Screenshot](./resources/screenshot.png)


## Build & Install

> Tested using latest msvc version

- install the [vulkan sdk](https://vulkan.lunarg.com/) (1.3.296 or later) 
- install the [xmake build system](https://xmake.io/#/)
- configure xmake `xmake f --build-tests=y` _(currently the only executable targets are test targets)_
- build `xmake build test_editor` 
- run `xmake run test_editor`

> Note : download example models and place them in `resource/models/samples/`
>  - Gltf sample models : https://github.com/KhronosGroup/glTF-Sample-Models
> - Bistro scene : https://developer.nvidia.com/orca/amazon-lumberyard-bistro

## Features and roadmap

- [x] Multi-render pass : add a simple interface to define a full render graph
- [x] Job system : add an async task scheduler
- [x] Asset registry : handle assets references and lifetime
- [x] MipMaps : automatically handle and generate textures mipmaps
- [x] Scene component allocator : allocate components using a custom allocator that optimise memory layout
- [x] Async import using asimp : import mesh and textures from gltf scenes using assimp
- [x] class inheritence reflection : Add an Unreal Engine - like reflection system (only class and inheritence)
- [x] slang backend : implement slang shader backend
- [x] Parallel command buffers recording : improve performances by splitting command buffer reccording in multiple tasks
- [x] Custom dynamic render passes : add or remove custom render passes on the fly (will be used to build shadow maps)
- [x] Frustum culling
- [ ] Custom render targets : Allow a shader to retrieve the result of a child render pass
- [ ] Directional Shadows (point shadows ?) : automatically handle multiple light sources with shadows
- [ ] Post process : add basic post process passes (bloom...)
- [ ] Camera-centric coordinates : We need to keep the camera at the origin to limite floating-point precision with fp32 on the gpu
- [ ] Planet landscape : Add 1:1 earth-like planet landscape (basic procedural generation)
- [ ] Planet atmosphere : Basic atmospheric scattering
- [ ] Foliages : Procedural foliages
- [ ] Try voxel rendering (test if voxel planet could be a good starting point)
- [ ] Property reflection : add properties to reflection system
- [ ] Serialization : automatically serialize assets using reflection

## Known issues

- Validation warning : vkCreateGraphicsPipelines(): pCreateInfos[0].pVertexInputState Vertex attribute not consumed by vertex shader.
- Texture are flipped to handle .dds format default orientation. Todo : flip imported .dds images

## Dependencies

- vulkan
- glfw
- imgui
- vulkan-memory-allocator
- freeimage
- slang
- glm
- assimp
