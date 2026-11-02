# TimeFix

Fixes wrong date/time on old jailbroken iPhones (tested target: iPhone 5, iOS 8.4.1).
`timefixd` runs every 3 minutes via launchd, asks an NTP server for the time
(no GPS, no HTTPS), and corrects the clock if it is off by more than 5 seconds.

## Build (Theos)
    make package
The .deb appears in `packages/`. Copy it into `repo/debs/`.
If the postinst/prerm files are not executable: `chmod 755 layout/DEBIAN/*`.
Edit `control` (Maintainer/Author) and, if you like, rename the package id
`com.timefix.daemon` (also in the plist filename, plist Label, postinst, prerm).

## Check it works (SSH into the phone)
    date -s "2007-01-01 00:00:00"   # break the date on purpose
    /usr/bin/timefixd; date          # should be corrected
    # logs: syslog / Console, process name "timefixd"

## Host as a Cydia source on your own domain
1. Build the .deb and put it in `repo/debs/`.
2. `cd repo && ./build-repo.sh http://repo.yourdomain.com`
   This creates Packages, Packages.bz2, Packages.gz, index.html (Release is already here).
3. Upload everything in `repo/` (Release, Packages*, index.html, debs/) to the web root of
   repo.yourdomain.com. Any static host works (nginx, Apache, shared hosting, VPS).
4. Point DNS (A or CNAME record) of repo.yourdomain.com to that server.
5. On the phone: Cydia -> Sources -> Edit -> Add -> http://repo.yourdomain.com/
   Or open http://repo.yourdomain.com/ in Safari and tap "Add to Cydia".

### HTTP vs HTTPS
iOS 8 does not trust newer root CAs (e.g. Let's Encrypt chains fail), and a wrong
phone date also breaks HTTPS certificate checks. Plain http:// is the safest choice
for this repo. If you use https://, test it on the actual iPhone 5.

To update the package: bump `Version` in `control`, rebuild, replace the .deb in
`repo/debs/`, re-run `build-repo.sh`, re-upload.
