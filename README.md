# Komplementary Kontrol #
_Linux support for the Komplete Kontrol A-series keyboard by Native Instruments_

## DISCLAIMER : USE THIS AT YOUR OWN RISK ##
I cannot be held responsible for any problems that might arise
from using these utilities.

Now that that is out of the way...

## About ##
This is a set of utilities to make the Komplete Kontrol A-series 
keyboards usable under Linux.

This has been developed using a A25 but I don't see any reason why 
this could not work on the A49 or other keyboards.

It consists of two utilities:
- `komplement`
- `konfigure` 

The `komplement` tool is the thing that makes the non-MIDI buttons 
usable on your system. See below.

The `konfigure` tool generates SysEx packets that can be sent
to the device to configure the current preset (i.e. that maps the 
rotary dials to actual MIDI events). See below.


### Compiling and installing ###
The `konfigure` requires libasound2-dev, the `komplement` tool additionally
requires libhidapi-libusb0.

To install required libraries, run
    `apt install libhidapi-libusb0 libasound2-dev`

To compile the binaries, simply run 
    `make all`
    
If you want to install it on your system, run 
    `sudo make install`
    
> Note: Installation will also create the folders `/usr/share/komplement` where 
> the mappings are stored, and `/usr/share/konfigure` for the presets. This is 
> where the utilities will try to find the files if they do not exist in the 
> path (i.e. running `komplement -m rosegarden.map` will load it 
> from `/usr/share/komplement/rosegarden.map`).

## komplement ##
This basically works by reading the relevant /dev/input/hiddevX and 
mapping that to keys useful for your favourite software. 

This means that if your software is not the active window, any other 
active application will receive the keypresses. This is something 
to keep in mind.

Due to how it works, it needs to run while you want to use the keyboard 
in your DAW. 

If you are done using it, you can simply hit Ctrl+C to abort it and it shall
gracefully exit.

#### Permissions ####
This utility required *read* access to `/dev/input/hiddevX` and *write* access 
to `/dev/uinput` for sending keypresses.
> Note that it doesn't read from uinput, so your passwords are safe.

Access to these can be accomplished by either running the `komplement` binary 
as root, or allowing user access to the necessary files. 

Obviously, the latter is encouraged. (Running as root is bad, mkay.)

The `setfacl` utility can be used to allow write access to the `/dev/uinput` file. 
```
    $> sudo setfacl -m u:$USER:w /dev/uinput
```
> Note that changes with `setfacl` are not permanent and will need to be applied 
> again after a reboot.

For convenience, I have created the file `udev/55-komplete-kontrol.rules` that
can be used to set up USB device access for the group `audio` (chances are you 
have already added yourself to that group). If not, edit the rules file to 
a group you do belong to.

After copying that file (as root) to `/etc/udev/rules.d` and then running
    `sudo udevadm control --reload-rules`

You can unplug / plug the device so that permissions are set-up properly and 
then you no longer need to run the utility as root.


#### Mapping files ####
An example mapping is supplied in `mappings/rosegarden.map` that maps 
useful buttons to functionality in Rosegarden. This file can serve as a 
basis to create mappings for other software. 

Check the file `doc/Supported_Keys.md` for a list of all the supported keys.

(If you do create your own, please share your mapping files so others 
can use it too!)


#### Examples ####
Example usage that loads the Rosegarden mapping and uses /dev/input/hiddev0 
and /dev/uinput as input and output respectively:
```
$> ./komplement -m mappings/rosegarden.map -i /dev/input/hiddev0 -o /dev/uinput
```

(The above example assumes that the permissions are correctly set up.)

## konfigure ##
This is a utility that creates a SysEx file that can be sent to the device
to configure it the rotary knobs.

#### Mapping files ####
An example mapping file is supplied in `presets/basic_cc.pst` that 
maps a few CC to the buttons.

#### Examples ####
Example that writes it to a separate file that you can then send
to the appropriate device with the `amidi` utility:
```
$> ./konfigure -i presets/basic_cc.pst > /tmp/sysex.syx
$> amidi -s /tmp/sysex.syx
```

It is also possible to send it directly to the device like so:
```
$> ./konfigure -i presets/basic_cc.pst -p "hw:3,0,0"
```
