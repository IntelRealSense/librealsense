# Intel® RealSense™ SDK - GitHub Pages

This branch is used to generate [intelrealsense.github.io/librealsense](https://intelrealsense.github.io/librealsense/index.html) - GitHub Pages for the SDK. 
The goal is having single place for latest download links, documentation, news and examples catalog. 

* To modify catalog content, please edit `news.json` / `examples.json`
* To modify page appearance, please edit `templates/index.html` / `templates/examples.html`
* To re-generate top level `html` files, run `python generate.py templates/index.html news.json` / `python generate.py templates/examples.html examples.json`

