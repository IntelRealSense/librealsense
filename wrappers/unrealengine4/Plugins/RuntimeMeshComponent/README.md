# UE4 Runtime Mesh Component

## The RMC takes a lot of effort to build, extend, and maintain. Please consider [supporting the project!](https://github.com/Koderz/RuntimeMeshComponent/wiki/Support-the-development!)


**There's a Discord server for the RMC!  https://discord.gg/KGvBBTv**

**Examples project can be found here https://github.com/Koderz/RuntimeMeshComponent-Examples**

***



**The RuntimeMeshComponent, or RMC for short, is a component designed specifically to support rendering and collision on meshes generated at runtime. This could be anything from voxel engines like Minecraft, to custom model viewers, or just supporting loading user models for things like modding. It has numerous different features to support most of the normal rendering needs of a game, as well as ability to support both static collision for things such as terrain, as well as dynamic collision for things that need to be able to move and bounce around!**

**Now, the RMC is very similar in purpose to the ProceduralMeshComponent or CustomMeshComponent currently found in UE4, but it far surpasses both in features, and efficiency! It on average uses 1/3 the memory of the ProceduralMeshComponent, while also being more efficient to render, and far faster to update mesh data. This is shown by the ability to update a 600k+ vertex mesh in real time! The RMC is also nearly 100% compatible with the ProceduralMeshComponent, while adding many features above what the PMC offers.**

*Current list of features*
* Full support for async collision cooking (See below for a known problem with UE4 regarding this)
* Brand new normal/tangent calculation utility that is several orders of magnitude faster
* Up to 8 UV channels
* High precision normals support 
* Collision only mesh sections.
* Tessellation support 
* Navigation mesh support 
* Fully configurable vertex structure 
* Ability to save individual sections or the entire RMC to disk 
* RMC <-> StaticMesHComponent conversions.  SMC -> RMC at runtime or in editor.  RMC -> SMC in editor.  
* Static render path for meshes that don't update frequently, this provides a slightly faster rendering performance.
* 70%+ memory reduction over the ProceduralMeshComponent and CustomMeshComponent
* Visibility and shadowing are configurable per section.
* Separate vertex positions for cases of only needing to update the position.
* Collision has lower overhead compared to ProceduralMeshComponent


**The RMC fully supports the cooking speed improvements of UE4.14 and UE4.17 including async cooking. As of right now, the RMC is forced to always use async cooking in a shipping build due to an engine bug which I've submitted a fix to Epic for**


For information on installation, usage and everything else, [please read the Wiki](https://github.com/Koderz/UE4RuntimeMeshComponent/wiki/)

**Some features that I'm looking into now that the core has been rewritten:**
* LOD (Probably with dithering support)
* Dithered transitions for mesh updates
* Instancing support (Probably ISMC style unless there's enough demand for HISMC style support)
* Mesh replication


**Supported Engine Versions:**
v1.2 supports engine versions 4.10+
v2.0 supports engine versions 4.12+
v3.0 supports engine versions 4.17+

*The Runtime Mesh Component should support all UE4 platforms.*
*Collision MAY NOT be available on some platforms (HTML5)*
