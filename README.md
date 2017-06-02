# foo_lifx
A foobar2000 component that visualises currently played music with a Lifx bulb connected to the same network.
Props to the guy who made https://github.com/codemaster/lib-lifx for communicating with the bulbs!

## Preferences
* Enable/Disable
* Brightness (%)
* Offset (ms)
    * The length of audio visualised before communicating with the bulb
    * Change this to affect smoothness
* Delay (ms)
    * Increase this to latency
* Intensity
    * Normal/High/Very High/Maximum
    * Higher levels increase the overall brightness determined by the visualisation   
* Colour Cycle
    * Cycles through the colour wheel over a duration in seconds
* Hue & Saturation
    * Custom values to use instead of the colour cycle

## Binary
You can get the binary [here](https://mega.nz/#!38wHWJIJ!yAy0SxSTCcPRdePAQm5oi7KKAItlcwC5kwvb_K-6u04).

Simply drop the binary into the foobar2000's components folder.