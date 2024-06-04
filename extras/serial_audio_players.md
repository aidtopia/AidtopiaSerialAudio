# Serial Audio Players

Adrian McCarthy
DRAFT 2019-05-11

## Introduction

There is a plethora of audio player modules (MP3 and WAV) that are controlled by serial commands.  The most common ones I've encountered are branded DFPlayer Mini and Catalex.  But these are likely brands applied to whitelabel products from various manufacturers.  For example, the DFPlayer Mini appears to be a branded version of MP3-TF-16P from Flyron Tech.

But there are multiple versions of each, as well as knock offs and outright counterfeits.  They implement different subsets of features and each have their own constellation of bugs.

Documentation is fragmented, poorly translated, incomplete, and sometimes inaccurate.  There are many libraries out there, some of which are pretty good, but usually just for one or two specific versions.

When purchasing a serial audio player, it can be difficult to know exactly which one you'll get.  My goal is to produce more complete documentation for these boards, and to offer a library that adapts to the quirks of whichever module you drop into your project.  I want an easy-to-use interface, without delays, that robustly handles these modules, regardless of which one you plug into your project.

### Audio Modules

* Catalex Serial MP3 Player
* DFRobot DFPlayer Mini
* Flyron Tech MP3-TF-16P or FN-M16P

The BY8001-16P is a different beast, perhaps based on an older version of these chips.  Is it worth supporting?  The chip has a serial mode controlled by the absence or presence of three resistors.  The pinout is different.  There's a Picaxe project whose PCB has spaces for plugging in either type of player.

## Theory of Operation

Most of the datasheets provide a message reference with little information about how to use them together.  We'll start with a practical exploration of how to connect and use the module, and save the message reference for later.

After the message reference is documentation for the AidtopiaSerialAudio library.

### Electrical Connections

The Catalex-style players are straightforward to connect to a 5-volt microcontroller.

The DFPlayer Minis, as well as the ones that look like it, have some non-obvious requirements worth some attention.

#### Power

You can power the DFPlayer Mini with a maximum of 5 volts DC (some datasheets say 5.2 volts).  The typical supply is 4.2 V.  You can insert a diode between a 5-volt supply and the VCC of the DFPlayer Mini to drop the voltage to approximately 4.3 V, which may improve the lifespan and reliability of the device.

DFPlayer Mini actually runs at 3.3 V.  An onboard regulator adjusts the supply voltage as needed.

Remember to tie the grounds between the audio module and the microcontroller.

#### Level Shifting for Serial Communication

The DFPlayer Mini's serial lines use and expect only 3.3 volts.

When using a 3.3-volt microcontroller, no special considerations are necessary.

When using a 5-volt microcontroller, however, you must use level shifting between the microcontroller's TX pin to DFPlayer Mini's RX pin.  If you don't, the TX pin may provide more current than the RX pin can handle, which could damage the device.  Here are three ways to handle this:

* Limit the current with a 1 k&ohm; resistor in series.  Many of the datasheets endorse this approach.

* Use a voltage divider to bring the 5-volt output from the microcontroller down to the 3.3-volt level.

* Use an opto-isolator to relay the 5-volt signals at the 3.3-volt level.  The hobby electronics suppliers have a variety of small modules called level shifters for doing exactly this.

In the opposite direction, from the chip's TX to the microcontroller's RX, a direct connection is fine.  A typical 5-volt microcontroller uses a threshold of 3.0 V to determine whether a digital input is high.  Since 3.3 V is higher than 3.0 V, the microcontroller shouldn't have a problem.

#### DAC outputs

The DFPlayer Mini provides access to the outputs of the DACs (digital-to-analog converters) for the left and right channels.

The DAC signals are DC biased about 200 mV.

For an audio file at maximum volume, the DAC signals range approximately from -600 mV to +600 mV (relative to the bias level).  That's a peak-to-peak range of 1.2 V and an RMS voltage of 425 mV.  These signals can power small earphones directly or be routed into an amplifier.

When a file starts playing, the amplitude ramps up to the nominal value in about 50 ms.

Do not send a DAC output directly to a GPIO pin.  Most microcontrollers require inputs to be between 0 V and the chip's operating voltage (e.g., 5 V for many common Arduino modules).  The DAC outputs can swing slightly negative, which could damage the microcontroller.  You could bias the signal to ensure it's always positive and also clip it to the allowed range to make it safe for interfacing to a GPIO pin.

#### SPK Outputs

There are two SPK outputs on the DFPlayer Mini.  They do _not_ correspond to the left and right audio channels.  The left and right channels are mixed down to mono and amplified by a low-power amplifier built into the module.  The SPK+ and SPK- signals together comprise a balanced output.

You can connect a small 8-ohm 3-watt speaker directly to the SPK pins.

The SPK outputs range approximately from 0 V to 4 V.  When no sound is being played, they float at roughly the middle of their range.  When sound is being played, the SPK- output is the inverse of the SPK+.

#### Optional USB Socket

#### Busy

### Source Devices

Most of the serial audio players can play files stored on a micro SD card, specifically an SD or SDHC card formatted as a FAT32 file system.  Sometimes these are labeled TF for True Flash.

With the Catalex-style modules, an SD card is the only option for storing the sound files to be played.

Some modules have onboard flash memory.  These are often listed as audio recorders rather than players.  Getting data onto this flash memory is currently outside the scope of this project.

Some modules may allow for a USB drive to be attached.  A USB connector may be included onboard the module, or, like the DFPlayer Mini, require an external USB connector.

Some of the USB-capable modules not only allow a USB drive to be attached, but also a computer.  The the physical USB port may sometimes be considered a USB drive and sometimes an auxilliary device (AUX).  Presumably, a connection to a computer can be used to transfer data to the onboard flash memory, but that's outside the scope of this project.

Some modules allow for two or more source devices.  When they power-up they will usually automatically select one of the devices to be the current device.  But it's a good idea to explicitly select the device you want before trying to play audio files from it.

### Serial Protocol

The serial communication is fixed at 9600 baud for all the devices I've studied.

If using Arduino, you can use a hardware or software serial port.  Either way, make sure you do the necessary level shifting as described earlier.

#### Message Format

Serial communication with the audio player is broken into messages.  Messages may be sent to the play and received from the player.

Some simple applications ignore the messages from the player altogether.

| Purpose | Value | Notes |
| :--- | :---: | :--- |
| Start Byte | 0x7F | Indicates the beginning of a message. |
| Version | 0xFF | Significance unknown. |
| Length | 0x06 (typically) | Number of message bytes, excluding Start Byte, Checksum, and End Byte.  The length is 0x06 for most messages, but it can be higher for Command 0x21, which can set a playlist. |
| Msg ID | (varies) | The following tables describe the meaning of the various message ID values. |
| Feedback | 0x00 _or_ 0x01 | Use 0x01 to request an acknowledgement of this message. |
| Param Hi | (varies) | High byte of the 16-bit parameter value used by some commands.  Commands that don't use the parameter should pass 0. |
| Param Lo | (varies) | Low byte of the 16-bit parameter value used by some commands.  Commands that don't use the parameter should pass 0. |
| Chksum Hi | (varies) | High byte of the _optional_ 16-bit checksum. |
| Chksum Lo | (varies) | Low byte of the _optional_ 16-bit checksum. |
| End Byte | 0xEF | Indicates the end of a message.  Note that it's almost impossible for Chksum Hi to be 0xEF, which is how a receiver can tell whether the message includes a checksum. |

Note that, instead of Param Hi and Param Lo, the Playlist command includes a list of one or more parameter bytes.  This is the one case where Length may be something other than 0x06.

The Feedback flag essentially requests that the module respond with an ACK message if the command is received.  The ACK doesn't necessarily mean the command was accepted or executed.  It simply means the message was received.

Messages sent to the player may optionally include the checksum.  If the checksum is present, both bytes are required, and the player will return a Bad Checksum message if the checksum is incorrect.

Messages from the player always include a checksum, but the controller may choose to ignore them.  Checksums are not essential when the controller and the player are in near proximity.  When communicating over a longer line that may be subject to electrical noise, checksums can be a good way to ensure reliable transmission.

The checksum is the two's complement of the 16-bit sum of the data bytes excluding the start byte, the end byte, and the checksum itself.  Since most (all?) microcontrollers use two's complement internally code samples usually compute this as:

```C++
const std::uint16_t sum = version + length + command + feedback + param_hi + param_lo;
return 0 - sum;
```

Depending on the compiler and the warning level, the above code can generate warnings about implicit conversions.  To avoid such warnings, I prefer to explicitly compute two's complement like this:

```C++
const std::uint16_t sum = version + length + command + feedback + param_hi + param_lo;
return ~sum + 1;
```

#### ACKs and Errors

#### Initialization

Upon power-up or sending the Reset (0x0C) command, the module initializes itself.  Depending on the number of source devices available and the number of folders and files on those devices, this can take anywhere from a couple hundred milliseconds to a few seconds.

If the modules resets successfully, it sends an Initialization Complete message (0x3F).  The low byte of the parameter is a bitmask indicating which source devices are available.

> In practice, if you have both an SD card and a USB drive inserted, the initialization complete message only sets the bit for the USB device.  So if you get an initialization complete mentioning only the USB, you can check if there's also an SD card by querying the number of files on the SD card.  If that returns a number other than 0, the SD card is also available.

If there are no source devices installed during a reset, initialization ends with an Error (0x40) and a code of 1, which means the module is unavailable or that there are no devices online.

In either case, you can track changes to the availability of source devices by watch for asynchronous Device Inserted (0x3A) and Device Removed (0x3B) messages.  Those seem to be quite reliable.

Once initialization is complete, the module will probably default to selecting one of the source devices as the "current" device.  If the USB is among the devices online, it will be selected.  If only the SD card is online, it will be selected.  Nonetheless, it is highly recommended to explicitly select the device you want with the Select Source (0x09) message.

Therefore the recommended procedure is:

1.  Reset the module.
2.  If you get an error code of 1, prompt the user to insert a source device and watch for the Device Inserted (0x3A) message.
3.  If you get an Initialization Complete (0x3F) message, verify which other devices are online by checking for non-zero file counts.
4.  Explicitly select the source device you want to use.
5.  Continue to watch for Device Inserted (0x3A) and Device Removed (0x3B) messages.

#### Module Parameters

##### Volume

DFPlayer Mini seems to be clamped to 30.

Upon initialization, Catalex reports a volume of 0, but it seems to default to the maximum setting.  Set volume commands work, and queries sent after setting the volume returns the last volume set.

Instead of clamping to 30, Catalex seems to use the lower 5 bits of the byte.  For example, if you set volume to 40, Catalex will confirm that it's set to 40, but the actual level is lower.  It's possible Catalex's top volume is 31 rather than 30.

##### Equalizer Profile

The devices have a pre-set selection of equalizer settings:

* Normal
* Pop
* Rock
* Jazz
* Classical
* Bass

#### Playing by Index

#### Playing by Folder and Track

#### Playing from the "MP3" Folder

#### Inserting an Advertisement

#### Stopping, Pausing, etc.

#### Reducing Power Consumption

When playing music at a typical volume through the amp to a 3 W, 8 Ohm speaker, the current ranges between 600 and 700 mA.  This is strange because I didn't think the power source could provide that much current.  Playing to the headphone jack draws about 27 mA.

| Mode | DFPlayer Mini | Catalex |
| :--- | ---: | ---: |
| Idle with DAC disabled | 16-17 mA | 10.5 mA |
| Idle | 17-20 mA | 13.5 mA |
| Sleeping | 20 mA | 13.5 mA |
| Playing with DAC disabled | 24 mA | 10 mA |
| Playing with DAC enabled | 27 mA | 19-20 mA |
| Playing to 3 W speaker | 600-700 mA | N/A |

TODO:  Update to check with USB sources

##### Sleep and Wake

Putting the module to sleep does not lower the quiescent current draw.  (For best results, ensure the player is stopped before putting it to sleep.)

The wake command is not supported by some modules.  Some modules will not even execute a reset command while sleeping.  Selecting a valid source device, however, will wake the device.  In a sense, when the device is asleep, it's as though no source device is selected.  For other modules, being asleep is as though the selected device is the "sleep" device.

##### Enabling and Disabling the DACs

A better way to save a few milliamps is to disable the DAC (0x1A,1), which drops the quiescent current to 16 mA.  Reenabling the DAC (0x1A,0) seems to work just fine.

## Message Reference

### ACKs and Errors

When a message is sent to the module with the feedback flag set, the module will acknowledge it by sending back an ACK message.  This simply means the message was received.  It's purpose is to ensure the communication channel is functioning correctly.

The parameter in the ACK message is undefined.  Some implementations seem to re-use the message buffer, so there may seem to be data in the parameter that appears meaningful, but it isn't.

If an error occurs while executing a command, the module will send a separate message to indicate the error, and this message will usually arrive after the ACK.  This means that you cannot know whether a command succeeded except by waiting for a period of time after the ACK to ensure that an error is not returned.

When sending commands, using feedback might be useful.  When sending queries, requesting feedback is unnecessary because the module should send a response or an error, making an extra ACK superfluous.

| Msg ID | Value | Parameter | Notes |
| :--- | :---: | :--- | :--- |
| Error | 0x40 | See error below | Sent by the module to report a problem. |
| ACK  | 0x41 | undefined | Sent by the module after receiving a message with the feedback flag set. |

| Error | Code | Meaning |
| :--- | :--- | :--- |
| Unsupported | 0x00 | The command or query received is not supported. |
| No sources | 0x01 | No storage device is selected. They module may still be initializing after a power-up or reset. |
| Sleeping | 0x02 | The module is sleeping.  See the Sleep and Wake commands for more information. |
| Serial Error | 0x03 | The message was not received because of an error with the serial communication.  Possible causes include incorrect baud rate, wrong message format, poor electrical connections, etc. |
| Bad Checksum | 0x04 | The checksum was incorrect, so the message was rejected. |
| File Out of Range | 0x05 | A file index parameter was higher than the number of files on the storage device. |
| Track Not Found | 0x06 | The specified track number (or folder number) was not found on the storage device. |
| Insertion Error | 0x07 | The advertisement track could not be played.  This can happen if the player is stopped, an advertisement is already playing, or the requested advertisement track was not found. |
| SD/TF Card Error | 0x08 | The module encountered a problem communicating with the Micro SD (True Flash) card. |

Note that the AidtopiaSerialAudio library reports a timeout as though it received an error message with the error code 0x100.

### Commands

Here are the commands that can be sent by the application to the serial audio player.  Note that different players support different subsets of these.

| Msg ID | Value | Parameter | Notes |
| :--- | :---: | :--- | :--- |
| Play Next File | 0x01 | 0 | Plays the next file by file index.  Wraps back to the first file after the last.  Resets the playback sequence to single. |
| Play Previous File | 0x02 | 0 | Plays the previous file by file index.  Resets the playback sequence to single. |
| Play File | 0x03 | _file index_ | This is the file system number, which is not necessarily the same as the number prefixed to the file name. TBD:  Verify range (reportedly 1-3000). |
| Volume Up | 0x04 | 0 | Increases one level unless the volume is already at level 30. |
| Volume Down | 0x05 | 0 | Decreases one level unless the volume is already at level 0. |
| Set Volume | 0x06 | DFPlayer Mini: 0-30<br>Catalex: 0-31 | Sets the volume to the selected value.  If the value is greater than 30, volume will be set to 30.  DFPlayer Mini defaults to 25.  Flyron claims default is 30. |
| Select EQ | 0x07 | 0: Normal<br>1: Pop<br>2: Rock<br>3: Jazz<br>4: Classical<br>5: Bass | On DFPlayer Mini, changing the EQ interrupts the current track. On Catalex, it doesn't. |
| Loop File | 0x08 | High: 0<br>Low: _file index_ | This command loops file index.  High byte must be zero to distinguish this from Loop Flash Track. |
| Loop Flash Track | 0x08 | High: _flash folder_<br>Low: _track_ | The high byte determines a folder in the flash memory source device and the low byte selects the track. |
| Select Source Device | 0x09 | 1: USB drive<br>2: SD card<br>4: Flash | When multiple sources are available, this command lets you select one as the "current" source.  AUX and FLASH are not readily available on most modules. |
| Sleep | 0x0A | 0 | Seems pointless. |
| Wake | 0x0B | 0 | If not supported, try Reset or Select Source Device. |
| Reset | 0x0C | 0 | Re-initializes the module with defaults.  Initialization is not instantaneous, and it typically causes the module to send a bitmask of which sources are available. |
| Resume<br>("unpause")| 0x0D | 0 | Resume playing (typically sent when playback is paused). Can also be used 100 ms after a track finishes playing in order to play the next track. |
| Pause | 0x0E | 0 | |
| Play From Folder | 0x0F | High: _folder number_<br>Low: _track number_ | Unlike Play File Number, this selects the folder by converting the high byte of the parameter into a two-decimal-digit string.  For example, folder 0x05 means "05" and folder 0x0A means "10".  Likewise the track number is matched against the file name that begins with a three or four digit representation of the value in the low byte of the parameter.  For example, 0x1A will match a file with a name like "026 Yankee Doodle.wav". |
| Amplifier | 0x10 | High: _enable_<br>Low: _gain_ | Enable/disable the amplifier and set the gain level.  This is independent from the volume.  Not supported by most modules. |
| Loop All Files | 0x11 | 1: start looping<br>2: stop looping<br> | Plays all the files of the selected source device, repeatedly. |
| Play From "MP3" Folder | 0x12 | _track_ | Like Play From Folder, but the folder name must be "MP3" and the track number can use the full 16-bits.  For speed, it's recommended that track numbers don't exceed 3000 (0x0BB8). Doesn't work on Catalex. |
| Insert From "ADVERT" Folder | 0x13 | _track_ | Interrupts the currently playing track and plays the selected track from a folder named "ADVERT".  When the inserted track completes, the interrupted track immediately resumes from where it left off.  There is no message sent when the inserted track completes, though, on the DFPlayer Mini, you could watch for the brief blink on the BUSY line.  Documentation seems to suggest that this be used to interrupt tracks from the MP3 folder, but it works regardless of where the original track is located.  For speed, it's recommended that track numbers don't exceed 3000.  If you try to insert when nothing is playing (stopped or paused), you'll get an insertion error. Doesn't work on Catalex. |
| Play From Big Folder | 0x14 | High nybble: _folder_<br>Low 12 bits: _track_ | Like Play From Folder but allows higher track numbers as long as the folder number can be represented in 4 bits. |
| Stop "ADVERT" | 0x15 | 0 | Stops play track from "ADVERT" folder and resumes the track that was interrupted when the advertisement began. |
| Stop | 0x16 | 0 | Stops all playback.  This is distinct from Pause in that you cannot Resume. Also resets the playback sequence to single. |
| Loop Folder | 0x17 | _folder number_ | Seems legit.  Weird in that it double-ACKs. |
| Random Play | 0x18 | 0 | Plays all the files on the source device in a random order. |
| Loop Current Track | 0x19 | 0: enable looping<br>1: disable looping | Send while a track is playing (not paused or stopped). |
| DAC Enable/Disable | 0x1A | 0: enable<br>1: disable | Enables or disables the DACs (digital to audio converters) which are the audio outputs from the module.  The DACs drive the headphone jack but also the small on-board amplifier for direct speaker connection.  Presumably disabling them when not necessary saves power. |
| Playlist? | 0x21 | series of byte-sized file indexes | TODO:  Learn more.  Note: Only command with a length that's not 0x06. Doesn't seem to work reliably.  Translations call this "combined play." |
| Play With Volume | 0x22 | High: _volume_<br>Low: _file index_ | Plays the specified file at the new volume. |
| Insert From "ADVERT*n*" Folder | 0x25 | High: _folder_ [1..9]<br>Low: _track_ [0..255] | Inserting an advertisement track from a top-level folder named "ADVERT*n*", where *n* is a digit from 1 to 9. |

### Asynchronous Notifications

The player sends notifications of events asynchronously.

| Msg ID | Value | Parameter | Notes |
| :--- | :---: | :--- | :--- |
| Device Inserted | 0x3A | _device_ | A storage device has been connected to the player. |
| Device Removed  | 0x3B | _device_ | A storage device has been disconnected from the player. |
| Finished USB File | 0x3C | _file index_ | A file on the USB drive has just finished playing. |
| Finished SD File | 0x3D | _file index_  | A file on the SD card has just finished playing. This notification is not sent for an advertisement has been "inserted". |
| Finished Flash File | 0x3E | _file index_ | A file in the Flash memory has just finished playing. |
| Initialization Completed | 0x3F | _devices_ | This notification is sent after powering on or resetting.  The parameter is a bitmap of the storage devices that are online. |

Some datasheets claim the Initialization Completed message (0x3F) can also be used as query.

### Queries

Responses to queries use the same message format as the queries themselves.  For example, querying the status by sending 0x42 results in a response that also uses 0x42, but the response will have the requested data in the parameter.  In the following table, the Query column describes the parameter for the query, and the Response column describes the interpretation of the response.

| Msg ID | Value | Query | Response | Notes |
| :--- | :---: | :--- | :--- | :--- |
| Status | 0x42 | 0 | High: _device_<br>Low: _state_ | Indicates the currently selected storage device (e.g., the SD card) and the state (e.g., Playing, Paused, Stopped) |
| Volume | 0x43 | 0 | _volume_ | Returns the current volume level, 0-30. |
| Equalizer | 0x44 | 0 | 0: Normal<br>1: Pop<br>2: Rock<br>3: Jazz<br>4: Classical<br>5: Bass | |
| Playback Sequence | 0x45 | 0 | 0: Loop All<br>1: Loop Folder<br>2: Loop Track<br>3: Random<br>4: Single | Indicates which sequencing mode the player is currently in. (Catalex reports Loop All upon startup, but will show the correct value when selected.) |
| Firmware Version | 0x46 | 0 | _version_ | DFPlayer Mini: 8.  Catalex: no response |
| USB File Count | 0x47 | 0 | _count_ | The _count_ is the total number of MP3 and Wave files found on the USB drive, excluding other files types, like .txt. |
| SD Card File Count | 0x48 | 0 | _count_ | The _count_ is the total number of MP3 and Wave files found on the SD card, excluding other files types, like .txt. |
| Flash File Count | 0x49 | 0 | _count_ | The _count_ is the total number of MP3 and Wave files found in the internal flash memory, excluding other files types, like .txt. |
| USB Current File | 0x4B | 0 | _file index_ | Returns the index of the current file for the USB drive. |
| SD Card Current File | 0x4C | 0 | _file index_ | Returns the index of the current file for the USB drive. |
| Flash Current File | 0x4D | 0 | _file index_ | Returns the index of the current file for the USB drive. (Catalex returns 256.) |
| Folder Track Count | 0x4E | _folder_ | _count_ | Query specifies a folder number that corresponds to a folder with a two-decimal digit name like "01" or "13".  Response returns the number of audio files in that folder. |
| Folder Count | 0x4F | 0 | _count_ | Returns the total number of folders under the root folder.  Includes numbered folders like "01", the "MP3" folder, and the "ADVERT" and "ADVERT*n*" folders. |
| Flash Current Folder | 0x61 | 0 | _folder_ | Returns the number of the current folder on the flash memory. |

Queries have a natural response or an error, so it's generally not necessary to set the feedback bit in query messages.  If you do set the feedback bit, you will typically receive an ACK message immediately after the response, and the ACK's parameter will repeat the parameter from the response.

## Aidtopia Serial Audio Library for Arduino

## Modules Tested

* DFPlayer Mini with AA1044HER406-94

Notes on model numbers:

The players often end with a suffix like -16P.  I believe the 16 means it uses 16-bit DACs, and the P means it's a player, that is, it's a module with some support circuitry and not the raw chip.

The chip numbers end with a suffix like -24SS.  I believe the 24 means it has 24-bit DACs.  The SS may indicate serial interface and stereo output.

The manufacturer of the YX chips (with numbers like YX5100, YX5200, YX6200, YX6300) appears to be yx080.com.  The YX chips seem to define this space, and other chip makers try to offer drop-in compatible versions.

## References

[Chinese datasheet](https://github.com/0xcafed00d/yx5300/blob/master/docs/YX5300-24SS%20DEBUG%20manual%20V1.0.pdf)  Includes the 0x21 playlist(?) command (which is not necessarily of length 0x06), and Current Flash Folder 0x61 query.

[A better, more complete translation of the datasheet]( http://www.flyrontech.com/uploadfile/download/20184121510393726.pdf)  Best description of commands, queries, and "ADVERT" feature.

[Catalex documentation](http://geekmatic.in.ua/pdf/Catalex_MP3_board.pdf)

[Picaxe SPE035](http://www.picaxe.com/docs/spe035.pdf) (a board for interfacing to a DFPlayer Mini)

[Garry's Blog](https://garrysblog.com/2022/06/12/mp3-dfplayer-notes-clones-noise-speakers-wrong-file-plays-and-no-library/) notes the variety of versions of just the DFPlayer Mini.

[DFPlayer Analyzer](https://github.com/ghmartin77/DFPlayerAnalyzer) is a project that helps to uncover the quirks of your particular module.
