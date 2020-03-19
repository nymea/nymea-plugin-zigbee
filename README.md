This plugin is the first attempt to add native zigbee support to nymea. The plugin depends on the 
[libnymea-zigbee](https://github.com/nymea/nymea-zigbee) which contains the stack implementation.


Following features are currently available:

* Add in the supported UART Hardware as a gateway and give control over the network
* Starting a Zigbee Network using the NXP JN5168/JN5169
* Permit devices to join the network
* Remove devices from the network
* Control the supported devices from this plugin

Follwoing features are planed:

* Provide generic classes for standardized Device Types like power sockets and different lights
* Add group and group management support
* Add scenes and scenes management support
* Manage bindings within the network
* Support different Zigbee Gateway Hardware
* Receive Zigbee Lightlink commands within nymea


# The network controller

Depending on your zigbee gateway hardware the controller can be added by discovery into nymea. During the setup the type of the stack can be specified.
Currently only the NXP stack is supported.

Once the Zigbee Controller has been added successfully, the network will be initialized automatically and started.



# Ikea TRÅDFRI

## TRÅDFRI remote

![Remote](docs/tradfri-remote-control.JPG)



# Lumi / Xiaomi / Aquara

# FeiBit


