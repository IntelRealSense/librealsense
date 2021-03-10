echo "Installing R200 connectivity workaround. Unplug any R200 connected to your machine."
echo "Press Enter to continue..."

read p

echo "Copying scripts"
sudo cp ../config/usb-R200-in /usr/local/bin/
sudo cp ../config/usb-R200-in_udev /usr/local/bin/

sudo chmod 755 /usr/local/bin/usb-R200-in
sudo chmod 755 /usr/local/bin/usb-R200-in_udev

echo "Done."
