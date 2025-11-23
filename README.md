# RssMaker

This is a small program intended for use on servers to create and manage simple RSS feeds that others may read from. I created it for use on [my personal server](https://alex-selter.com/). It is most entirely complete, I only need to finish one top-level menu option (that is not strictly required anyway).

## Compilation

This program depends on `ncurses` and `tinyxml2`. You should install those before compilation. For Debian-based systems, this command should do that:

```bash
sudo apt -y install ncurses libtinyxml2-dev
```

## Usage

All operations are done through CLI. The menus should be relatively self-explanatory, so I won't bother re-explaining them.

When run without a command-line option, you will be given the choice of creating a new feed or exiting (the option to load a feed is not yet implemented).

To load a pre-existing feed, pass it as the first command-line option:

```bash
./rss-maker feed.xml
```
