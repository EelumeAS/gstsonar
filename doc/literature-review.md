# Obstacle detection using FLS

This document is an introduction into theory of operation of a FLS sonar, along with theory around detecting obstacles in sonar images.

## FLS theory of operation

A FLS (Forward-Looking Sonar) is a multibeam sonar mounted to look forward from the vehicle.
It can be used to detect obstacles.

![FLS setup](https://ars.els-cdn.com/content/image/1-s2.0-S0003682X21000797-gr1.jpg "FLS concept")

The FLS consists of a hydrophone array arranged along a curve in the horizontal plane of the vehicle.
There is one, wide transmitting beam, illuminating the scene.
There are multiple (virtual) receiving beams, configured to be sensitive in different directions using beam steering.
During one measurement from the FLS, it starts by producing the transmitting beam.
Then it waits a specified number of sample intervals $t0$, depending on the configured minimum range, before it starts recording the intensities of the echo with the configured sample rate $f$.
It stops recording after a certain number of sample intervals, depending on the resolution or the maximum range.

As output from the FLS, you get a two-dimensional grid of sound intensities with one axis corresponding to the number of beams, and the other to the range resolution of the sonar.
The range of an intensity for a given beam index and sample index can be calculated from the sample index $ti$, start index $t0$, the sample rate $f$ and the speed of sound, $c$:

$$range = { (t0 + ti) c \over 2f }$$

You also get a one-dimensional array containing the angle of each beam.
Together with the ranges, this gives you your two-dimensional picture of the three-dimensional scene you are illuminating.
Similar to a picture taken with a camera, this is a degenerate measurement with ambiguity in one dimension.
For a camera picture, the unresolved dimension is the picture depth, while for a FLS, the unresolved dimension is the elevation angle.
So a given echo could have come from anywhere within your vertical opening angle.
Provided the vehicle lies in the horizontal plane, you don't know if the echo comes from an object which is over, under or right in front of the vehicle, in terms of depth.

## Noise

### Speckle noise

There is a lot of noise in sonar images.
Unlike normal camera images, there is a lot of granular multiplicative noise in the background called "speckle noise" (See [Zhang et al](https://www.hindawi.com/journals/js/2021/8845814/)).
This makes segmentation / detection challenging.
It also makes it difficult to match points of interest in consecutive images.

The speckle noise can be modeled and characterized using different distributions (See [Teixeira et al](https://www.cs.cmu.edu/~kaess/pub/Teixeira18iros.pdf)).
The noise characteristics can unfortunately vary wildly between different environments.

### Side lobes

![beam patterns](https://www.raymarine.no/uploadedimages/beam_patterns_450.gif "Beam patterns")

There is also noise due to side lobes.
This is a particularly challenging noise source arising because of the beam pattern of each receiving beam.
A beam pattern describes how sensitive a transducer is for sound coming from different directions.
A beam pattern usually has a main "lobe" along the direction of the beam, where it is most sensitive, but it also has "side lobes" on each side of the main lobe.
Echoes picked up from the direction of the side lobes will be erroneously registered as coming from the direction of the main lobe.
This can be seen in sonar images as ghosts of a high energy echo in other beams along the same range (see the left part of the image below).

![side lobe noise](https://raw.githubusercontent.com/pvazteixeira/multibeam/feature/mrf/images/scans.png "Noise due to side lobes")

### Multipath

Multipath is another source of noise, where an echo might reflect off a second surface and thus take a different path back to the receiver instead of, or in addition to, the straight one.
This can potentially be seen as ghosts of a object in a different part of the sonar image, at a further range.

## Segmentation

Segmentation of sonar images is the process of dividing the image into different categories, based on what you think the echoes came from.
Advanced segmentation algorithms might use the known echo profiles of different materials, or the shape of different objects to segment the image into multiple categories.
For this kind of analysis, a calibration process is necessary, where the noise characteristics of the sonars environment is accurately modeled.
If you are using a convolutional neural network, then the noise characteristics will hopefully be learned by your network.

### Obstacle detection

Obstacle detection is the simplest case of segmentation, where you only divide the image into two categories, obstacle and water.
From now on we will refer to this simple case of segmentation unless further specified.

The most basic and obvious segmentation algorithm is to pick a threshold and classify all intensities which are higher as object and all lower as water.
This performs generally poorly because of the sonar image noise.
Many early methods expanded on this method by preprocessing the image using histogram smoothing etc. (f.ex. [Burguera et al](https://www.researchgate.net/publication/224196712_Range_extraction_from_underwater_Imaging_Sonar_data)).
Generally speaking, the preprocessing does improve the performance, but only to a certain degree.

In contrast to the smoothing based methods, methods which model the sonar noise have in theory a higher potential, since they actually add information in the preprocessing stage, instead of just smoothing (f.ex. [Teixeira et al](https://www.cs.cmu.edu/~kaess/pub/Teixeira18iros.pdf)).
But as mentioned, this comes at the expense of having to calibrate for the noise characteristics in each new environment.

In addition to improving the preprocessing, others have tried different segmentation strategies, based for instance on region growing (f.ex. [Wang, Wang et al](https://www.mdpi.com/1424-8220/21/21/6960/pdf-vor)).
The basic observation used is that obstacles usually generate echoes at neighbouring beams, as well as over a few different ranges.
Comparing intensities with the ones in the neighburing beam can potentially be used to reject outliers.

## Feature detection

Feature detection is an alternative to segmentation, although is could be viewed as a subclass.
In computer vision, a feature detector tries to find distinguished points, for instance corners, in a camera image.
The features are in general chosen to be easily recognisable in images from different perspectives.
This way they can be tracked and used to determine the geometry of the scene or the motion of the camera.

Traditional feature detectors perform poorly on raw sonar images because of the speckle noise.
Some detectors, like [Akaze](https://docs.opencv.org/4.x/db/d70/tutorial_akaze_matching.html) might perform a bit better ([Toro et al](https://arxiv.org/abs/1709.02150), [Westman](https://www.cs.cmu.edu/~kaess/pub/Westman20joe.pdf)).
Dedicated sonar features, which take into account a noise model have also been proposed ([Zhang et al](https://www.hindawi.com/journals/js/2021/8845814/)).

## Resolving degeneracy

By tracking features (or regions) over consecutive sonar images with enough motion, it is possible to resolve the inherent degeneracy of the sonar images, supplying an estimate of the missing elevation angle of each feature.
This is implicitly done in SLAM implementations (f.ex. [Westman](https://www.cs.cmu.edu/~kaess/pub/Westman20joe.pdf), [Wang et al](https://arxiv.org/abs/2202.08359)).
Given accurate odometry measurements from other sensors, f.ex. imu, pressure sensor, dvl or usbl, the elevation angle can be estimated explicitly.

[Vogiatzis and Hernandez](https://www.google.com/url?q=http://www.george-vogiatzis.org/publications/ivcj2010.pdf&sa=U&ved=2ahUKEwjNhvatuZz8AhWRlMMKHRxMAcwQFnoECAQQAg&usg=AOvVaw04CkusxLDmSE1ujwouaeWz) outlines a bayesian inference based approach for estimating depth of image patches in camera images.
The method also maintains a probability distribution for the likelihood that the image patch has been correctly tracked, which also can be used to reject false positives.
A modified approach, using the sonar geometry instead of epipolar geometry, could potentially be used to estimate the elevation angle in sonar images, as well as to reject false positives from segmentation / feature extraction.

Note that while "enough motion" in the case of camera images usually means translational movement or tilting and yawing, this is different in sonars.
For FLS sonars rolling is desirable.
So in the case of a vehicle moving forwards while maintaining orientation perfectly, it is in theory impossible to resolve the elevation angle of features.







