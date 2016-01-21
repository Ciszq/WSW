import datetime
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

    def simple_processing_images(self, path_to_images):
        self.logger.debug('In simple_processing_images with path %s' % path_to_images)
        kernel = numpy.ones((5, 5), numpy.uint8)
        for img_file_name in os.listdir(path_to_images):
            self.logger.info('Processing: %s' % img_file_name)
            img_number = self.get_image_number(img_file_name)
            if 31 < img_number < 40:
                img = cv2.imread(os.path.join(path_to_images, img_file_name), 3)
                self.logger.debug('Shape of the image: %s' % str(img.shape))
                print img
                grayed = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
                blurred = cv2.blur(grayed, (5, 5))
                ret, thresholded = cv2.threshold(blurred, 127, 255, cv2.THRESH_BINARY)
                erosion = cv2.erode(thresholded, kernel, iterations=1)
                img_list = [img, grayed, blurred, thresholded, erosion]
                titles = ['Org', 'Gray', 'Avg', 'Thres', 'Erod']
                self.plot_images(img_list, titles)

    def object_distinction(self, path_to_images):
        self.logger.debug('In object_distinction with path %s' % path_to_images)
        dict_of_images = {}
        kernel = numpy.ones((5, 5), numpy.uint8)

        for img_file_name in os.listdir(path_to_images):
            img_number = self.get_image_number(img_file_name)
            dict_of_images[img_number] = img_file_name
        min_img_index = min(dict_of_images.keys())
        max_img_index = max(dict_of_images.keys())
        self.logger.debug('Indices range: %d, %d' % (min_img_index, max_img_index))

        for img_index in range(min_img_index + 1, max_img_index):
            t1 = datetime.datetime.now()
            self.logger.debug('Processing image number: %d' % img_index)
            this_img = cv2.imread(os.path.join(path_to_images, dict_of_images[img_index]), cv2.IMREAD_GRAYSCALE)
            prev_img = cv2.imread(os.path.join(path_to_images, dict_of_images[img_index - 1]), cv2.IMREAD_GRAYSCALE)
            this_preprocessed = self.preprocessing_of_image(this_img)

            t2 = datetime.datetime.now()
            exec_time = t2 - t1
            self.logger.info('Img %d was processed in %s' % (img_index, exec_time))

            prev_preprocessed = self.preprocessing_of_image(prev_img)

            subtracted = cv2.subtract(this_preprocessed, prev_preprocessed)
            eroded = cv2.erode(subtracted, kernel, iterations=1)
            dilatated = cv2.dilate(eroded, kernel, iterations=1)
            sub_edges = cv2.Canny(dilatated, 100, 200)

            this_edges = cv2.Canny(this_preprocessed, 100, 200)
            this_dilat = cv2.dilate(this_edges, kernel)

            final_image = cv2.bitwise_and(sub_edges, this_dilat)

            if max([max(elem) for elem in final_image]) == 255:
                img_list = [this_img, prev_img, this_preprocessed, prev_preprocessed, subtracted, eroded, dilatated,
                            sub_edges, this_edges, this_dilat, final_image]
                titles = ['Org', 'Prev', 'Preproc', 'PrevPreproc', 'SUBTRACTED', 'SubEroded', 'SubDilat',
                          'SubEdges', 'PrevEdges', 'PrevDilat', 'FINAL']
                self.plot_images(img_list, titles)

    @staticmethod
    def preprocessing_of_image(img):
        blurred = cv2.blur(img, (5, 5))
        ret, thresholded = cv2.threshold(blurred, 100, 255, cv2.THRESH_BINARY)
        return thresholded

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
            plt.gray()
            plt.imshow(image)
            plt.title(titles_list[i])
        plt.show()


def main():
    path_to_images = "../fan_captured_images/FanImages_10kHz"
    vis_meas = VisualMeasurement('DEBUG')
    # vis_meas.simple_processing_images(path_to_images)
    vis_meas.object_distinction(path_to_images)


if __name__ == '__main__':
    main()
