# What is this?
A simple Ubuntu package for MultiMC that contains a script that downloads and installs real MultiMC on Ubuntu based systems.

It contains a `.desktop` file, an icon, and a simple script that does the heavy lifting.

This is also the source for the files in the [RPM package](../rpm). If you rename, create or delete files here, you'll likely also have to update the RPM spec file there.

# How to build this?
You need dpkg utils. Rename the `multimc` folder to `multimc_1.3-1` and then run:
```
fakeroot dpkg-deb --build multimc_1.3-1
```

Replace the version with whatever is appropriate.
