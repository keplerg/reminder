# Reminder

This is a lightweight reminder program written in C++ and Qt. It functions similarly to kalarm.

The event windows are also blocking which means if an event window is open, no more will show. When the blocking window is shut, processing resumes and the blocked event windows will be displayed. 
This is a limitation of the Qt windowing library but could be gotten around by implementing a server along the lines of [https://doc.qt.io/qt-6/qtnetwork-fortuneserver-example.html].

You could also run multiple instances referencing separate reminder event files. For example:

```bash
nohup reminder tasks.txt &
nohup reminder meetings.txt &
```

## Usage

There are 3 ways to use this program:

```bash
nohup reminder &
```
Uses the default ~/.reminder file. This command should be put in your login script (e.g. ~/.bashrc or ~/.profile).

```bash
nohup reminder ~/reminder.txt &
```
Uses the passed file name instead of the default reminder file. Again this should be added to your login script.

```bash
reminder Lime - '<h1>Hello World</h1>'
```
Makes the reminder window appear on demand (no schedule). This could be useful called from a script.

This command can NOT be run from cron since you need to be connected to your terminal display.

The best way to run this executable is to add it to your startup programs.


## Build

To build the executable run the following commands from your Linux command line:

```bash
cd libcron
cmake .
make
cd ..
qmake
make
```

## Reminder file format

Each schedule line conststs of 8 parts, all mandatory. The first 5 parts (separated by whitespace) are similar to cron (see below for details on formatting). The next 2 parts are the background and text colors, the defaults are yellow black. All the remaining text is the message which can use HTML syntax.

```text
┌───────────── minute (0 - 59)
│ ┌───────────── hour (0 - 23)
│ │ ┌───────────── day of month (1 - 31)
│ │ │ ┌───────────── month (1 - 12)
│ │ │ │ ┌───────────── day of week (0 - 6) (Sunday to Saturday)
│ │ │ │ │ ┌───────────── background CSS color (e.g. red or #f00 or #ff0000)
│ │ │ │ │ │ ┌───────────── text CSS color
│ │ │ │ │ │ │ ┌───────────── reminder message (supports HTML)
│ │ │ │ │ │ │ │
* * * * * * * reminder message
```

### Example

A reminder to take out the trash every 2nd and 4th Sunday between October and December:

15 16 7-14,22-28 Oct-Dec Sun lime white <b>Take out the trash!</b>

## libcron was used for time/date scheduling
A C++ scheduling library using cron formatting. *Note: Included is a modified version which allows for simultaneous day of month and day of week. I also included a a single schedule check (no range)*

- Allowed formats:
  - Special characters: '*', meaning the entire range.
  - '?' or '*' can be used to ignore day of month/day of week as noted below.
  - Any single character (e.g. '-', '?', etc.) can be used to fallback to the default color.

  - Ranges: 1,2,4-6
    - Result: 1,2,4,5,6
  - Steps: n/m, where n is the start and m is the step.
    - `1/2` yields 1,3,5,7...<max>
    - `5/3` yields 5,8,11,14...<max>
    - `*/2` yields Result: 1,3,5,7...<max>
    - `Tue/2` yields Result: Tue,Thu,Sat (day of week field only)
    - `Mar/3` yields Result: Mar,Jun,Sep,Dec (month field only)
  - Reversed ranges:
    - `0 0 23-2 * * *`, meaning top of each minute and hour, of hours, 23, 0, 1 and 2, every day.
      * Compare to `0 0 2-23 * * *` which means top of each minute and hour, of hours, 2,3...21,22,23 every day.


For `month`, these (case insensitive) strings can be used instead of numbers: `JAN, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC`.
Example: `JAN,MAR,SEP-NOV`

For `day of week`, these (case insensitive) strings can be used instead of numbers: `SUN, MON, TUE, WED, THU, FRI, SAT`. 
Example: `MON-THU,SAT`

Each part is separated by one or more whitespaces. It is thus important to keep whitespaces out of the respective parts.

- Valid:
  - 0 15,18 * * * ? Red Green Watch Squid Games

- Invalid:
  - 0 15, 18 * * * ? Red Green Watch Squid Games
  

`Day of month` and `day of week` are NOT mutually exclusive. This is different from crontab (libcron was also modified).
You can now support schedules like "2nd Tuesday at 10AM for every month" => 0 0 10 8-14 * Tue - - Bridge Club

### Examples

| Expression         | Meaning                                   |
| ------------------ | ----------------------------------------- |
| * * * * *          | Every minute                              |
| 0 12 * * MON-FRI   | Every Weekday at noon                     |
| 0 12 1/2 * *	     | Every 2 days, starting on the 1st at noon |
| 0 */12 * * *       | Every twelve hours                        |
| @hourly            | Every hour                                |

### Convenience scheduling

These special time specification tokens which replace the 5 initial time and date fields, and are prefixed with the '@' character, are supported:

| Token     | Meaning                           |
| --------- | --------------------------------- |
| @yearly   | Run once a year, ie.  "0 0 1 1 *" |
| @annually | Run once a year, ie.  "0 0 1 1 *" |
| @monthly  | Run once a month, ie. "0 0 1 * *" |
| @weekly   | Run once a week, ie.  "0 0 * * 0" |
| @daily    | Run once a day, ie.   "0 0 * * *" |
| @hourly   | Run once an hour, ie. "0 * * * *" |
	
## Randomization

The standard cron format does not allow for randomization, but with the use of `CronRandomization` you can generate random
schedules using the following format: `R(range_start-range_end)`, where `range_start` and `range_end` follow the same rules
as for a regular cron range (step-syntax is not supported). All the rules for a regular cron expression still applies
when using randomization, i.e. mutual exclusiveness and no extra spaces.

### Examples
| Expression            | Meaning                                                   |
| --------------------- | --------------------------------------------------------- |
| 0 0 R(13-20) * * ?    | On the hour, on a random hour 13-20, inclusive.           |
| 0 0 0 ? * R(0-6)      | A random weekday, every week, at midnight.                |
| 0 R(45-15) */12 ? * * | A random minute between 45-15, inclusive, every 12 hours. |
| 0 0 0 ? R(DEC-MAR) *  | On the hour, on a random month december to march.         |


## Used Third party libraries

Qt6

Per Malmberg's [*modified* cron library](https://github.com/PerMalmberg/libcron)

Howard Hinnant's [date libraries](https://github.com/HowardHinnant/date/)
