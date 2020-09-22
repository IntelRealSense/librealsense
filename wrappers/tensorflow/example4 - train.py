import os, sys, shutil
import keras
import time
import tensorflow as tf
from keras import backend as kb
from keras.models import *
from keras.layers import *
from keras.optimizers import *
from keras.callbacks import ModelCheckpoint
import glob
from PIL import Image
import numpy as np
import cv2
from skimage import img_as_uint


#============================================ T R A I N   M A I N ========================================
root = r"./unet_flow"
images_path = root + r"/images"
models_path = root  + r"/models"
logs_path = root  + r"/logs"
train_images = images_path + r"/train"
train_cropped_images_path = images_path + r"/train_cropped"
paths = [root, images_path, models_path, logs_path, train_images, train_cropped_images_path]

print("Creating folders for training process ..")
for path in paths:
    if not os.path.exists(path):
        print("Creating  ", path)
        os.makedirs(path)

#============================================ T R A I N ===============================================
# other configuration
channels = 2
img_width, img_height = 128, 128

unet_epochs = 1

IMAGE_EXTENSION = '.png'
# Get the file paths
kb.clear_session()
gpus = tf.config.experimental.list_physical_devices('GPU')
print("Num GPUs Available: ", len(tf.config.experimental.list_physical_devices('GPU')))
if gpus:
    try:
        # Currently, memory growth needs to be the same across GPUs
        for gpu in gpus:
            tf.config.experimental.set_memory_growth(gpu, True)
        logical_gpus = tf.config.experimental.list_logical_devices('GPU')
        print(len(gpus), "Physical GPUs,", len(logical_gpus), "Logical GPUs")
    except RuntimeError as e:
        # Memory growth must be set before GPUs have been initialized
        print(e)

print("Creating and compiling Unet network .. ")

# save output to logs
old_stdout = sys.stdout
timestr = time.strftime("%Y%m%d-%H%M%S")
model_name = 'DEPTH_' + timestr + '.model'
name = logs_path + r'/loss_output_' + model_name + '.log'
log_file = open(name, "w")
sys.stdout = log_file
print('Loss function output of model :', model_name, '..')


input_size = (img_width, img_height, channels)
inputs = Input(input_size)
conv1 = Conv2D(64, 3, activation='relu', padding='same', kernel_initializer='he_normal')(inputs)
conv1 = Conv2D(64, 3, activation='relu', padding='same', kernel_initializer='he_normal')(conv1)
pool1 = MaxPooling2D(pool_size=(2, 2))(conv1)
conv2 = Conv2D(128, 3, activation='relu', padding='same', kernel_initializer='he_normal')(pool1)
conv2 = Conv2D(128, 3, activation='relu', padding='same', kernel_initializer='he_normal')(conv2)
pool2 = MaxPooling2D(pool_size=(2, 2))(conv2)
conv3 = Conv2D(256, 3, activation='relu', padding='same', kernel_initializer='he_normal')(pool2)
conv3 = Conv2D(256, 3, activation='relu', padding='same', kernel_initializer='he_normal')(conv3)
pool3 = MaxPooling2D(pool_size=(2, 2))(conv3)
conv4 = Conv2D(512, 3, activation='relu', padding='same', kernel_initializer='he_normal')(pool3)
conv4 = Conv2D(512, 3, activation='relu', padding='same', kernel_initializer='he_normal')(conv4)
drop4 = Dropout(0.5)(conv4)
pool4 = MaxPooling2D(pool_size=(2, 2))(drop4)

conv5 = Conv2D(1024, 3, activation='relu', padding='same', kernel_initializer='he_normal')(pool4)
conv5 = Conv2D(1024, 3, activation='relu', padding='same', kernel_initializer='he_normal')(conv5)
drop5 = Dropout(0.5)(conv5)

up6 = Conv2D(512, 2, activation='relu', padding='same', kernel_initializer='he_normal')(UpSampling2D(size=(2, 2))(drop5))
merge6 = concatenate([drop4, up6], axis=3)
conv6 = Conv2D(512, 3, activation='relu', padding='same', kernel_initializer='he_normal')(merge6)
conv6 = Conv2D(512, 3, activation='relu', padding='same', kernel_initializer='he_normal')(conv6)

up7 = Conv2D(256, 2, activation='relu', padding='same', kernel_initializer='he_normal')(UpSampling2D(size=(2, 2))(conv6))
merge7 = concatenate([conv3, up7], axis=3)
conv7 = Conv2D(256, 3, activation='relu', padding='same', kernel_initializer='he_normal')(merge7)
conv7 = Conv2D(256, 3, activation='relu', padding='same', kernel_initializer='he_normal')(conv7)

up8 = Conv2D(128, 2, activation='relu', padding='same', kernel_initializer='he_normal')(
    UpSampling2D(size=(2, 2))(conv7))
merge8 = concatenate([conv2, up8], axis=3)
conv8 = Conv2D(128, 3, activation='relu', padding='same', kernel_initializer='he_normal')(merge8)
conv8 = Conv2D(128, 3, activation='relu', padding='same', kernel_initializer='he_normal')(conv8)

up9 = Conv2D(64, 2, activation='relu', padding='same', kernel_initializer='he_normal')(UpSampling2D(size=(2, 2))(conv8))
merge9 = concatenate([conv1, up9], axis=3)
conv9 = Conv2D(64, 3, activation='relu', padding='same', kernel_initializer='he_normal')(merge9)
conv9 = Conv2D(64, 3, activation='relu', padding='same', kernel_initializer='he_normal')(conv9)
conv9 = Conv2D(2, 3, activation='relu', padding='same', kernel_initializer='he_normal')(conv9)
conv10 = Conv2D(channels, 1, activation='sigmoid')(conv9)

model = Model(inputs=inputs, outputs=conv10)
model.compile(optimizer=Adam(lr=1e-4), loss='binary_crossentropy', metrics=['accuracy'])
model.summary()
compiled_model = model

print("Unet network is successfully compiled !")
print('Preparing training data for CNN ..')
print('Cropping training data ..')

# clean dir
print("Clean cropped images directory ..")
for filename in os.listdir(train_cropped_images_path):
    file_path = os.path.join(train_cropped_images_path, filename)
    try:
        if os.path.isfile(file_path) or os.path.islink(file_path):
            os.unlink(file_path)
        elif os.path.isdir(file_path):
            shutil.rmtree(file_path)
    except Exception as e:
        print('Failed to delete %s. Reason: %s' % (file_path, e))

cropped_w, cropped_h = img_width, img_height

noisy_images = [f for f in glob.glob(train_images + "**/res*" + IMAGE_EXTENSION, recursive=True)]
pure_images = [f for f in glob.glob(train_images + "**/gt*" + IMAGE_EXTENSION, recursive=True)]
ir_images = [f for f in glob.glob(train_images + "**/left*" + IMAGE_EXTENSION, recursive=True)]
config_list = [(noisy_images, False), (pure_images, False), (ir_images, True)]

print("Cropping training images to size ", cropped_w, cropped_h)
for config in config_list:
    filelist, is_ir = config
    w, h = (cropped_w, cropped_h)
    rolling_frame_num = 0
    for i, file in enumerate(filelist):
        name = os.path.basename(file)
        name = os.path.splitext(name)[0]
        if is_ir:
            ii = cv2.imread(file)
            gray_image = cv2.cvtColor(ii, cv2.COLOR_BGR2GRAY)
            img = Image.fromarray(np.array(gray_image).astype("uint16"))
        else:
            img = Image.fromarray(np.array(Image.open(file)).astype("uint16"))
        width, height = img.size
        frame_num = 0
        for col_i in range(0, width, w):
            for row_i in range(0, height, h):
                crop = img.crop((col_i, row_i, col_i + w, row_i + h))
                save_to = os.path.join(train_cropped_images_path,
                                       name + '_{:03}' + '_row_' + str(row_i) + '_col_' + str(col_i) + '_width' + str(
                                           w) + '_height' + str(h) + IMAGE_EXTENSION)
                crop.save(save_to.format(frame_num))
                frame_num += 1
        rolling_frame_num += frame_num

print("Training images are successfully cropped !")

save_model_name = models_path +'/' + model_name
images_num_to_process = 1000
all_cropped_num = len(os.listdir(train_cropped_images_path)) // 3 # this folder contains cropped images of pure, noisy and ir
iterations = all_cropped_num // images_num_to_process
if all_cropped_num % images_num_to_process > 0 :
    iterations += 1

print('Starting a training process ..')
print("Create a 2-channel image from each cropped image and its corresponding IR image :")
print("Channel 0 : pure or noisy cropped image")
print("Channel 1 : corresponding IR image")
print("The new images are the input for Unet network.")
for i in range(iterations):
    print('*************** Iteration : ', i, '****************')
    first_image = i*images_num_to_process
    if i == iterations-1:
        images_num_to_process = all_cropped_num - i*images_num_to_process

    ### convert cropped images to arrays
    cropped_noisy_images = [f for f in glob.glob(train_cropped_images_path + "**/res*" + IMAGE_EXTENSION, recursive=True)]
    cropped_pure_images = [f for f in glob.glob(train_cropped_images_path + "**/gt*" + IMAGE_EXTENSION, recursive=True)]
    cropped_ir_images = [f for f in glob.glob(train_cropped_images_path + "**/left*" + IMAGE_EXTENSION, recursive=True)]

    cropped_images_list = [(cropped_noisy_images, "noisy"), (cropped_pure_images, "pure")]

    for curr in cropped_images_list:
        curr_cropped_images, images_type = curr
        im_files, ir_im_files = [], []
        curr_cropped_images.sort()

        limit = first_image + images_num_to_process
        if first_image + images_num_to_process > len(curr_cropped_images):
            limit = len(curr_cropped_images)

        for i in range(first_image, limit):
            path = os.path.join(train_cropped_images_path, curr_cropped_images[i])
            if os.path.isdir(path):
                # skip directories
                continue
            im_files.append(path)
        cropped_ir_images.sort()

        for i in range(first_image, limit):
            path = os.path.join(train_cropped_images_path, cropped_ir_images[i])
            if os.path.isdir(path):
                # skip directories
                continue
            ir_im_files.append(path)

        im_files.sort()
        ir_im_files.sort()
        images_plt = [cv2.imread(f, cv2.IMREAD_UNCHANGED) for f in im_files if f.endswith(IMAGE_EXTENSION)]
        ir_images_plt = [cv2.imread(f, cv2.IMREAD_UNCHANGED) for f in ir_im_files if f.endswith(IMAGE_EXTENSION)]
        images_plt = np.array(images_plt)
        ir_images_plt = np.array(ir_images_plt)
        images_plt = images_plt.reshape(images_plt.shape[0], img_width, img_height, 1)
        ir_images_plt = ir_images_plt.reshape(ir_images_plt.shape[0], img_width, img_height, 1)

        im_and_ir = images_plt
        if channels > 1:
            im_and_ir = np.stack((images_plt, ir_images_plt), axis=3)
            im_and_ir = im_and_ir.reshape(im_and_ir.shape[0], img_width, img_height, channels)

        # convert your lists into a numpy array of size (N, H, W, C)
        img = np.array(im_and_ir)
        # Parse numbers as floats
        img = img.astype('float32')
        # Normalize data : remove average then devide by standard deviation
        img = (img - np.average(img)) / np.var(img)

        if images_type == "pure":
            pure_input_train = img
        else:
            noisy_input_train = img

    # Start training Unet network
    model_checkpoint = ModelCheckpoint(models_path + r"/unet_membrane.hdf5", monitor='loss', verbose=1, save_best_only=True)
    steps_per_epoch = len(cropped_noisy_images) // unet_epochs
    model.fit(noisy_input_train, pure_input_train,
              steps_per_epoch=steps_per_epoch,
              epochs=unet_epochs,
              callbacks=[model_checkpoint])

    # save the model
    compiled_model.save(save_model_name)
    compiled_model = keras.models.load_model(save_model_name)

sys.stdout = old_stdout
log_file.close()
print("Training process is done successfully !")
print("Check log {} for more details".format(name))
#============================================ T E S T   M A I N =========================================

origin_files_index_size_path_test = {}
test_img_width, test_img_height = 480, 480
img_width, img_height = test_img_width, test_img_height
test_model_name = save_model_name

test_images = images_path + r"/test"
test_cropped_images_path = images_path + r"/test_cropped"
denoised_dir = images_path + r"/denoised"
paths = [test_images, test_cropped_images_path, denoised_dir]

print("Creating folders for testing process ..")
for path in paths:
    if not os.path.exists(path):
        print("Creating  ", path)
        os.makedirs(path)

#================================= S T A R T   T E S T I N G ==========================================
old_stdout = sys.stdout
try:
    model = keras.models.load_model(test_model_name)
except Exception as e:
    print('Failed to load model %s. Reason: %s' % (test_model_name, e))


print('Testing model', str(os.path.basename(test_model_name).split('.')[0]), '..')
name = logs_path + '/output_' + str(test_model_name.split('.')[-1]) + '.log'
print("Check log {} for more details".format(name))

log_file = open(name, "w")
sys.stdout = log_file
print('prediction time : ')

# clean directory before processing
for filename in os.listdir(test_cropped_images_path):
    file_path = os.path.join(test_cropped_images_path, filename)
    try:
        if os.path.isfile(file_path) or os.path.islink(file_path):
            os.unlink(file_path)
        elif os.path.isdir(file_path):
            shutil.rmtree(file_path)
    except Exception as e:
        print('Failed to delete %s. Reason: %s' % (file_path, e))


noisy_images = [f for f in glob.glob(test_images + "**/res*" + IMAGE_EXTENSION, recursive=True)]
ir_images = [f for f in glob.glob(test_images + "**/left*" + IMAGE_EXTENSION, recursive=True)]

total_cropped_images = [0]*len(noisy_images)
ir_total_cropped_images = [0]*len(ir_images)

########### SPLIT IMAGES ##################

print("Crop testing images to sizes of ", test_img_width, test_img_height)
ir_config = (ir_images, ir_total_cropped_images, True, {})
noisy_config = (noisy_images, total_cropped_images, False, origin_files_index_size_path_test)
config_list = [ir_config, noisy_config]

for config in config_list:
    filelist, total_cropped_images, is_ir, origin_files_index_size_path = list(config)
    for idx, file in enumerate(filelist):
        w, h = (test_img_width, test_img_height)
        rolling_frame_num = 0
        name = os.path.basename(file)
        name = os.path.splitext(name)[0]

        if not os.path.exists(test_cropped_images_path + r'/' + name):
            os.makedirs(test_cropped_images_path + r'/' + name)
            new_test_cropped_images_path = test_cropped_images_path + r'/' + name

        if is_ir:
            # ir images has 3 similar channels, we need only 1 channel
            ii = cv2.imread(file)
            gray_image = cv2.cvtColor(ii, cv2.COLOR_BGR2GRAY)
            img = Image.fromarray(np.array(gray_image).astype("uint16"))
            #img = np.array(gray_image).astype("uint16")
        else:
            img = Image.fromarray(np.array(Image.open(file)).astype("uint16"))
            #img = np.array(Image.open(file)).astype("uint16")
            #cv2.imwrite(r"C:\Users\user\Documents\test_unet_flow\images\im"+str(idx)+".png", img)

        width, height = 848, 480 #img.size
        frame_num = 0
        for col_i in range(0, width, w):
            for row_i in range(0, height, h):
                crop = img.crop((col_i, row_i, col_i + w, row_i + h))
                #crop = img[row_i:row_i+h, col_i:col_i+w]
                save_to = os.path.join(new_test_cropped_images_path, name + '_{:03}' + '_row_' + str(row_i) + '_col_' + str(col_i) + '_width' + str(w) + '_height' + str(h) + IMAGE_EXTENSION)
                crop.save(save_to.format(frame_num))
                #cv2.imwrite(save_to.format(frame_num), crop)
                frame_num += 1
            origin_files_index_size_path[idx] = (rolling_frame_num, width, height, file)

        total_cropped_images[idx] = frame_num


########### IMAGE TO ARRAY  ##################
cropped_noisy_images = [f for f in glob.glob(test_cropped_images_path + "**/res*" , recursive=True)]
cropped_noisy_images.sort()
for i,directory in enumerate(cropped_noisy_images):

    cropped_image_offsets = []
    ir_cropped_images_file = test_cropped_images_path + r'/' + 'left-' + str(directory.split('-')[-1])

    cropped_w, cropped_h = test_img_width, test_img_height
    im_files = [f for f in glob.glob(directory + "**/res*" , recursive=True)]
    ir_im_files = [f for f in glob.glob(ir_cropped_images_file + "**/left*" , recursive=True)]

    im_files.sort()
    ir_im_files.sort()

    for i in range(len(im_files)):
        cropped_image_offsets.append([os.path.basename(im_files[i]).split('_')[3], os.path.basename(im_files[i]).split('_')[5]])

    images_plt = [cv2.imread(f, cv2.IMREAD_UNCHANGED) for f in im_files if
                  f.endswith(IMAGE_EXTENSION)]
    ir_images_plt = [cv2.imread(f, cv2.IMREAD_UNCHANGED) for f in ir_im_files if
                     f.endswith(IMAGE_EXTENSION)]

    images_plt = np.array(images_plt)
    ir_images_plt = np.array(ir_images_plt)
    images_plt = images_plt.reshape(images_plt.shape[0], cropped_w, cropped_h, 1)
    ir_images_plt = ir_images_plt.reshape(ir_images_plt.shape[0], cropped_w, cropped_h, 1)

    im_and_ir = images_plt
    if channels > 1:
        im_and_ir = np.stack((images_plt, ir_images_plt), axis=3)
        im_and_ir = im_and_ir.reshape(im_and_ir.shape[0], cropped_w, cropped_h, channels)

    img = np.array(im_and_ir)
    # Parse numbers as floats
    img = img.astype('float32')

    # Normalize data : remove average then devide by standard deviation
    img = (img - np.average(img)) / np.var(img)
    samples = img

    rolling_frame_num, width, height, origin_file_name = origin_files_index_size_path_test[i]
    cropped_w, cropped_h = test_img_width, test_img_height
    whole_image = np.zeros((height, width, channels), dtype="float32")

    t1 = time.perf_counter()
    for i in range(total_cropped_images[i]):
        # testing
        sample = samples[i:i+1]
        row, col = cropped_image_offsets[i]
        row, col = int(row), int(col)
        denoised_image = model.predict(sample)
        row_end = row + cropped_h
        col_end = col + cropped_w
        denoised_row = cropped_h
        denoised_col = cropped_w
        if row + cropped_h >= height:
            row_end = height-1
            denoised_row = abs(row-row_end)
        if col + cropped_w >= width:
            col_end = width-1
            denoised_col = abs(col - col_end)
        # combine tested images
        whole_image[row:row_end, col:col_end]=  denoised_image[:, 0:denoised_row,0:denoised_col, :]
    t2 = time.perf_counter()
    print('test: ', os.path.basename(directory.split('/')[-1]), ': ', t2 - t1, 'seconds')
    denoised_name = os.path.basename(directory.split('/')[-1])
    outfile = denoised_dir + '/' + denoised_name.split('-')[0] + '' + '_denoised-' + denoised_name.split('-')[1] + IMAGE_EXTENSION
    whole_image = img_as_uint(whole_image)
    cv2.imwrite(outfile, whole_image[:,:,0])
sys.stdout = old_stdout
log_file.close()
print("Testing process is done successfully !")