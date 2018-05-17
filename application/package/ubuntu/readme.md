# What is this?
A simple ubuntu package for MultiMC that wraps the contains a script that downloads and installs real MultiMC on ubuntu based systems.

It contains a `.desktop` file, an icon, and a simple script that does the heavy lifting.

# How to build this?
You need dpkg utils. Rename the `multimc` folder to `multimc_1.2-1` and then run:
```
fakeroot dpkg-deb --build multimc_1.2-1
```

Replace the version with whatever is appropriate.
