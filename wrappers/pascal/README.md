# Pascal-LibreSense

## Table of contens

* [Introduccion](#introduccion)
* [Requirements](#requirements)
* [Install](#install)



## Introduccion

This proyect is a warpping  for modern pascal Pascal of [librealsense library from intel](https://github.com/IntelRealSense/librealsense).

Why modern free Pascal?. Here there is a good [article](https://castle-engine.io/modern_pascal).


## Requirements

You need a modern Pascal compiler. You have two options:

* Lazarus is a Delphi compatible cross-platform IDE for Rapid Application Development. It has variety of components ready for use and a graphical form designer to easily create complex graphical user interfaces.

* Delphi is a general-purpose programming language and a software product that uses the Delphi dialect of the Object Pascal programming language and provides an integrated.



## Install

* [Lazarus](#lazarus)


### Lazarus
The IntelRealsense is a 64 bits library. So you must create a 64 bits aplication. 

If you don't have a copy of lazarus download Lazarus for 64 bits from here: https://www.lazarus-ide.org/index.php?page=downloads

Be sure that you download the correct version.
![Lazarus 64 bits](./images/installlazarus64.png)

Download or clone the repository in a folder where you like.

From Lazarus, open a package.
![Open pacakge](./images/lazarusopenpackage.png)

Navigate to the folder where you downloaded of cloned the repository. Open the folder wrappers,free pascal,Lazarus package. And select the file laz_realsense.
![package laz_realsense](./images/lazaruselectpackage.png)

Now compile the pacakge.
![compile package](./images/lazaruscompilepackage.png)

After compile you can close the the window. 

Now we are ready to create create applications. Create a new project.
![Create a project](./images/lazarusnewproyect.png)

Can be a console program or a application.
![select project](./images/lazarusprojects.png)

Add the realsense package to your project. In menu project, select project inspector.
![project options menu](./images/lazarusprojectinspector.png)

Now add a new requeriment. 
![add requeriment](./images/lazarusaddreq.png)

Search for laz_realsense, and select it.
![laz_realsense](./images/lazarusselectreq.png)

Now you are ready start write your code. For more information see the examples in examples folder.

Note. In windows be sure that Realsense.dll can be found in the path or is in the same folder of your executable.





