import numpy
import cv2
import os
import logging


class VisualMeasurement(object):
    def __init__(self, log_level):
        self.logger = logging.getLogger(__name__)
        self.logger.setLevel(log_level)
        formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
        ch = logging.StreamHandler()
        ch.setLevel(log_level)
        ch.setFormatter(formatter)
        fh = logging.FileHandler('log_from_VisualMeasurement.log')
        fh.setLevel(log_level)
        fh.setFormatter(formatter)
        self.logger.addHandler(ch)
        self.logger.addHandler(fh)

    def process_images(self, path_to_images):
        self.logger.debug('In process_images with path %s' % path_to_images)
        kernel = numpy.ones((5,5), numpy.uint8)
        for img_file_name in os.listdir(path_to_images):
            self.logger.info('Processing: %s' % img_file_name)
            img = cv2.imread(os.path.join(path_to_images, img_file_name), 0)
            self.logger.debug('Shape of the image: %s' % str(img.shape))

            erosion = cv2.erode(img, kernel, iterations=1)
            cv2.imshow(img_file_name + " ERODED", erosion)
            cv2.waitKey(0)
        cv2.destroyAllWindows()


def main():
    path_to_images = "../fan_captured_images/FanImages_10kHz"
    vis_meas = VisualMeasurement('DEBUG')
    vis_meas.process_images(path_to_images)


if __name__ == '__main__':
    main()
