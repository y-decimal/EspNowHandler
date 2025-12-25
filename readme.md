# EspNowHandler

This library serves as a low level packet based networking protocol.

It has custom defined packet types, up to 256 uniquely addressable Devices, 
automatic broadcast based pairing, persistent pairing data storage, and more.

You can define a custom packet type, and enter literally any byte level data you want.
Currently you still have to translate your packets into and from bytes, 
but I may implement automatic struct conversions later.

Documentation on usage is still being worked on, the library is not currently stable and 
subject to frequent, breaking changes.

It is currently not in a state ready for usage unless you know what you're doing.
