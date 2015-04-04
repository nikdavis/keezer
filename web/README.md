## What is this?!

This was an archaic live graph of the temperature. It requires a locally running:
- apache server with modpython (or other flavor)
- mysql server
- keezer board with usb connected locally
I think that was it.

## What files do what?
- ```average_temp.py```
  - Used to find average (integrated) temp to study keezer's over/undershoot
- ```index.html```
  - View live temp graph
- ```pulltemp.py```
  - Server-side python script to dump X # of reading in JSON
- ```readtemp.py```
  - A (really basic) background script to read temps from the keezer board