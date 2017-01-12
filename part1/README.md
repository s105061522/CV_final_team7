Read_me for face_landmark_detection_to_file.vcxproj and face_landmark_detection_to_file.exe

  1.Environment settings for face_landmark_detection_to_file.vcxproj:
  
      Developped with Visual Studio 2015. 
  
      You need to download OpenCV (2.4.13)(Note: cannot use 3.0 or above), DLib (19.2) libraries and cmake(3.7.1)
  
      You have to use cmake-gui.exe for face_landmark_detection_to_file.cpp in DLib with proper OpenCV path first to get its vcxproj.
 
      After building vcxproj., you have to set proper environment variable for OpenCV.
  
      That is,add .dll paths to windows $PATH and change include + .lib paths in Visual Studio Settings.
  
      You can watch https://www.youtube.com/watch?v=-cWdLm7WZm8 on youtube to know how to do it.

  2.Input argument for face_landmark_detection_to_file.exe(Under linux system or CMD):
  
      arg[1]: XXXXX.dat(trainging data).We use shape_predictor_68_face_landmarks.dat in current project.
 
      arg[2] to arg[N]: XXXXX.jpg.Input image.

  3.Output of face_landmark_detection_to_file.exe:
  
      XXXXX_k_orignal.txt: It contains features points with (x,y) coordinate of Input image.
  
      XXXXX_k_orignal.jpg: This image is same as input XXXXX.jpg. It is used for debugging.
  
      XXXXX_k_resize.txt: It contains features points with (x,y) coordinate of resize image.This file is used for part 2 of our project.
  
      XXXXX_k_resize.jpg:This image is resized image from input XXXXX.jpg.This image is used for part 2 of our project.
 
      XXXXX_k_resize_result.jpg:This image just shows the result after facial feature points detecting.
  
      The k means order of face we detect in each input image. 
