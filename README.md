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

Open your VirtualBox Ubuntu VM.
Go to Settings >> Ports >> USB. Enable the USB 3.0 Controller. 

Start your VirtualBox Ubuntu VM. Connect your microSD card. 

On the menu bar of VirtualBox go to Devices >> USB and choose your microSD card. 

Once you choose your microSD card is properly inserted on your VM, it should display a check mark next to it and a USB icon will display in the bottom left.

## Step 2: Partitioning and formatting your microSD card.
Open a Terminal window.
Download and install the gparted utility
```bash
sudo apt-get install gparted
```
Once done, open the gparted Graphical User Interface that will allow you to format and partition the microSD card.
```bash
sudo gparted
```

Choose your microSD card on the top-right corner of the gparted GUI. **Make sure to choose the correct device. If you choose the incorrect device, you may destroy your machine's file system and cause irreversible damages.** Usually your machine's disk driver is called _sda_ and your microSD is called _sdb_ if you only have to devices connected. Be sure that you choose the right device before continuing.

Click on all the partition(s) of the device that have been allocated. On the menu bar go to Partition >> Unmount. **Leave the unallocated partition alone.**

After unmounting the partition(s) select the Delete icon.

Select the Checkmark icon to apply the changes made to the microSD card.

Click the Apply button in the prompt that pops up. If All operation are successfully completed then click the Close button.

Once all the partitions of the microSD have been cleared, we create the new partitions necessary for the boot loader (Das U-Boot) and for the Linux Root File System.

Click on the unallocated partition of the microSD and select the New icon to create a new partition.

The first partition will be the boot loader partition. Assign around _1 GB_ of space for this partition and the File System format will be _fat32_. **You must use the Label name _BOOT_ or else U-Boot will not function properly.** Use the partition configurations as shown below and click Add.

The second partition will be the Linux Root File System partition. Assign _all of the remaining space_ for this partition and the File System format will be _ext3_. **Use the Label _RFS_ to indicate that this is the RFS.** Use the partition configurations as shown below and click Add.

Once done select the Checkmark icon to apply the changes made to the microSD card.

Click on the _BOOT_ partition then on the menu bar click Partition >> Manage Flags. Select the boot option, this way BeagleBone Black knows what partition is the boot partition when reading from the microSD card.

Eject your microSD card and insert it back in. Navigate to the microSD partitions by accessing the /media/[username]/BOOT path and the /media/[username]/RFS path.**Make sure that you are in the correct directory path or you could erase all your files.**

Enter the following commands in order to erase all files that may be present in these two partitions:
```bash
cd /media/[username]/RFS
sudo rm -rf *
cd /media/[username]/BOOT
sudo rm -rf *
```

## Step 3: Setting up the boot loader (Das U-Boot)
Install the required OpenSSL libssl-dev package.
```bash
sudo apt-get install libssl-dev
```
Install and set up git.
```bash
sudo apt-get install git
sudo git config --global user.email "[your_email@here]"
```

Get the latest version of the code for u-boot. _As of December 8, 2020 it is u-boot-2020.10_
```bash
wget ftp://ftp.denx.de/pub/u-boot/u-boot-[version].tar.bz2
tar -xjf u-boot-[version].tar.bz2
```

Build the tools necessary to configure u-boot to set up the boot loader.
```bash
cd u-boot-[version]
sudo apt-get install flex bison
make sandbox_defconfig tools-only
```

Install the cross-compiler for the BeagleBone Black processor.
```bash
sudo apt-get install gcc-arm-linux-gnueabihf
sudo apt-get install gcc-arm-linux-gnueabi
```

Install the mkimage program to create the u-boot image.
```bash
sudo install tools/mkimage /usr/local/bin
```

Configure u-boot for the BeagleBone Black
```bash
cd u-boot-[version]
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- am335x_boneblack_vboot_defconfig
```

Cross compile u-boot.
```bash
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
```

## Step 4: Setting up the Linux Root File System on the microSD card
Download the Root File System [here](https://www.dropbox.com/s/k93doprl261hwn2/rootfs.tar.xz?dl=0)

Unzip the rfs tar file into the RFS directory of the microSD.
```bash
sudo tar -xvf ~/Downloads/rootfs.tar.xz -C /media/[username]/RFS
```

Move the contents inside the rootfs directory into the root directory of the RFS partition, and then remove the roofs directory.
```bash
sudo mv /media/[username]/RFS/rootfs/* /media/[username]/RFS/ && sudo rmdir rootfs
```

## Step 5: Compiling the Linux Kernel and Installing Kernel Modules
Create a development directory and navigate to your development directory.
```bash
mkdir ~/[proj_directory] && cd ~/[proj_directory]
```

Download the source code of the Linux Kernel for the BeagleBone Black. (This might take a while)
```bash
git clone git://github.com/beagleboard/linux
```

Change directory into the new linux directory
```bash
cd linux
```

Set the kernel version to the latest version
```
git checkout 4.1
```

Change directory into the Linux character drivers directory
```
cd ~/[proj_directory]/linux/drivers/char
```

Make a directory for the driver and change into this directory. Here you will put the driver source code and the Makefile to compile the driver source code. This directory must be the same name as the driver name when it get loaded. We use _morse_bbb_ for the directory name since in this tutorial the driver name is _morse_bbb.c_.

Create a Makefile in this directory that looks like this::

Modify the Kconfig file in the parent directory.
```bash
obj-$(CONFIG_MORSEBBB) += morsebbb.o
```

The Kconfig file should include the following lines under _Character Drivers_:
```bash
config MORSEBBB
	tristate “Morse Driver”
	default m
	---help---
		Select this option to enable the morse driver
```

Modify the Makefile. The Makefile include the following line this:
```bash
obj-$(CONFIG_MORSEBBB) += morsebbb/
```

Change into the cloned BeagleBone Black kernel directory.

W

sudo make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- CFLAGS_MODULE=-fno-pic -j4 modules

make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- INSTALL_MOD_PATH=/media/[username]/RFS/ CFLAGS_MODULE=-fno-pic modules_install
