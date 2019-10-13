# Bluetooth Multiplexer

This is a project log of a device idea in its infancy.

Almost all bluetooth audio devices are limited to connect to one speaker/pair of headphones even though it isnt't prohibited by the bluetooth standard.
Some modern android phones support it but after some time googleing a few problems arose:

* The multi connect is implemented on the system level 
* App solutions don't work
	* one can't access the master audio stream from an app
	* low level bluetooth stack access is not permitted in android studio
	* (may actually be false if there is a way please tell me)

So the plan is to build a device that connects to multiple headphones/speakers and acts like a speaker itself so one can connect to and it forwards the data to the speakers.
---

## Project Notes
### Basics

* The ESP32 bluetooth/wifi module will be used because its affordable and its powerful enough to handle the data, unlike arduinos with HC5 bt trancievers.
* The bluetooth stack will be bluekitchens's btstack because i know for a fact that it's low level access allows to send audio data to multiple devices.
* Two ESP32 will be neccessary because one can't simultaniously act as a2dp src and snk

#### To Do (Basics):
- [ ] Write all the code
	- [ ] GAP discovery and proactive connecting to multiple headphones
	- [ ] A2DP source that sends to all the headphones simultaniously
* final code will probably look like a mixture between the btstack gap and a2dp examples

### Interfacing

* A 16 x 2 lcd display controlled by i2c will be used.
* For input a rotary encoder with integrated button will be used.

#### To Do (Interfacing):
- [ ] Write all the code
	- [ ] Since btstack (the library) doesnt work with the arduino ide all the librarys (i2c lct and rotary encoder) will have to be written from the grond up for the esp 32 board.
		- [ ] Rotary encoder: Will be ideally one function with a static variable that increments and decrements according to the input.
		- [ ] I2C LCD: should support marking (inverting), writing text, and scrolling using a display struct and the esp32 i2c driver.

### Extras

* While at it a feature for aux audio input would be pretty nice.
	* so the final product can transmit BT -> BT, AUX -> BT and BT -> AUX
	* still have to figure out a way to have a single audio jack act both as input and output debending on use setting 
		* so connect a dac and a adc to the same audiojack contact (somehow still trying to figure this one out) 