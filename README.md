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
