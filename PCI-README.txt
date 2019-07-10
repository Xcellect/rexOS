Before we had drivers for mouse and keyboard, we did not have to worry about them having a fixed interrupt and port. They are standardazied I/O devices and every computer is expected to have them. However, a non-standard device (ie. a specific brand network/graphics chip - one or multiple) would not be the same for all computers.
This is why we need PCI (Peripheral Component Interconnect). They do not have hardcoded interrupt and ports. Because you don't know what dev is on what port/what its interrupts it's going to send you. There are too many different devices.
Solution: Talk to the PCI Controller.
PCI Controller:
	- has upto 8 BUSes
