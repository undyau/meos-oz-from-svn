10 December 2014 3.1.383.5
- Allow user to save details of rental sticks so that rental
flag is set automatically.
- Prevent crash if trying to print rogaining result splits for 
someone who hasn't run.

24 October 2014 3.1.383.4
- Improve quality of SSS upload so that uploaded data can be used
  for RouteGadget.

16 July 2014 3.1.383.3
- Add support for printing labels as competitor finishes or
  afterwards. Configure via splits printing options.

8 May 2014 3.1.383.2
- Australianise the competition report and add reporting on
  competitors/course for simple (1 course per class) events.

5 May 2014 3.1.383.1
- Merge with standard MEOS 3.1.383.1  (the 3.1 release)
  For changes see http://www.melin.nu/meos/en/whatsnew25.php
  
22 March 2014 3.1.361.1
- Merge with standard MEOS 3.1.361.
  For changes see http://www.melin.nu/meos/en/whatsnew25.php

22 February 2014 3.1.316.2 
- Display Rogaining (score) points when download occurs, along 
  with existing result information.

9 February 2014 3.1.316.1 
- Merge with new release of MEOS (3.1.316 - 3.1 Beta)

8 December 2013 (fork of 3.0.311)
- When importing IOF 3 XML entries attempt match on existing entries by name first.
- Club names were not being included in import from XML.

20 October 2013 (fork of 3.0.311)
- Improve formatting of SSS style results so that position numbers aren't truncated.
- Import from XML tries to match existing runner by name if match by external Id fails.
- Treat competitors who have explicit course set the same as those who have course set from class.
- SSS quick-start now uses current date as event date.
- Import from XML now sets class based on imported name if XML doesn't contain CourseId.


14 October 2013 (fork of 3.0.311)
- Implement upload to existing SSS processor.
- Some additional language conversion for multi-stage events.


13 August 2013 (fork of 3.0.311)
- Alter "Remaining Runners" to show who may be missing 
  when running with people being auto-entered as they
  download. Also show when stick used by an existing
  runner was later re-used and hasn't downloaded.

13 July 2013 (fork of 3.0.311)
- Splits shown incorrectly when more than one control with
  a minimum-time (i.e. two road crossings).

26 June 2013 (fork of 3.0.311)
- add missing HTML help for English

19 June 2013 (fork of 3.0.311)
- SSS results report
- Start & finish time converted from UTC in results export
- Replace remaining reference to Swedish Eventor

20 May 2013 (fork of 3.0.311)
- fix export to XML by course
- English header line for OE csv export

May 2013 (fork of 3.0.311)
- New simple drawing mechanism, accessed via Classes|Draw Several Classes
  MEOS-Random Fill - then Draw Automatically.
  This will do a random draw over the range of start times, only using
  times where nobody else is running on same course.

April 2013 (fork of 3.0.311)

Australian Customisations:
- New list, results by course, hard-coded
- Enhance line course splits printout
- Import of Or format start lists
- Automatic set-up of Sydney Summer Series event
- Default to Australian Eventor
- Include course in by-minute start list
- Some changes to start draw to compress starts
- Format score (rogaine) splits in a more meaningful way
- Supply Australian competitor/club database not Swedish one
- Shorten long club names provided by Eventor to XX.Y format


Bug Fixes (to be sent to MEOS owners):
- oRunner::getLegPlace() doesn't work.

Bug Fixes (sent to MEOS owner):
- oRunner::doAdjustTimes() - splits shown incorrectly 
  when more than one control with a minimum-time.
- New time-zone adjustment logic failed if the supplied
  timestamp was in hh:mm:ss format.