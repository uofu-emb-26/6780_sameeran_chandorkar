Powershell: 
usbipd list
usbipd bind --busid 1-1
usbipd attach --wsl --busid 1-1

WSL (inside VScode): 
cmake -B build -S ./
cmake --build build --target all
cmake --build build --target flash_lab1