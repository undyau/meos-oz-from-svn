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
- oRunner::doAdjustTimes() - splits shown incorrectly 
  when more than one control with a minimum-time.

Bug Fixes (sent to MEOS owners):
- Convert event start time from Eventor from UTC to local
- Convert XML 3.0 start times to UTC for export to Eventor
- TabCompetition disabled a button that didn't exist on startlist export