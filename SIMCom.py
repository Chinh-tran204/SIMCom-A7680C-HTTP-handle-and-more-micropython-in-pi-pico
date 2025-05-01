import utime


#use if you want interrupt while it's posting change to utime.sleep for no interrupt
def clock_irq(second):
    for i in range(clock_irq):
        utime.sleep_ms(100)


class SIMCom():
    def __init__(self, UART, PHONE_NUM):
        self.UART = UART
        self.NUM = PHONE_NUM                                    #only use for SMS sending
        self.data = ''
        self.initS = 0
        self.warning = 0
        self.firstHandShake = 1
        self.authorization = 'Basic Auth_BASE64'                #add your own authorization code if had
        self.contentType = 'application/json'
    def handShake(self):                                        #check to see if the SIMCom is ready to commute
        #reset handshake bit
        timeOut = 500
        if self.firstHandShake == 1:
            startTime = utime.ticks_ms()
            while utime.ticks_diff(utime.ticks_ms(), startTime) <= timeOut:
                self.UART.read()                                    
                self.UART.write('AT\r')                             #init heart beat
                #buffer sleep for msg send
                utime.sleep_ms(100)
                if self.UART.any():
                    self.data = self.UART.read().decode().strip()    
                    if self.data == 'AT\r\r\nOK':                   #heart beat success
                        self.UART.write('ATE0\r')                   #turn off echo
                        #clear uart buffer
                        self.UART.read()                            
                        self.initS = 1
                        self.firstHandShake = 0
                        self.UART.read()
                        break                                       #indicate handshake success
        elif self.firstHandShake == 0:
            startTime = utime.ticks_ms()
            while utime.ticks_diff(utime.ticks_ms(), startTime) <= timeOut:
                self.UART.read()                                    
                self.UART.write('AT\r')                             #init heart beat
                #buffer sleep for msg send
                utime.sleep_ms(100)
                if self.UART.any():
                    self.data = self.UART.read().decode().strip()    
                    if self.data == 'OK':                   #heart beat success                           
                        self.initS = 1
                        self.UART.read()
                        break                                       #indicate handshake success
        if self.initS == 0:
            return 1
    def decodeMsg(self, data):
        startIndex = data.find(b'{')
        endIndex = data.find(b'}', startIndex + 1)
        data = data[startIndex:endIndex + 1]
        return data.decode()
    def MSG(self, msg):                                          #sending message to a destinated phone num
        confBit = 0
        timeOut = 1000
        startTime = utime.ticks_ms()
        self.UART.read()
        self.handShake()
        while utime.ticks_diff(utime.ticks_ms(), startTime) <= timeOut and self.initS:                                          #handshake successfully
            self.UART.read()
            self.UART.write('AT+CMGF=1\r')
            utime.sleep_ms(200)
            while not self.UART.any():
                pass
            if self.UART.read().decode().strip() == 'OK':
                numFormat = 'AT+CMGS=\"'+ self.NUM + '\"\r\n'
                self.UART.write(numFormat)
                utime.sleep_ms(100)
                self.UART.write(msg)
                self.UART.write(bytes([26])) #submit to sen the sms
                confBit = 1
                self.initS = 0
                break
            else:
                return 2
        if not confBit:
            return 2
    def HTTP_GET(self):                                             #auto run and return 0.1 if success
        trials = 0
        data = ''
        self.UART.read()
        self.handShake()
        if self.initS:
            self.handShake()
            #Enable SNI
            self.UART.write('AT+CSSLCFG="enableSNI",0,1\r')
            utime.sleep_ms(100)
            #clean the UART port
            self.UART.read()
            #start HTTP service and info
            self.UART.write('AT+HTTPINIT\r')
            utime.sleep_ms(100)
            self.UART.write('AT+HTTPPARA="URL","https://test_api.com"\r')       #change to the http or api you're working with
            utime.sleep_ms(100)
            self.UART.write(f'AT+HTTPPARA="USERDATA","Authorization: {self.authorization}"\r')
            utime.sleep_ms(200)
            while trials < 3 and (data == '' or data == None):
                utime.sleep_ms(100)
                self.UART.read()
                #get action
                self.UART.write('AT+HTTPACTION=0\r')
                #buffer time
                utime.sleep(3)
                self.UART.read()
                utime.sleep_ms(100)
                #read the respond
                self.UART.write('AT+HTTPREAD=0,300\r')
                utime.sleep(2)
                data = self.UART.read()
                trials = trials + 1
                if data != '' and data != None:
                    data = self.decodeMsg(data)
            self.UART.write('AT+HTTPTERM\r')
            utime.sleep_ms(100)
            self.UART.read()
            if data != '' and data != None:
                if data == '{"success":true,"authorized":true}':
                    return 0.1 
                else:
                    return 4.1
            else:
                return 4
        else:
            return 1
    #update when timer comes
    def postUpdate(self,payload):
        #info
        url = "https://test_api.com"                                #change to the api your working with
        payload = payload                                           #string type other base on your api setting
        data = ''                                                   #echo message if post success full
        trials = 0
        self.UART.read()
        self.handShake()
        if self.initS and wakeUp:
            self.handShake()
            #Enable SNI
            self.UART.write('AT+CSSLCFG="enableSNI",0,1\r')         #if remove and still can do post then remove it, keep it for safe
            utime.sleep_ms(100)
            self.UART.read()
            #start HTTP service and info
            self.UART.write('AT+HTTPINIT\r')
            utime.sleep_ms(200)
            self.UART.read()
            self.UART.write(f'AT+HTTPPARA="URL","{url}"\r')
            utime.sleep_ms(150)
            self.UART.read()
            self.UART.write(f'AT+HTTPPARA="CONTENT","{self.contentType}"\r')
            utime.sleep_ms(100)
            self.UART.read()
            self.UART.write(f'AT+HTTPPARA="USERDATA","Authorization: {self.authorization}"\r')
            utime.sleep_ms(200)
            #try agian of fail
            while trials < 3 and (data == '' or data == None and data != '{"success":true}'):
                utime.sleep_ms(100)
                self.UART.read()
                #set POST data length
                data_length = len(payload)
                self.UART.write(f'AT+HTTPDATA={data_length},10000\r')
                utime.sleep_ms(200)
                self.UART.read()
                #post action
                self.UART.write(payload)
                clock_irq(4)
                self.UART.read()
                #posting
                self.UART.write('AT+HTTPACTION=1\r')
                clock_irq(5)
                utime.sleep_ms(100)
                #read the respond
                self.UART.write('AT+HTTPREAD=0,300\r')
                clock_irq(2)
                data = self.UART.read()
                trials += 1
                if data != '' and data != None:
                    data = self.decodeMsg(data)
            #terminate the http
            self.UART.write('AT+HTTPTERM\r')
            utime.sleep_ms(100)
            self.UART.read()
            if data != '' and data != None:
                if data == '{"success":true}':
                    return 0.1
                else:
                    return 3.1
            else:
                return 3
        else:
            return 1