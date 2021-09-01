import serial

class VetoControl:

    def __init__(self, address='/dev/tty.usbmodem1431'):
        self.pos_up = 0
        self.pos_down = 0
        #16 revolutions for 1 inch=25.4mm
        #1/25.4*(16*200)=125.98
        self.conversion= 126 #mm/step
        #main program
        print 'Searching for arduino ...'

        try:
            self.ser = serial.Serial('/dev/tty.usbmodem1431', 9600, timeout=0.1)
            print 'Arduino is found at port %s.'%self.ser
            #self.ser.open()
            self.status()
            print 'Retreving previous status information ... '
            print 'Waiting for commands:'
        except serial.serialutil.SerialException:
            print "The port is not available"

        return

    def exit(self):
        self.status()
        self.ser.close()
        return


    def status(self):
        print '\n\n==== Veto Status ===='
        print 'Position upper: ',self.pos_up/self.conversion,'mm' 
        print 'Position lower: ',self.pos_down/self.conversion,'mm'
        print '---- From Serial Port ----'
        print '<<< '+self.ser.readline()
        print '<<< '+self.ser.readline()
        print '<<< '+self.ser.readline()
        print '<<< '+self.ser.readline()
        return 

    def move(self, arm, direction, Nmm, override=0):
        # [arm, direction, distance in mm, override(option)]

        # move the arm in the direction for N mm

        # postive position means farther away from zero point

        # override to move to negative position, also position will be reset to
        # 0 after the movement
        
        # argument checks
        check1 = arm not in ['up','down'] 
        check2 = direction not in ['close', 'far'] 
        check3 = not isinstance(Nmm, float) or Nmm<0
        check4 = override not in [0,1]
        if check1 or check2 or check3 or check4:
            raise SyntaxError('Invalid parameters')

        pos=float(Nmm) #absolute position
        step=pos#int(float(Nmm)*self.conversion) # number of steps
        
        #safety check
        if not override:
            temp=0
            if direction=='close':
                if arm=='up':
                    temp=self.pos_up+pos
                else:
                    temp=self.pos_down+pos
            else:
                #correct the direction
                pos=-pos
            if temp>0:
                print 'Over the limit, abort.'
                return 

        #serial port output encoding
        #
        armbit=0
        if arm=='down':
            armbit=1

        output=''
        if direction=='close':
            output='+'+str(step)+' '+str(armbit)
        else:
            output='-'+str(step)+' '+str(armbit)
        self.ser.write(output+'\n')
        print output
        
        #position update
        if arm=='up':
            if override:
                self.pos_up=0
            else:
                self.pos_up+=pos
        if arm=='down':
            if override:
                self.pos_down=0
            else:
                self.pos_down+=pos

        self.status()
        return

    def return_pos(self, value):
        check1 = value not in ['up','down', 'all'] 
        #move arm(s) back to zero position
        if check1:
            raise SyntaxError('Invalid parameters')

        if value=='all':
            print 'Return both arm to zero position'
            move(1,'close', abs(self.pos_up))
            move(2,'close', abs(self.pos_down))
            return 
        if value=='up':
            print 'Return upper arm to zero position'
            move(value,'close', abs(self.pos_up))
            return
        if value=='down':
            print 'Return lower arm to zero position'
            move(value,'close', abs(self.pos_down))
            return 

    def reset(self, value):
        # set the current position as zero position
        check1 = value not in ['up','down', 'all'] 
        if check1:
            raise SyntaxError('Invalid parameters')
        if value=='all':
            self.pos_up=0
            self.pos_down=0
            print 'Reset both arm readings to zero'
            return 
        if value=='up':
            self.pos_up=0
            print 'Reset upper arm reading to zero'
            return
        if value=='down':
            self.pos_down=0
            print 'Reset lower arm reading to zero'
            return

    def help(self):
        print '''
        Veto control system v0.1 by Yimin Wang
        This provide the control for the positions of the veto arms.
        The functions available are:
            move(arm, direction, distance in mm) 
                arm type can be 'up','down','all'
                direction can be 'close', 'far'
            help():
                print this help
            exit():
                exit this program
            return(arm):
                arm type can be 'up','down','all'
                return both arms back to zero
            reset(arm):
                arm type can be 'up','down','all'
                set the current position of the selected arm as zero point
            status():
                Report the status of the Veto
        '''
        return

