#! /usr/bin/python2
#
# Author: Gaute Hope <eg@gaute.vetsj.com> /  2013-09-21
#
# List localized or to-be localized events with status:
# * plot events with finished jobs
# * gather position and rms values
#
# options:
# -p  re-plot
# -l  re-do locations

import os
import os.path
import sys
import shutil

replot = False
if '-p' in sys.argv:
  replot = True
  sys.argv.remove ('-p')

locate = False
if '-l' in sys.argv:
  locate = True
  sys.argv.remove ('-l')

# stations
stations = ['GAK2', 'GAK3', 'GAK4', 'GAKS']
sdirs = { 'GAK2' : 'b2', 'GAK3' : 'b3', 'GAK4': 'b4', 'GAKS' : 's5' }

# dirs
contdir = '01_cont'
rawevents = '02_events_raw'
readyevents = '03_events_ready'
localizeevents = '04_events_localize'

# figure out which dir im run in: either db root or one of sub-dbs
root = './'
if not os.path.exists (os.path.join (root, contdir)):
  root = '../'
  if not os.path.exists (os.path.join (root, contdir)):
    root = '../../'
    if not os.path.exists (os.path.join (root, contdir)):
      print "Could not figure out root db dir, run from either root db or one of sub dirs."
      sys.exit (1)

contdir = os.path.join (root, contdir)
rawevents = os.path.join (root, rawevents)
readyevents = os.path.join (root, readyevents)
localizeevents = os.path.join (root, localizeevents)


# Search for events in 04
localize = []
files = os.listdir (localizeevents)
files.sort ()
for f in files:
  if '.S' in f:
    localize.append (f)

for event in localize:
  print "event: %s: " % (event,),
  edir = os.path.join (localizeevents, event)

  # do locations if specified
  if locate:
    # set up all jobs
    for n in range(1,5):
      j = "%02d" % n
      os.system ("cd " + edir + "; makehcjob.py -j " + j + " > /dev/null" )

      # solve
      os.system ("cd " + os.path.join(edir, 'job_hc' + j) + "; hyp " + event + " > /dev/null")


  # check if job has been created
  files = os.listdir (edir)
  jobs = []
  for f in files:
    if 'job_' in f:
      jobs.append (f)

  if len(jobs) == 0:
    print "no jobs."
    continue

  # jobs exist, re-run plotevent_ibcao
  if replot:
    os.system ("cd " + edir + "; plotevent_ibcao.py > /dev/null 2>&1")
  else:
    os.system ("cd " + edir + "; plotevent_ibcao.py -n > /dev/null 2>&1")

  # read explanation
  inf = os.path.join (edir, 'notes.txt')
  if os.path.exists (inf):
    inff = open (inf, 'r')
    explanation = inff.read ()
    inff.close ()
    print explanation.strip ()

  # read report
  repf = open(os.path.join (edir, 'report.txt'), 'r')
  rep  = repf.readlines ()
  repf.close ()
  first = True
  if len(rep) == 0:
    print "no jobs."
    continue

  for l in rep:
    if l.strip () == "":
      continue

    if first:
      print l.strip()
      first = False
    else:
      print "                             " + l.strip()




