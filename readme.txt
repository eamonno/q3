==============================================================================
                                  To-do list
==============================================================================
  - BSP Curve support is needed
  - Loading of all the various entities in game in particular
    - spawn points
	- ambient sounds
	- static models
  - collission detection
  - some simple bot ai would be great
  - add a cfunclist console function
  - try controlling vsync with the SwapEffect being set to SWAPEFFET_COPY 
    if in windowed mode, though it may have performance implications
  - Change to using D3DPOOL_DEFAULT when loading textures to save having 
    duplicates of every texture in system memory, to do this they will all 
	have to be re-loaded if there is a device change. At present memory usage
	is relatively low so it isnt an issue
  - Dont check every D3DFORMAT for surface formats creating d3dinfo as many
    can never be valid as various surface types, would save a small amount of
	memory per device potentially. 
  - Add a reset() method to restore any video memory resources if the device
    has been lost, not sure how this can happen at present with everything in
	D3DPOOL_DEFAULT
  - Dont render anything if the app is idle or invisible (minimized etc)

==============================================================================
                                  Known bugs
==============================================================================
  - Console commands must have at least 1 paramater to work
  - Fullscreen mode is not working at present
  - CVF_HIDDEN cvars show on cvarlist
  - map models are drawn even when they cannot possibly be visible
