#! /usr/bin/python2
#
# Author: Gaute Hope <eg@gaute.vetsj.com> / 2011-09-26
#
# Central logging and managing point
#
# Requires:
#  - pyserial

import serial
import signal
import time
from   threading import Thread
import multiprocessing, logging, logging.config
import sys

from ad       import *
from protocol import *
from buoy     import *
from ui       import *

from buoys    import buoys

class Zero:
  port = '/dev/ttyUSB0'
  baud = 115200
  ser  = None

  uimanager       = None
  uimanagerserver = None
  uiservicethread = None

  logger = None

  protocol = None # Serial protocol to ZeroNode

  buoys    = []   # List of Buoys
  currenti = 0    # Current buoy index

  # Reading thread
  go       = True

  # Current buoy thread
  cthread = None
  acquire = True # Do continuous acquisition of buoy data

  def get_current (self):
    try:
      return self.buoys[self.currenti]
    except ValueError as e:
      return None

  def set_current (self, b):
    if self.current:
      self.current.active  = False

    self.currenti = self.buoys.index(b)

    self.protocol.ensure_zn_address ()

    self.current.active = True
    self.current.index.cleanup = False
    self.current.index.idle    = False

    self.logger.info ("[Zero] Setting current Buoy to: " + b.name)

  current  = property(get_current, set_current) # Current Buoy

  def __init__ (self):
    multiprocessing.process.current_process ().name = 'Main'

    logging.config.fileConfig ('zero.logging.conf')
    self.logger = logging.getLogger ('root')

    # Protocol handler; receives data from ZeroNode
    self.protocol = Protocol (self)

    self.logger.info ("==================================================")
    self.logger.info ("[Zero] Starting Zero..")
    self.logger.info ("[Zero] Logging to console and to file log/zero.log.")

    for b in buoys:
      if b['enabled']:
        self.buoys.append (Buoy(self, b))

    # set first buoy active
    self.set_current (self.buoys[0])

    # Start UI manager
    self.uimanagerthread = Thread (target = self.run_ui_service, name = 'ZeroUIService')
    self.uimanagerthread.daemon = True
    self.uimanagerthread.start ()

    # Start thread for current buoy
    self.cthread = Thread (target = self.current_thread, name = 'CurrentBuoy')
    self.cthread.start ()

    # Start thread reading stdin
    #t = Thread (target = self.stdin, name = 'StdinHandler')
    #t.daemon = True
    #t.start ()

    self.main ()

  def run_ui_service (self):
    self.uimanager = ZeroUIManager ()
    self.uimanager.setup_server (self)
    self.uimanagerserver = self.uimanager.get_server ()
    self.uimanagerserver.serve_forever ()


  def openserial (self):
    if self.go:
      self.logger.info ("[Zero] Opening serial port " + str(self.port) + "..")

    msg = False
    while ((self.ser == None or not self.ser.isOpen ()) and self.go):
      self.protocol.zerooutput = -1 # reset zeronode outputmode to undef.
      self.protocol.adressedbuoy = 0 # reset adressed buoy
      try:
        try:
          self.ser = serial.Serial (port = self.port, baudrate = self.baud) #, timeout = 0)
          self.logger.info ("[Zero] Serial port open.")
        except serial.SerialException as e:
          if not msg:
            self.logger.error ("[Zero] Failed to open serial port.. retrying every 5 seconds.")
            msg = True
          else:
            self.logger.debug ("[Zero] Failed to open serial port.. retrying in 5 seconds.")

          self.ser = None
          time.sleep (5)

      except:
        self.stop ()

  def closeserial (self):
    try:
      if self.ser != None:
        self.ser.close ()
    except Exception as e:
      self.logger.exception ("[Zero] Could not close serial: " + str(e))

    self.ser = None

  def get_buoys (self):
    for i in self.buoys:
      yield i

  def send (self, msg):
    try:
      if self.ser is not None:
        self.logger.debug ("[Zero] Sending: " + msg)
        self.ser.write (msg + "\n")
    except serial.SerialException as e:
      self.logger.exception ("[Zero] Exception with serial link, reconnecting..: " + str(e))
      self.closeserial ()
      self.openserial ()


  def main (self):
    try:
      self.logger.info ("[Zero] Entering main loop..")
      self.openserial ()

      while self.go:
        try:
          if not self.ser == None:
            r = self.ser.read (1) # non-blocking
            if self.current is not None:
              self.protocol.handle (r)


        except serial.SerialException as e:
          self.logger.exception ("[Zero] Exception with serial link, reconnecting..: " + str(e))
          self.closeserial ()
          self.openserial ()

        except KeyboardInterrupt as e:
          self.logger.info ("[Zero] Shutting down by local request.")
          self.stop ()
        except:
          self.logger.exception ("[Zero] General exception in inner main loop")
          self.stop ()

    except KeyboardInterrupt as e:
      self.logger.info ("[Zero] Shutting down by local request.")
      self.stop ()

    except:
      self.logger.exception ("[Zero] General exception in main loop")
      self.stop ()

    finally:
      self.logger.info ("[Zero] Main loop finished..")

  def current_thread (self):
    self.logger.info ("[Zero] Starting current buoy thread..")
    MAX_BUOY_TIME = 40 # max time (seconds) before changing buoys
    lastchange    = time.time ()

    # if all is good, sync time within limits alternate through all buoys
    STATUS_INTERVAL = 10 # seconds before sorting on seconds

    while self.go:
      if self.current is not None:
        self.current.loop ()

        if self.ser is not None and self.acquire:
          # when current is done, go to next
          if (time.time () - lastchange > MAX_BUOY_TIME) and not self.current.index.cleanup and not self.current.index.idle:
            # give current buoy time to cleanup (timeout or receive)
            self.logger.info ("[Zero] Requesting current buoy to cleanup..")
            self.current.index.cleanup = True


          if self.current.index.idle:
            # iterate through buoys, skipping those with recent updates. if
            # all are uptodate, iterate.

            #bn = range (0, len(self.buoys))
            #bn.remove (self.currenti)
            #bn.sort (key = lambda bn: self.buoys[bn].index.sync_status_t)

            # priority:
            # 1. out of sync
            # 2. next incomplete
            # 3. next in line

            # pick next item with too long sync time, or just next if all are good
            i = -1
            ii = (self.currenti + 1) % len (self.buoys)
            while ii != self.currenti:
              if (time.time () - self.buoys[ii].index.sync_status_t > STATUS_INTERVAL):
                i = ii
                break
              ii = (ii + 1) % len(self.buoys)

            # pick next buoy with incomplete data
            if i == -1:
              ii = (self.currenti + 1) % len (self.buoys)
              while ii != self.currenti:
                if not self.buoys[ii].complete:
                  i = ii
                  break
                ii = (ii + 1) % len(self.buoys)

            # iterate: all good, pick next buoy
            if i == -1:
              i = (self.currenti + 1) % len(self.buoys)

            self.logger.info ("[Zero] Changing to buoy: " + self.buoys[i].name + " [" + str(self.buoys[i].id) + "]..")

            self.set_current (self.buoys[i])

            lastchange = time.time ()

      time.sleep (0.001)

  # go through list of buoys and return index of id
  def indexofid (self, id):
    c = 0
    for b in self.buoys:
      if b.id == id:
        return c
      c = c + 1

    return None

  def startacquire (self):
    self.logger.info ("[Zero] Starting continuous acquistion from buoys..")
    self.acquire = True

  def stopacquire (self):
    self.logger.info ("[Zero] Stopping continuous acquistion from buoys..")
    self.acquire = False

  def stdin (self):
    # Wait for input on stdin, then exit..

    raw_input ()
    self.stop ()

  def stop_manual (self):
    self.stop (True)

  def stop (self, manual = False):
    if manual:
      self.logger.info ("[Zero] Stopping Zero on request..")
    else:
      self.logger.info ("[Zero] Stopping Zero..")

    self.go = False

    # Stop all Buoys..
    self.cthread.join ()

    if self.current:
      self.current.deactivate ()
      self.current = None

    for i in self.buoys:
      i.stop ()

    self.closeserial ()
    self.logger.info ("[Zero] Stopped.")

if __name__ == '__main__':
  z = Zero ()



