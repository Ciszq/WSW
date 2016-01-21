import numpy
import cv2
import os
import logging
from matplotlib import pyplot as plt


class VisualMeasurement(object):
    def __init__(self, log_level, file_logs=False):
        self.logger = logging.getLogger(__name__)
        self.logger.setLevel(log_level)
        formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
        ch = logging.StreamHandler()
        ch.setLevel(log_level)
        ch.setFormatter(formatter)
        self.logger.addHandler(ch)
        if file_logs:
            fh = logging.FileHandler('log_from_VisualMeasurement.log')
            fh.setLevel(log_level)
            fh.setFormatter(formatter)
            self.logger.addHandler(fh)

    def process_images(self, path_to_images):
        self.logger.debug('In process_images with path %s' % path_to_images)
        kernel = numpy.ones((5,5), numpy.uint8)
        for img_file_name in os.listdir(path_to_images):
            self.logger.info('Processing: %s' % img_file_name)
            img_number = self.get_image_number(img_file_name)
            if 31 < img_number < 40:
                img = cv2.imread(os.path.join(path_to_images, img_file_name), 3)
                self.logger.debug('Shape of the image: %s' % str(img.shape))
                blurred = cv2.blur(img, (5, 5))
                ret, thresholded = cv2.threshold(img, 127, 255, cv2.THRESH_BINARY)
                erosion = cv2.erode(img, kernel, iterations=1)
                img_list = [img, blurred, thresholded, erosion]
                titles = ['Org', 'Avg', 'Thres', 'Erod']
                self.plot_images(img_list, titles)

    @staticmethod
    def get_image_number(image_name):
        image_number = image_name[image_name.rfind('_') + 1:image_name.rfind('.')]
        return int(image_number)

    @staticmethod
    def plot_images(images_list, titles_list):
        images_number = len(images_list)
        if len(titles_list) != images_number:
            raise ValueError('Wrong titles number. Got %d images and %d titles.' % (images_number, len(titles_list)))
        for i, image in enumerate(images_list):
            plt.subplot(2, (images_number + 1) / 2, i + 1)
            plt.imshow(image)
            plt.title(titles_list[i])
        plt.show()


def main():
    path_to_images = "../fan_captured_images/FanImages_10kHz"
    vis_meas = VisualMeasurement('DEBUG')
    vis_meas.process_images(path_to_images)


if __name__ == '__main__':
    main()
