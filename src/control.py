import sys
import yaml
import time
#import serial
from PyQt5.QtGui import QIcon
from PyQt5.QtCore import QTimer
from PyQt5.QtWidgets import QPushButton, QApplication, QGridLayout, QWidget, QHBoxLayout

WDW_ICON = 'logo.png'
WDW_TITLE = 'Manipulation Control'
with open('style.css') as f:
    STYLE = f.read()
FWD_BTN, FWD_NUM = 'w', '1'
BWD_BTN, BWD_NUM = 's', '2'
LFT_BTN, LFT_NUM = 'a', '3'
RGT_BTN, RGT_NUM = 'd', '4'
RST_BTN, RST_NUM = 'esc', '7'
UP_BTN,  UP_NUM = 'r', '5'
ACT_BTN, ACT_GNUM, ACT_RNUM = 'backspace', '8', '9'
DOWN_BTN, DOWN_NUM = 'f', '6'


#arduino = serial.Serial(port = '/dev/ttyACM0', baudrate = 115200, timeout = 0.1)


class ArduinoControl:

    button = ''
    closed = True
        
    def write_arduino(self):
        print(self.button)
        #arduino.reset_input_buffer()
        #arduino.write((num+'\n').encode('utf-8'))
        ##data = arduino.readline().decode('utf-8').rstrip()
        #data = arduino.readline()
        #print(data)
        time.sleep(0.1)


class App(QWidget):

    timer = QTimer()
    arduinoControl = ArduinoControl()
    
    def __init__(self):
        super().__init__()
        self.main_UI()
        self.left_UI()
        self.center_UI()
        self.right_UI()
        self.timer.timeout.connect(lambda: self.every_second_while_pressed())


    def main_UI(self):
        self.mainLayout = QGridLayout()
        self.mainLayout.setHorizontalSpacing(50)
        self.setLayout(self.mainLayout)
        self.setWindowIcon(QIcon(WDW_ICON))
        self.setWindowTitle(WDW_TITLE)
        self.setStyleSheet(STYLE)


    def left_UI(self):
        layout = QGridLayout()
        self.mainLayout.addLayout(layout, 0, 0)
        fwd = QPushButton(
            text = '↑', 
            objectName = 'forward_backward',
            pressed = lambda: self.on_press(FWD_NUM),
            released = lambda: self.on_release(),
            shortcut = FWD_BTN
        )
        bwd = QPushButton(
            text = '↓',
            objectName = 'forward_backward',
            pressed = lambda: self.on_press(BWD_NUM),
            released = lambda: self.on_release(),
            shortcut = BWD_BTN
        )
        lft = QPushButton(
            text = '←', 
            objectName = 'left_right',
            pressed = lambda: self.on_press(LFT_NUM),
            released = lambda: self.on_release(),
            shortcut = LFT_BTN
        )
        rgt = QPushButton(
            text = '→', 
            objectName = 'left_right',
            pressed = lambda: self.on_press(RGT_NUM),
            released = lambda: self.on_release(),
            shortcut = RGT_BTN
        )
        layout.addWidget(fwd, 1, 2)
        layout.addWidget(bwd, 4, 2)
        layout.addWidget(lft, 3, 1)
        layout.addWidget(rgt, 3, 3)


    def center_UI(self):
        layout = QHBoxLayout()
        self.mainLayout.addLayout(layout, 0, 1)
        rst = QPushButton(
            text = 'Reset', 
            objectName = 'reset', 
            clicked = lambda: self.on_click(RST_NUM), 
            shortcut = RST_BTN
        )
        layout.addWidget(rst)


    def right_UI(self):
        layout = QGridLayout()
        layout.setVerticalSpacing(10)
        self.mainLayout.addLayout(layout, 0, 2)
        up = QPushButton(
            text = '↑', 
            objectName = 'up_down',
            pressed = lambda: self.on_press(UP_NUM),
            released = lambda: self.on_release(),
            shortcut = UP_BTN
        )
        self.act = QPushButton(
            text = 'Release', 
            objectName = 'act_release',
            clicked = lambda: self.grab_release(),
            shortcut = ACT_BTN
        )
        down = QPushButton(
            text = '↓',
            objectName = 'up_down',
            pressed = lambda: self.on_press(DOWN_NUM),
            released = lambda: self.on_release(),
            shortcut = DOWN_BTN
        )
        layout.addWidget(up, 0, 0)
        layout.addWidget(self.act, 1, 0)
        layout.addWidget(down, 2, 0)


    def on_press(self, button):
        self.arduinoControl.button = button
        self.timer.start()


    def on_release(self):
        self.timer.stop()


    def every_second_while_pressed(self):
        self.arduinoControl.write_arduino()


    def on_click(self, button):
        self.arduinoControl.button = button
        self.arduinoControl.write_arduino()


    def grab_release(self):
        if self.arduinoControl.closed:
            self.on_click(ACT_RNUM)
            self.act.setText('Grab')
            self.arduinoControl.closed = False
            self.act.setObjectName('act_grab')
        else:
            self.on_click(ACT_GNUM)
            self.act.setText('Release')
            self.arduinoControl.closed = True
            self.act.setObjectName('act_release')
        self.act.setShortcut(ACT_BTN)
        self.setStyleSheet(STYLE)


if __name__ == '__main__':
    app = QApplication(sys.argv)
    App().show()
    sys.exit(app.exec_())

#arduino.close()