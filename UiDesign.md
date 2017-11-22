-- UI Design --

--- On mobile & TV platforms ---

Target platforms: Android, iOS, UWP

Main screen

The main screen should present a view of games that are available on the user's system.

The screen must display games as a list of boxes.

- Scanning process -

There are two separate scanning processes. A full scan and a startup scan.

-- Full scan -- 

A full scan goes through all potential locations on the system to find any bootable that might be present.

-- Startup scan --

The startup scan only goes through directories where games have been found before. This scanning strategy
is used to improve the application's starting time while allowing new bootables to be discovered at places
where the user is most likely to place them.

.nomedia files

- Clicking on a box -

1. If the bootable is not present on the file system when it's clicked on,
a message asking the user if the file should be deleted from the database.

2. If the bootable is present on the system, its last played time is updated.
