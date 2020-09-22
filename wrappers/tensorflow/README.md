1. Introduction (Sergey)
2. Installation
To install all relevant python packages, run the following command by using the package manager pip :

pip install -r "...\requirements.txt"
Find requirements.txt in installation folder. If you have GPU, install Tensorflow with GPU:

pip install tensorflow-gpu
pip install keras
Versions
Tensorflow-gpu - 2.2.0
Keras - 2.4.3

================================================================================================
Example 1 : explain how to download cocoset etc
Example 2: like 1, for each object from RGB (ex 1 - bounding box) take its depth (take example from sergey)
Example 3: like #1 but with opencv
Example 4: 
		4.1: Introduction to autoencoder (Unet Network)
		4.2 Downloading dataset and explanation about images
		4.3 explain about data preparation (augmentation, file tree, ..)
		4.4 training: epchs, strides, etc
		4.5 monitoring tensorboard 
How to use the tools: RMSE, conver to bag
Example 5: model from #4 or download, run on real data

3D (not a priority): "C:\work\git\denoise\wrappers\python\examples\pyglet_pointcloud_viewer.py"
3D : "C:\work\git\denoise\wrappers\python\examples\pyglet_pointcloud_viewer.py"
3D : "C:\work\git\denoise\wrappers\python\examples\opencv_pointcloud_viewer.py"

Example 6: How covert keras to frozen graph --> camera simulation as #5 but use only opencv



https://cocodataset.org/#home 
https://drive.google.com/file/d/1ZCnqb7OB5fkk4ba4lK2WZry3qu0RUZfZ/view?usp=sharing 