#!/usr/bin/python3

from PIL import ImageGrab, Image
import numpy as np
import time
import random
from mss import mss
import sys
import pynput.keyboard as kb
import pynput.mouse as m
import os
from collections import defaultdict
from datetime import datetime
from multiprocessing import Pool
from functools import partial

class IOHandler():
    def __init__(self, offset=None, game_dims=None, verbose=True, SCREEN_TYPES=None):
        self.verbose = verbose
        self.SCREEN_TYPES = SCREEN_TYPES
    
        self.keyboard = kb.Controller()
        self.mouse = m.Controller()

        if os.name == "nt":
            self.PLATFORM = "windows"
        elif os.name == "posix":
            self.PLATFORM = "macOS"

        if offset is None:
            # macOS, Safari: 16" MBP 2019, resolution scaled more space, resolution (4096x2560)
            self.MAC_OFFSET = (761, 402)         # Safari (in top left)
            # self.MAC_OFFSET = (1449, 797)        # Desktop app (centered)
            # self.MAC_DIMS = (1200, 900)
            # windows: Microsoft edge, win+<-, my desktop, monitor (1920x1080)
            self.WIN_OFFSET = (389, 230)
            # self.WIN_DIMS = (600, 450)
        else:
            self.set_offset(offset[0], offset[1])

        # TODO: not sure if works on Windows
        # Sets the size of the window
        if game_dims is None:
            self.GAME_DIMS = (1200, 900)
        else:
            self.GAME_DIMS = game_dims

    # Load images from filenames (e.g. load masks)
    def load_images(self, filenames):
        return [self.load_image(filename) for filename in filenames]
    
    # Load images given a directory (pngs)
    # TODO: make static
    def load_images_from_directory(self, main_folder):
        filenames = [filename for filename in os.listdir(main_folder) if filename.endswith('.png')]
        filepaths = [os.path.join(main_folder, filename) for filename in filenames]
        images = self.load_images(filepaths)
        return dict(zip(filenames, images))

    # Load image from file (e.g. load a mask)
    def load_image(self, filename):
        return np.array(Image.open(filename)).swapaxes(0, 1).astype(np.int32)[:,:,:3]


    def debug(self, *args, **kwargs):
        if self.verbose:
            print(*args, **kwargs)

    @staticmethod
    def calc_image_dist(A, B):
        return np.mean(np.linalg.norm(A - B, ord=2, axis=2))
    
    # TODO: dedupe with color_dist
    @staticmethod
    def calc_color_dist(a, b):
        return np.linalg.norm(a-b)

    @staticmethod
    def same_images(A, B, delta = 5):
        dist = IOHandler.calc_image_dist(A, B)
        return dist < delta
    
    @staticmethod
    # On mac, use color picker with display in native (or P3)
    def same_colors(a, b, delta = 10):
        dist = IOHandler.calc_color_dist(a, b)
        # print(dist)
        return dist < delta

    def _capture_portion_windows(self, x0, y0, w0, h0):
        x = x0/2 + self.WIN_OFFSET[0]
        y = y0/2 + self.WIN_OFFSET[1]
        w = w0/2 + 2
        h = h0/2 + 2

        with mss() as sct:
            monitor = {'left': int(x), 'top': int(y), 'width': int(w), 'height': int(h)}
            sct_img = sct.grab(monitor)

        img = Image.frombytes('RGB', sct_img.size, sct_img.bgra, 'raw', 'BGRX')
        img = np.array(img).swapaxes(0, 1).astype(np.int32)[:,:,:3]
        
        img = img.repeat(2,axis=0).repeat(2,axis=1)
        img = img[x0%2:, x0%2:, :]
        img = img[:w0, :h0, :]
        return img


    def _capture_portion_mac(self, x, y, w, h):
        x = x + self.MAC_OFFSET[0]
        y = y + self.MAC_OFFSET[1]

        with mss() as sct:
            monitor = {'left': int(x/2), 'top': int(y/2), 'width': int(w/2 + 1), 'height': int(h/2 + 1)}
            sct_img = sct.grab(monitor)

        img = Image.frombytes('RGB', sct_img.size, sct_img.bgra, 'raw', 'BGRX')
        img = np.array(img).swapaxes(0, 1).astype(np.int32)[:,:,:3]

        img = img[x%2:, y%2:, :]
        img = img[:w, :h, :]
        return img

    def capture_portion(self, x, y, w, h):
        if self.PLATFORM == "windows":
            return self._capture_portion_windows(x, y, w, h)
        else:
            return self._capture_portion_mac(x, y, w, h)

    def capture_screen(self):
        w, h = self.GAME_DIMS
        return self.capture_portion(0, 0, w, h)
    
    ############################################################
    # START OF FUNCTIONS THAT MAY NOT BE COMPATIBLE WITH WINDOWS
    ############################################################

    @staticmethod
    def crop_portion(x, y, w, h, image):
        return image[x:x+w,y:y+h]
    
    def is_given_screen(self, x, y, color, delta=10, image=None):
        if image is None:
            pixel = self.capture_pixel(x, y) 
        else:
            pixel = IOHandler.crop_pixel(x, y, image)
        return self.same_colors(pixel, color, delta)
    
    @staticmethod
    def crop_pixel(x, y, image):
        return image[x:x+1,y:y+1]

    ############################################################
    # END OF FUNCTIONS THAT MAY NOT BE COMPATIBLE WITH WINDOWS
    ############################################################

    def capture_whole_screen(self):
        with mss() as sct:
            monitor = sct.monitors[1]
            sct_img = sct.grab(monitor)

        img = Image.frombytes('RGB', sct_img.size, sct_img.bgra, 'raw', 'BGRX')
        img = np.array(img).swapaxes(0, 1).astype(np.int32)[:,:,:3]
        
        if self.PLATFORM == "windows":
            img = img.repeat(2,axis=0).repeat(2,axis=1)

        return img

    def capture_pixel(self, x, y):
        return self.capture_portion(x, y, 1, 1)[0,0,:]
    
    @staticmethod
    def img_from_arr(arr):
        return Image.fromarray(arr.swapaxes(0, 1).astype(np.uint8), "RGB")

    # Dunno if this works, not used currently
    @staticmethod
    def arr_from_img(img):
        return np.array(img).swapaxes(0, 1).astype(np.int32)[:,:,:3]

    @staticmethod
    def show_image(arr):
        img = IOHandler.img_from_arr(arr)
        img.show()
    
    @staticmethod
    def get_average_image(image_arrs):
        stacked_images = np.stack(image_arrs, axis=0)
        avg_img = np.mean(stacked_images, axis=0)
        return avg_img

    @staticmethod
    def save_image(arr, filename=None):
        if filename is None:
            filename = "saved/{}.png".format(time.time())
        img = IOHandler.img_from_arr(arr)
        img.save(filename)

    def save_game_capture(self, filename=None):
        img = self.capture_screen()
        if filename is None:
            curr_time = datetime.now()
            filename = curr_time.strftime(f'saved/%Y-%m-%d %H.%M.%S.png')
        IOHandler.save_image(img, filename)


    def set_offset(self, x, y):
        if self.PLATFORM == "macOS":
            self.MAC_OFFSET = (x, y)
        else:
            self.WIN_OFFSET = (int(x)/2, int(y)/2)

    # Should be called. Think it only works for games with a dark blue border
    def calibrate_offset(self):
        screen = self.capture_whole_screen()

        screen_top = -1
        screen_bot = -1
        screen_right = -1
        screen_left = -1

        for x in range(0, screen.shape[0], 1208):
            stride = 0
            for y in range(screen.shape[1]):
                if self.same_colors(screen[x][y], [32, 41, 74], 5):
                    stride += 1
                else:
                    if stride == 908:
                        screen_top = y - stride + 4
                        screen_bot = y - 4
                    elif stride == 4:
                        if screen_top == -1:
                            screen_top = y
                        elif screen_top == y - 904:
                            screen_bot = y - 4
                    stride = 0

        for y in range(0, screen.shape[1], 908):
            stride = 0
            for x in range(screen.shape[0]):
                if IOHandler.same_colors(screen[x][y], [32, 41, 74], 5):
                    stride += 1
                else:
                    if stride == 1208:
                        screen_left = x - stride + 4
                        screen_right = x - 4
                    elif stride == 4:
                        if screen_left == -1:
                            screen_left = x
                        elif screen_left == x - 1204:
                            screen_right = x - 4
                    stride = 0
        
        self.debug("screen bounds: ({}, {}) to ({}, {})".format(screen_left, screen_top, screen_right, screen_bot))

        if screen_top != -1 and screen_bot != -1 and screen_right != -1 and screen_left != -1:
            if self.PLATFORM == "macOS":
                self.MAC_OFFSET = (screen_left, screen_top)
            else:
                self.WIN_OFFSET = (int(screen_left)/2, int(screen_top)/2)
        else:
            self.debug("trouble finding all edges of screen")


    # See if a specific pixel is ~equal to a given color
    # E.g. does_pixel_equal(1066, 684, [229, 99, 42])
    def does_pixel_equal(self, x, y, color, delta=10, image=None):
        if image is None:
            pixel = self.capture_pixel(x, y)
        else:
            pixel = self.crop_pixel(x, y, image)
        # print(pixel)
        return IOHandler.same_colors(pixel, color, delta)

    # See if a portion of the screen is ~equal to a given image
    def does_image_equal(self, x, y, w, h, other_img, delta=5):
        image = self.capture_portion(x, y, w, h)
        return IOHandler.same_images(image, other_img, delta)

    @staticmethod
    def get_best_mask(target, masks):
        mask_arr = np.stack(list(masks))
        pixel_dists = np.sqrt(np.sum(np.square(target - mask_arr), axis=(3)))
        img_dists = np.mean(pixel_dists, axis=(1,2))
        best_idx = np.argmin(img_dists)
        best_dist = img_dists[best_idx]
        return best_idx, best_dist
    
    # TODO: now that we have this, use it
    @staticmethod
    def get_best_mask_dict(target, mask_dict):
        best_idx, best_dist = IOHandler.get_best_mask(target, mask_dict.values())
        return list(mask_dict.keys())[best_idx], best_dist
    
    @staticmethod
    def get_best_color(target, colors):
        color_arr = np.stack(list(colors))
        pixel_dists = np.sqrt(np.sum(np.square(target - color_arr), axis=(1)))
        best_idx = np.argmin(pixel_dists)
        best_dist = pixel_dists[best_idx]
        return best_idx, best_dist
    
    # TODO: now that we have this, use it
    @staticmethod
    def get_best_color_dict(target, color_dict):
        best_idx, best_dist = IOHandler.get_best_color(target, color_dict.values())
        return list(color_dict.keys())[best_idx], best_dist

    def press_key(self, key, delay=0.1):
        self.keyboard.press(key)
        time.sleep(delay)

    def release_key(self, key, delay=0.1):
        self.keyboard.release(key)
        time.sleep(delay)

    def click_key(self, key, delays=None):
        if delays is None:
            delays = [random.uniform(0.1, 0.15), random.uniform(0.1, 0.15)]
        self.press_key(key, delays[0])
        self.release_key(key, delays[1])
        

    # Click the mouse (relative to arcade screen?)
    def click_mouse(self, x, y, delays=None):
        if self.PLATFORM == "macOS":
            x = (x + self.MAC_OFFSET[0])/2
            y = (y + self.MAC_OFFSET[1])/2
        else:
            x = x + self.WIN_OFFSET[0]
            y = y + self.WIN_OFFSET[1]
        if delays is None:
            delays = [random.uniform(0.2, 0.5), random.uniform(0.1, 0.15), random.uniform(0.1, 0.15)]
        self.mouse.position = (x, y)
        time.sleep(delays[0])
        self.mouse.press(m.Button.left)
        time.sleep(delays[1])
        self.mouse.release(m.Button.left)
        time.sleep(delays[2])

    # Move the mouse
    def move_mouse(self, x, y, delay=None):
        if self.PLATFORM == "macOS":
            x = (x + self.MAC_OFFSET[0])/2
            y = (y + self.MAC_OFFSET[1])/2
        else:
            x = x + self.WIN_OFFSET[0]
            y = y + self.WIN_OFFSET[1]
        if delay is None:
            delay = random.uniform(0.1, 0.15)
        self.mouse.position = (x, y)
        time.sleep(delay)

    def set_screen_types(self, SCREEN_TYPES):
        self.SCREEN_TYPES = SCREEN_TYPES

    # TODO: use this across bots
    def is_screen_type(self, screen_type, image=None):
        x, y, color = self.SCREEN_TYPES[screen_type]
        return self.is_given_screen(x, y, color, delta=10, image=image)

    def calc_screen_types(self, image=None):
        return {screen_type for screen_type in self.SCREEN_TYPES.keys() if self.is_screen_type(screen_type, image)}
