# Morse-Code-BBB-Distro
Special Debian Linux distribution that contains morse code character driver as part of the kernel.

# Requirements
* microSD card larger than 2 GB and less than 32 GB.
* micro SD card reader (with support for USB 3.0 if possible).
* Linux Ubuntu Virtual Machine with internet connection.

## Step 1: Connecting your microSD card to your Ubuntu VM.

### Install VirtualBox
Download and install the latest version of VirtualBox for your host system. You can use this link for [VirtualBox 6.1.16 platform packages](https://www.virtualbox.org/wiki/Downloads). Set it up using the wizard installer.

### Install VirtualBox Extension Pack
Download and install the latest version of the VirtualBox Extension Pack to support USB3.0 devices. You will need this if using a modern microSD card. You can use this link for [VirtualBox 6.1.16 Oracle VM VirtualBox Extension Pack](https://www.virtualbox.org/wiki/Downloads). Set it up using the wizard installer.

Open your VirtualBox Ubuntu VM. Go to Settings >> Ports >> USB. Enable the USB 3.0 Controller.



```bash
pip install wxPython
pip install faker
pip install pandas
pip install matplotlib
```

Once done with installing the required dependencies, to run this application open a Terminal/Command Line Prompt window, navigate to the directory where the file app.py is found, and do:
```bash
python app.py
```
### Note
If you had any of the dependencies listed above already pre-installed in your machine, then you might receive an error when trying to run the application. To fix this you will need to uninstall the four dependencies listed above and reinstall them. Alternatively, you can create a new python virtual environment and install the depencies, and run the application from within the virtual environment.

## Usage
Once the application is opened, the user will need to generate IoT data by clicking "File", and then clicking "Generate IoT".
After data has been generated, the user can:

* Save data as a JSON file.
* Save data as a CSV file.
* Compute common statistical values for the sensor data gathered.
* Plot four different histograms of the outside temperature sampled every 6 hours.
* Plot a line graph of the outside temperature vs the room temperature.
* Plot four histograms of room and outside temperature and humidity for all 1000 user entries.
