# Generates colorbar image at the size 320x240

Colorbars(320,240)

# Converts the image to greyscale

Greyscale()

# Adds framenumbers to the image

Showframenumber()

# Selects the range 0 to 50000

Trim (0, 50000)

# Selects every second frame. Fps is now half of 29.97

SelectEven()

# Selects first 1000 frames inside the stream, and assigns it to variable substream

SubStream = Trim (0, -999)

# Loop the substream 25 times

Substream = Loop(Substream,25)
 
# Put the subtream on top of the original

Stackvertical(substream)

