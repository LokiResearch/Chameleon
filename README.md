# About
This repository contains Chameleon's source code. Chameleon is a software that combines computer vision feature-matching algorithms with an open database format to allow the incorporation of dynamic HTML5 interactive content over any type of document (e.g. PDF files, PowerPoint documents, etc.) without modifying existing applications or the source document.
Read the associated research article for more details: [https://hal.inria.fr/hal-02467817/document](https://hal.inria.fr/hal-02467817/document)

If you use Chameleon for academic purposes, please cite: Damien Masson, Sylvain Malacria, Edward Lank, and Géry Casiez. 2020. Chameleon: Bringing Interactivity to Static Digital Documents. In Proceedings of the 2020 CHI Conference on Human Factors in Computing Systems (CHI ’20). Association for Computing Machinery, New York, NY, USA, 1–13. 

[![DOI](https://img.shields.io/badge/doi-10.1145%2F3313831.3376559-blue)](https://doi.org/10.1145/3313831.3376559)

```
@inproceedings{10.1145/3313831.3376559,
    author = {Masson, Damien and Malacria, Sylvain and Lank, Edward and Casiez, G\'{e}ry},
    title = {Chameleon: Bringing Interactivity to Static Digital Documents},
    year = {2020},
    isbn = {9781450367080},
    publisher = {Association for Computing Machinery},
    address = {New York, NY, USA},
    url = {https://doi.org/10.1145/3313831.3376559},
    doi = {10.1145/3313831.3376559},
    booktitle = {Proceedings of the 2020 CHI Conference on Human Factors in Computing Systems},
    pages = {1–13},
    numpages = {13},
    keywords = {interactivity, augmented documents, feature matching},
    location = {Honolulu, HI, USA},
    series = {CHI ’20}
}
```

# How to use
![](resources/GIF_Chameleon_HowTo.gif)

To add a dynamic figure to a document:
- Start Chameleon
- Copy the dynamic figure's URL (either local or online)
- Open the document
- Select "Registration Tool" from Chameleon's menu
- Select the figure to augment in the document
- Paste the dynamic figure's URL and select OK

You can test Chameleon by following this process using the resources provided in the [/test](/test) folder. In this folder, you will find a PDF document (chameleon_paper.pdf) and two HTML figures (figure2_left.html and figure2_right.html) which are interactive version of charts in the PDF document.

# Build from source
Chameleon currently runs only on macOS

## Requirements
- Qt (tested with Qt 5.14.2) with "Qt WebEngine" and Qt Creator
- OpenCV 4 (using ``brew install opencv``)

## Compiling
Chameleon uses a .pro file. Therefore, to compile, you can either generate a makefile from the .pro by using qmake or you can open the project in QtCreator which should handle the compilation process automatically.

The .pro has been set to use pkg-config to find the location of OpenCV. Alternatively, you can directly edit the .pro file by adding the location to opencv on your system if you do not want to rely on pkg-config.


# Authorizations on macOS
Chameleon needs several permissions to work properly on macOS. Beware that new versions of macOS regularly break Chameleon / require more permissions. Following are all the permissions required, as of the time of writing these lines, on macOS Catalina.

On Chameleon's first launch, you should be prompted for your password and for accessibility permissions.
Chameleon needs these two permissions to run properly:
- sudo access is needed to use dtrace which monitors file access on the system and to install an event tap (see related code in src/os_specific/macos/window.mm).
- Accessibility is needed to retrieve files open by currently running applications & to make augmentations seemless (i.e. when scrolling or moving windows, see related code in src/os_specific/macos/accessibility.mm).

On macOS Catalina and above, you should also grant Chameleon's permission for "Screen Recording" in "System Settings > Security & Privacy > Privacy".

On OSX El Capitan and later, this should only be enough to use Chameleon with applications that were already running when Chameleon was started and that specify the file they load in their title (e.g. Preview, Keynote, etc...).
If you want Chameleon to work with all applications, even the one run after the launch of Chameleon, you will need to disable "System Integrity Protection" for dtrace.
This is done by starting your mac in recovery mode (Cmd+R at boot) and type ``csrutil enable --without dtrace`` in the Terminal (accessed from Utilities).