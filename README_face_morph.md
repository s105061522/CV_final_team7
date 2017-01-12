readme for face_morph.exe

0.face_morph.exe is in exe file.

1.Environment settings for face_morph:
 
    Developped with Visual Studio 2013.(https://www.visualstudio.com/downloads/) 
 
    You need to download OpenCV (3.0.0).(http://opencv.org/downloads.html)
 
    And you have to set proper environment variable for OpenCV.
 
    That is,add .dll paths to windows $PATH and change include + .lib paths in Visual Studio Settings.
 
    You can watch https://www.youtube.com/watch?v=-cWdLm7WZm8 on youtube to know how to do it.

2.Input argument for face_morph.exe(Under linux system or CMD):
 
    You can just run face_morph.exe or input some arguments to choose images and parameters.
 
    parents image are in source_image file,and babies image are in baby file.
 
    arg[1]: parent name1(ex:001_1)
 
    arg[2]: parent name2(ex:001_2)

    arg[3]: baby name(ex:021)
  
    arg[4]: eye ratio(ex:0~1)  
  
    arg[5]: nose ratio(ex:0~1)
  
    arg[6]: mouse ratio(ex:0~1)
  
    arg[7]: alpha ratio of Location Correspondence for parents(ex:0~1)
  
    arg[8]: alpha ratio ofLocation Correspondence for generated baby(ex:0~1)
 
    arg[9]: alpha ratio of intensity for generated baby(ex:0~1)

3.Output of face_morph.exe:
  
    ouput to result file.
  
    baby_arg[1]_arg[2].jpg: Generated baby image.
