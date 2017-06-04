# foo_lifx
A foobar2000 component that visualises currently played music with a Lifx bulb connected to the same network.
Props to the guy who made https://github.com/codemaster/lib-lifx for communicating with the bulbs!

## Preferences
* Enable/Disable
* Brightness (%)
* Offset (ms)
    * The length of audio visualised before communicating with the bulb
    * Change this to affect smoothness
* Intensity
    * Normal/High/Very High/Maximum
    * Higher levels increase the overall brightness determined by the visualisation   
* Colour Cycle
    * Cycles through the colour wheel over a duration in seconds
* Hue & Saturation
    * Custom values to use instead of the colour cycle

## Binary
You can get the binary [here](https://mega.nz/#!HoRUxZ4Y!nM58hCiamaLp7Wd-OFigDqpesk5vGDhz__IHyacZHqo).

Simply drop the binary into the foobar2000's components folder.