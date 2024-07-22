Real-time rendering engine written in C++ and OpenGL 4.3.

![OpenGL project cover](data/img/TP_OpenGL.jpg)

Implemented features:
- ImGui integration
- Frustum culling
- Shadow mapping (Percentage closer filtering)
- Microfacet BRDF : Metallic and roughness
![roughness](data/img/roughness.jpg)
- Textures : Diffuse, mettalic, roughness, normals (normal mapping)
- Normal mapping (left with, right without)
![normal mapping](data/img/normal_mapping.jpg)
- Irradiance Mapping (precomputation of the diffuse irradiance component from an environment map)
![irradiance map](data/img/irradiance_map.jpg)
- Skyspheres & skyboxes support
- HDR tone mapping (gamma et exposition)
