# foo_lifx
A foobar2000 component that visualises currently played music with a Lifx bulb connected to the same network.
Props to the guy who made https://github.com/codemaster/lib-lifx for communicating with the bulbs!

## Preferences
* Enable/Disable
* Brightness (%)
* Offset (ms)
    * The length of audio visualised before communicating with the bulb 
    * Increase this to reduce latency
* Intensity
    * Normal/High/Very High/Maximum
    * Higher levels increase the overall brightness determined by the visualisation   
* Colour Cycle
    * Cycles through the colour wheel over a duration in seconds
* Hue & Saturation
    * Custom values to use instead of the colour cycle

## Binary
You can get the binary [here](https://mega.nz/#!KtIxALqB!MaZNPVw8-_Ri5Ti24oQpG2Uzk3j9mp0LWXTonKqkOg0).

Simply drop the binary into the foobar2000's components folder.