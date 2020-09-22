# Introduction
# Installation
To install all relevant python packages, run the following command by using the package manager pip :

	pip install -r "...\requirements.txt"

Find requirements.txt in installation folder. If you have GPU, install Tensorflow with GPU:

	pip install tensorflow-gpu
	pip install keras
### Versions
	Tensorflow-gpu - 2.2.0
	Keras - 2.4.3

# Examples
Set of example showing the use of Tensorflow.

## Example 1 
explain how to download cocoset etc


## Example 2
like 1, for each object from RGB (ex 1 - bounding box) take its depth (take example from sergey)


## Example 3
like #1 but with opencv


## Example 4
#### Unet Network Introduction
Unet is a deep learning Architecture used for image segmentation problems
It was particularly released as a solution for biomedical segmentation tasks
but it became used for any segmentation problem. Unlike classification problems that use hundred of layers, Unet represent a very compact architecture
with limited number of parameters.
self-driving cars for example use image segmentation for awareness about the environment.
In this project, image segmentation is needed to know the exact depth of each object. 
Unet network is the ideal architecture that serves our goal.

Unlike image classification that classify the entire image, image segmentation classify every single pixel
and to what class it belongs to.
It is not enough to see if some feature is present or not, we need to know the exact shape and all the entricate 
properities: how each object behaves, the shapes and forms it takes to exactly know which pixels belong to that object.

This kind of task requires the network to have considerably more in-depth understanding about the object.
Before Unet, segmentation tasks were approched using a modified convolution networks by replacing
fully connected with fully convolutional layers then they were just upsample it to the same
resolution of the image and try to use it as a segmentation mask, but because the image was compressed
so much and it gone through so much processing (e.g max pooling and convolutions) a lot of information have thrown away.

Just doing upsampling will not really give us the fine resolution that we want. It is very
coarse and not very accurate. so regular upsampling doesn't work very well.
What Unet does, the first (left) pathway of it looks very similar to classic convolutional network
instead of just directly upsampling, it has another pathway which it gradually builds upon the up sampling
procedure, so it serves as directly up sampling via learning the best way that this compressed image should
be up sampled and using convolution filters for that.
		
#### Unet Network Architecture: 
Input image goes through some convolutions, then it is downsampled by using
max pooling then it goes through more convolutions, downsampled again, and so on until it reaches the deepest layer
after that it is upsampled by half so we get back the sizes (see image below), then we concatenate the features of each connection 
and these concatenated features go through some more convolutions, then upsampled then it is joined (concatenated) back 
with the parallel layer, but we lose information as we go down through max pooling (mostly because it reduces the dimention by half), and
also through convolution because convolution throw away information from raw input to repupose them into
meaningful features. That what happens also in classification networks, where a lot of information is 
thrown away by the last layer. But in segmentation we want those low-level features because those
are essential to deconstructing the image. 

In the left pathway of Unet, the number of filters (features) increase as we go down, it means that it becomes
very good at detecting more and more features, the first few layers of a convolution network capture a very small semantic information and lower level
features, as you go down these features become larger and larger, but when we throw away information the CNN
knows only approximate location about where those features are.
When we upsample we get the lost information back (by the concatination process)
so we can see last-layer features in the perspective of the layer above them.



TODO :: PUT IMAGE HERE OF CONV NETWORK <conv networks then FC networks> and UNET
		
#### Training Dataset
The dataset is located here: https://drive.google.com/file/d/1cXJRD4GjsGnfXtjFzmtLdMTgFXUujrRw/view?usp=drivesdk
It containes 3 types of 484x480 png images : 
##### Ground Truth Images
- clean depth images that Neural Network should learn to predict. 
- 1-channel image of 16 bits depth
- name : gt-*.png
##### Depth Images : 
- noisy depth images as captured by ds5.
- 1-channel image of 16 bits depth
- name : res-*.png
##### Infra Red (IR) Images 
- used to help Unet learning the exact depth of each object
- 3-channel image of 8 bits depth for each channel. 
- name : left-*.png
		
#### Data Augmentation
To help the neural network learning images features, the images should be cropped to optimal size.
Large images contain many features, it requires adding more layers to detect all the features, this will impact the learning process negatively.
On the other hand, very small images may not contain any explicit feature, because most likely each feature would be splitted to several number of images.
It is very essential to choose the cropped images size optimally, considering the original size the average size of features inside the image.
In the set of experiments we did, image size of 128x128 found to be optimal.

Each ground truth image has a corressponding depth and infra red (IR) image. Given that, the dataset is augmented as following:
IR images are converted to 1-channel image of 16-bits depth.
##### Cropping 
Each image in the dataset is padded to get a size of 896x512 then each of them is cropped to 128x128. In total, each image is cropped to 28 images of size 128x128.  
Each cropped image is saved with the original image name, adding to it information about the column and row the image was cropped from. It helps corresponding to each ground-truth cropped-image, the IR and depth image from the cropped set.
##### Channeling
IR (infra red) image is added as a second channel to both ground truth and depth image, to add more information about the depth of each object in the image.

Eventually, the data that is fed to Unet network contains:
- Noisy images: 
consistes of 2 channels: first channel is a depth image and second channel is the corressponding IR image
- Pure images: 
consistes of 2 channels: first channel is a ground truth image and second channel is the corressponding IR image. 
Each channel in both pure and noisy is a 16-bits depth.

#### Training Process
In order to start a training process, the following is required:
- Unet Network Implementation : choosing the sizes of convolutions, max pools, filters and strides, along downsampling and upsampling.
- Data Augmentation: preparing dataset that contains noisy and pure images as explained above.
- Old model (optional): there is an option of training the network starting from a previously-trained model. 
- Epochs : epoch is one cycle through the full training dataset (forth and back). The default value of epochs number is 100, it could be contolled by an argument passed to the application.

#### Files Tree 
the application will create automatically a file tree:
- images folder: contains original and cropped images for training and testing, also the predicted images
- logs folder: all tensorflow outputs throughout the training are stored in txt file that has same name as the created model. It contains also a separate log for testing statistics.
- models folder: each time a training process starts, it creates a new folder for a model inside models folder. If the traing starts with old model, 
				 it will create a folder with same name as old model adding to it the string "_new"
		
		.
		├───images
		│   ├───denoised
		│   ├───test
		│   ├───test_cropped
		│   ├───train
		│   ├───train_cropped
		│   └───train_masked
		├───logs
		└───models
			└───DEPTH_20200910-203131.model
				├───assets
				└───variables
		
####  Testing Process
The tested images should be augmented as trained images, except the cropping size should be 480x480 (each image is cropped to 2 images), considering performance improvement.
For testing, there is no need to ground truth data, only depth and IR images are required.
The relevant folders in file tree are: 
- test: original images to test of sizes 848x480
- test_cropped: cropped testing images, size: 480x480
- denoised: the application stores predicted images in this folder.
		
		

#### monitoring tensorboard 
		
# Tools
##  RMSE
Used to show surface smoothness by showing RMSE of pixels inside a selected rectangle on image.
The Whiter the pixels, the more far they are.
Smooth surface will result in homogeneous pixels color in the selected rectangle
 TODO :: add image
 
## Convert to Bag
	

## Example 5:
model from #4 or download, run on real data

3D (not a priority): "C:\work\git\denoise\wrappers\python\examples\pyglet_pointcloud_viewer.py"
3D : "C:\work\git\denoise\wrappers\python\examples\pyglet_pointcloud_viewer.py"
3D : "C:\work\git\denoise\wrappers\python\examples\opencv_pointcloud_viewer.py"

## Example 6
How covert keras to frozen graph --> camera simulation as #5 but use only opencv



https://cocodataset.org/#home 
https://drive.google.com/file/d/1ZCnqb7OB5fkk4ba4lK2WZry3qu0RUZfZ/view?usp=sharing 
