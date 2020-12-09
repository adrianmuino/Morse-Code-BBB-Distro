#include <linux/init.h>         // Macros used to mark up functions e.g., __init __exit
#include <linux/module.h>        // Core header for loading LKMs into the kernel
#include <linux/kernel.h>       // Contains types, macros, functions for the kernel
#include <linux/device.h>    // Header to support the kernel Driver Model
#include <linux/fs.h>        // Header for the Linux file system support
#include <linux/uaccess.h>    // Required for the copy to user function
#include <linux/string.h>    // Contains strcmp, strlen
#include <linux/mutex.h>    // Required for the mutex lock  functionality
#include <linux/timer.h>    // Contains timer_list, add_timer, del_timer, and mod_timer
#include <linux/jiffies.h>    // Contains msecs_to_jiffies
#include <asm/io.h>         // Required for ioremap to map physical addresses to virtual addresses

#define DEVICE_NAME "mcode"    ///< The device will appear at /dev/mcode using this value
#define CLASS_NAME  "test"    ///< The device class -- this is a character device drive

#define ONE_SEC 1000
#define CQ_DEFAULT 0
#define GPIO1_START_ADDR 0x4804c000
#define GPIO1_END_ADDR 0x4804ffff
#define GPIO1_SIZE (GPIO1_END_ADDR - GPIO1_START_ADDR + 1) // size of GPIO port 1 (8KB)
#define GPIO_SETDATAOUT 0x194   // address offset to set  bits in the GPIO_DATAOUT register
#define GPIO_CLEARDATAOUT 0x190   // address offset to clear bits in the GPIO_DATAOUT register
#define USR0 (1<<21)    // bit offset to access the top left LED on the BBB
#define USR1 (1<<22)    // bit offset to access the second LED on the BBB
#define USR2 (1<<23)    // bit offset to access the third LED on the BBB
#define USR3 (1<<24)    // bit offset to access the bottom left LED on the BBB

MODULE_LICENSE("GPL");            ///< The license type -- this affects runtime behavior
MODULE_AUTHOR("Adrian Muino");        ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("Multi-user morse code device driver for BBB");    ///< The description -- see modinfo
MODULE_VERSION("0.1");            ///< The version of the module

static int majorNumber;        ///< Stores the device number -- determined automatically
static int numberOpens=0;    ///< Keep track of how many times the device driver has been opened
static char mcstring[512] = {0};  ///< String passed from user program
static int  c = 0;  ///< Variable to iterate over all letters in mcstring
static volatile void* gpio_addr;
static volatile unsigned int* gpio_setdataout_addr;
static volatile unsigned int* gpio_cleardataout_addr;

static DEFINE_MUTEX(mcode_mutex);  /// A macro that is used to declare a new mutex that is visible in this file
                                     /// results in a semaphore variable ebbchar_mutex with value 1 (unlocked)
                                     /// DEFINE_MUTEX_LOCKED() results in a variable with value 0 (locked)

static struct class* mcodeClass = NULL; ///< The device-driver class struct pointer
static struct device* mcodeDevice = NULL; ///< The device-driver device struct pointer

// Function prototypes for the character driver -- must come before the struct file_operations definition
static int dev_open(struct inode*, struct file*);
static int dev_release(struct inode*, struct file*);
static ssize_t dev_write(struct file*, const char*, size_t, loff_t*);
static void timer_isr(unsigned long);
static char* mcodestring(int);
static void mcode_mcletter(char* mcstr, char* mcletter);
static void mcode_word(char* mcstr, char* word);
static void mcode_sentence(char* mcstr, char* str);

/** @brief Devices are represented as file structure in the kernel. The file_operations structure
 *  from /linux/fs.h lists the callback functions that you wish to associated with your file operations
 *  using a C99 syntax structure. char devices usually implement open, read, write and release calls
 */

static struct timer_list timer;

static struct file_operations fops = {
    .open = dev_open,
    .write = dev_write,
    .release = dev_release,
};

static int __init mcode_init(void) {
    printk(KERN_INFO "mcode: Initializing the LKM\n");
   
    // Try to dynamically allocate a major number for the device
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber<0){
       printk(KERN_ALERT "mcode: Failed to register a major number\n");
       return majorNumber;
    }
    printk(KERN_INFO "mcode: Registered correctly with major number %d\n", majorNumber);

    // Register the device class
    mcodeClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(mcodeClass)){                // Check for error and clean up if there is
       unregister_chrdev(majorNumber, DEVICE_NAME);
       printk(KERN_ALERT "mcode: Failed to register device class\n");
       return PTR_ERR(mcodeClass);          // Correct way to return an error on a pointer
    }
    printk(KERN_INFO "mcode: Device class registered correctly\n");

    // Register the device driver
    mcodeDevice = device_create(mcodeClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(mcodeDevice)){               // Clean up if there is an error
       class_destroy(mcodeClass);           // Repeated code but the alternative is goto statements
       unregister_chrdev(majorNumber, DEVICE_NAME);
       printk(KERN_ALERT "mcode: Failed to create the device\n");
       return PTR_ERR(mcodeDevice);
    }
    printk(KERN_INFO "mcode: Device class created correctly\n"); // Made it! device was initialized

    mutex_init(&mcode_mutex);       /// Initialize the mutex lock dynamically at runtime

    //set up members of the timer structure
    timer.expires = jiffies;
    timer.flags = 0;
    timer.function = timer_isr;

    add_timer(&timer);    //add timer to the list of kernel timers

    //map physical memory addresses to virtual memory addresses

    gpio_addr = ioremap(GPIO1_START_ADDR, GPIO1_SIZE);
       
    if(!gpio_addr) {
             printk(KERN_INFO "mcode: Failed to remap memory for GPIO Port 1.\n");
    }
       
       gpio_setdataout_addr = gpio_addr + GPIO_SETDATAOUT;
       gpio_cleardataout_addr = gpio_addr + GPIO_CLEARDATAOUT;

    *gpio_cleardataout_addr = USR0 | USR1 | USR2 | USR3;

    printk(KERN_INFO "mcode: device class created correctly, exiting __init\n"); // Made it! device was initialized
       return 0;
}

static void __exit mcode_exit(void) {
    mutex_destroy(&mcode_mutex);  /// destroy the dynamically-allocated mutex
    device_destroy(mcodeClass, MKDEV(majorNumber, 0));     // remove the device
    class_unregister(mcodeClass);                          // unregister the device class
    class_destroy(mcodeClass);                             // remove the device class
    unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
    del_timer(&timer);
    printk(KERN_INFO "mcode: Goodbye from the mcode LKM!\n");
}

static int dev_open(struct inode* inodep, struct file* filep) {
    if(!mutex_trylock(&mcode_mutex)){    /// Try to acquire the mutex (i.e., put the lock on/down)
                                           /// returns 1 if successful and 0 if there is contention
       printk(KERN_ALERT "testchar: Device in use by another process\n");
       return -EBUSY;
    }

    numberOpens++;
    printk(KERN_INFO "mcode: Device has been opened %d time(s)\n", numberOpens);
    return 0;
}

static int dev_release(struct inode* inodep, struct file* filep) {
    mutex_unlock(&mcode_mutex);          /// Releases the mutex (i.e., the lock goes up)
    printk(KERN_INFO "mcode: Device successfully closed\n");
    return 0;
}

static ssize_t dev_write(struct file* f, const char* buffer, size_t len, loff_t* offset) {
    int r;
    char b[256] = {0};

    //Buffer overflow prevention
    if (len > 255)
        return -EINVAL;

    r = copy_from_user(b, buffer, len);

    //Error copying buffer string
    if (r)
        return -EFAULT;

    c = 0;

    printk(KERN_INFO "mcode: Received word from the user\n");

    mcode_sentence(mcstring, b);

    mod_timer(&timer, jiffies);
    return len;
}

static void timer_isr(unsigned long data) {
    if(c < strlen(mcstring)){
        if (mcstring[c] == '-') {
            *gpio_setdataout_addr = USR0;
            mod_timer(&timer, jiffies + msecs_to_jiffies(3*ONE_SEC));
        }
        else if (mcstring[c] == '.') {
            *gpio_setdataout_addr = USR0;
            mod_timer(&timer, jiffies + msecs_to_jiffies(ONE_SEC));
        }
        else if (mcstring[c] == ',') {
            *gpio_cleardataout_addr = USR0;
            mod_timer(&timer, jiffies + msecs_to_jiffies(ONE_SEC));
        }
        else if (mcstring[c] == '*') {
            *gpio_cleardataout_addr = USR0;
            mod_timer(&timer, jiffies + msecs_to_jiffies(3*ONE_SEC));
        }
        else if (mcstring[c] == ' ') {
            *gpio_cleardataout_addr = USR0;
            mod_timer(&timer, jiffies + msecs_to_jiffies(5*ONE_SEC));
        }
        else{
            printk(KERN_INFO "mcode: Finished!\n");
            *gpio_cleardataout_addr = USR0;
            c = 0;
            memset(mcstring, 0, sizeof(mcstring));  //set array back to being empty
            mutex_unlock(&mcode_mutex);         //unlock the mutex
        }
        c++;
    }else{
        printk(KERN_INFO "mcode: Finished!\n");
            *gpio_cleardataout_addr = USR0;
            c = 0;
            memset(mcstring, 0, sizeof(mcstring));  //set array back to being empty
            mutex_unlock(&mcode_mutex);         //unlock the mutex
    }
}

static char * mcodestring(int asciicode)
{
   char *morse_code[40] = {"",
      ".-","-...","-.-.","-..",".","..-.","--.","....","..",".---","-.-",
      ".-..","--","-.","---",".--.","--.-",".-.","...","-","..-","...-",
      ".--","-..-","-.--","--..","-----",".----","..---","...--","....-",
      ".....","-....","--...","---..","----.","--..--","-.-.-.","..--.."};

   char *mc;   // this is the mapping from the ASCII code into the mcodearray of strings.

   if (asciicode > 122)  // Past 'z'
      mc = morse_code[CQ_DEFAULT];
   else if (asciicode > 96)  // lowercase
      mc = morse_code[asciicode - 96];
   else if (asciicode > 90)  // uncoded punctuation
      mc = morse_code[CQ_DEFAULT];
   else if (asciicode > 64)  // uppercase
      mc = morse_code[asciicode - 64];
   else if (asciicode == 63)  // Question Mark
      mc = morse_code[39];    // 36 + 3
   else if (asciicode > 57)  // uncoded punctuation
      mc = morse_code[CQ_DEFAULT];
   else if (asciicode > 47)  // Numeral
      mc = morse_code[asciicode - 21];  // 27 + (asciicode - 48)
   else if (asciicode == 46)  // Period
      mc = morse_code[38];  // 36 + 2
   else if (asciicode == 44)  // Comma
      mc = morse_code[37];   // 36 + 1
   else
      mc = morse_code[CQ_DEFAULT];
   return mc;
}

static void mcode_mcletter(char* mcstr, char* mcletter){
 int i;
 for(i=0; i < strlen(mcletter); i++){
  if(mcletter[i] == '.'){                       // Dot is 1 sec ON
   strcat(mcstr, ".");
  }else{                                        // Dash is 3 secs ON
   strcat(mcstr, "-");
  }
  if(i != strlen(mcletter)-1){                  //space between parts of the same letter is 1 sec OFF
   strcat(mcstr, ",");
  }
 }
}

static void mcode_word(char* mcstr, char* word){
int i;
 for(i=0; i < strlen(word); i++){
  mcode_mcletter(mcstr, mcodestring(word[i]));
  if(i != strlen(word)-1){           //space between different letters is 3 secs OFF
   strcat(mcstr, "*");
  }
 }
}

static void mcode_sentence(char* mcstr, char* str){
    char word[46] = {0};    // longest commonly used word is 45 characters long
    int i = 0, j = 0;
    while(i < strlen(str)){
        while(str[i] && str[i] != ' ')
            word[j++] = str[i++];
        mcode_word(mcstr, word);
        memset(word, 0, sizeof(word));  // set word string back to empty
        j = 0;
        if(str[i]){                     //space between words is 5 secs OFF
            strcat(mcstr, " ");
        }
        i++;
    }
}

module_init(mcode_init);
module_exit(mcode_exit);