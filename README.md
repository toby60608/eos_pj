"# eos_pj"   

http://tech-marsw.logdown.com/blog/2013/08/17/git-notes-github-n-person-cooperation-settings 操作說明  

https://www.wongwonggoods.com/raspberry-pi-3/raspberry-pi-3-camera-module/  pi照相  

https://www.wongwonggoods.com/raspberry-pi-3/raspberry-pi-3-camera-module/  lcd讀取  

https://github.com/WiringPi/WiringPi/blob/master/examples/lcd-adafruit.c lcd 呼叫

#缺少cv2  
pip install opencv-python  
pip install opencv-contrib-python  
pip install opencv-python-headless  
pip install opencv-contrib-python-headless  

#缺少ssim  
pip install -U scikit-image  

#lcd用的lib  
sudo apt install wiringpi  
(可改用以下git clone https://github.com/WiringPi/WiringPi 並進資料夾 下sudo ./build)  
安裝確認下sudo gpio readall  

#在rpi板子上下載板子kernel的build  
sudo apt install raspberrypi-kernel-headers  

#當上面抓不到build表示太新請回復最新穩定版  
sudo apt-get update; sudo apt-get install --reinstall raspberrypi-bootloader raspberrypi-kernel  

------------------WiringPi只能在rpi板make---------------------   
0.需先開啟rpi板子的i2c  
1. cd /WiringPi/examples  
2. make lcd-eos7  
3. 會產生出執行檔(system(sudo /path light_color line1))  

------------------Run Prog---------------------    
1. make  
2. sudo insmod driver.ko  
3. cd /dev  
4. sudo chmod 777 eos7_driver  
5. ./server port  
6. ./client ServerIp port  
 

