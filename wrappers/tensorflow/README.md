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
		-> Unet is a deep learning Architecture used for image segmentation problems
		It was particularly released as a solution for biomedical segmentation tasks
		but it became used for any segmentation problem
		Unlike classification problems that use hundred of layers, Unet represent a very compact architecture
		with limited number of parameters.
		self-driving cars for example use image segmentation for awareness about the environment.
		In this project, image segmentation is needed to know the exact depth of each object. 
		Unet network is the ideal architecture that serves our goal.
		Unlike image classification that classify the entire image, image segmentation classify every single pixel
		as to what class it belongs to.
		It is not enough to see if some feature is present or not, we need to know exactly the shape and allthe entricate 
		properities and how each object behaves, the shapes and forms it takes to exactly know which pixels belong to that.
		This kind of task requires the network to have considerably more in-depth understanding about the object.
		Before unet, segmentation tasks were approched using a modified convolution networks by replacing
		fully connected with fully convolutional layers then they were just upsample it to the same
		resolution of the image and try to use it as a segmentation mask. But because the image was compressed
		so much and it gone through so much processing: the max pools have thrown away some information.
		Just doing upsampling will not really give us the find of fine resolution that we want. It is very
		coarse and not really very accurate. so regular upsampling doesn't work very well.
		What the Unet does, the first (left) pathway of it looks very similar to class convolutional network
		instead of just directly upsampling, it has another pathway which it gradually builds upon the up sampling
		procedure, so it serves as directly up sampling via learning the best way that this compressed image should
		be up sampled and using convolution filters for that.
		Unet Architecture: input image which goes through some convolutions, then it is downsampled by using
		max pooling then it goes through more convolutions downsampled again, and so on until it reaches the deepest layer
		after that it is upsampled by half so we get H and W back, then we concatenate the features of each connection 
		then these concatenated features go through some more convolutions, then upsampled then it is joined (concatenated) back 
		with the paraller layer
		TODO :: PUT IMAGE HERE OF CONV NETWORK <conv networks then FC networks>
		
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