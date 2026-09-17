# Generated content

`Scripts/prepare_content.py`, executed inside Unreal Editor, creates the bootstrap map in `Maps/Przebudzenie.umap` and the generated materials, textures and audio under `Generated/`.

The runtime `ASliceWorld` creates the complete small gameplay space and interactables. The editor script creates a real map and navigation bounds so the project can be cooked; it does not replace Unreal packaging.

Generated `.uasset` and `.umap` files are reproducible build outputs and ignored by Git. All procedural texture/audio sources are authored in `Scripts/make_source_assets.py`. Engine BasicShapes are supplied by Unreal Engine and remain governed by Epic's license. No external marketplace or third-party assets are bundled.
