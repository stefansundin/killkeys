# Introduction

Windows has a functionality known as a _scan code mapper_ in its keyboard driver. It enables a user to remap any key on the keyboard to another key, or simply disable it.

This change has effect globally in Windows, and is a great alternative if KillKeys doesn't work in a game you are playing. You do not need KillKeys to use any of this. One great benefit is that you can map keys such as F1-F12 to something more useful, such as back and forward in browsers, media keys, etc.

You can find some technical information on [this MSDN page](http://msdn.microsoft.com/en-us/library/windows/hardware/gg463447.aspx).


# How to use it

It can be quite difficult to configure this registry value manually, and there is a chance of breaking things, so instead I recommend that you use a separate program to do that.

There is a tool called [KeyTweak](http://www.softpedia.com/get/System/OS-Enhancements/KeyTweak.shtml) that works really well. Use it to create your mappings. Once that's done, you can uninstall the tool if you wish.

You have to reboot for changes to take effect.


# Quick and dirty commands

Make sure you are running it with administrator privileges, hold Ctrl+Shift if running from the start menu. In Windows 8, press Win+S, paste the command and then right-click the entry and click _Run as administrator_.

These commands do not work together. When you run one, you replace your previous mapping.

Run this to disable both windows keys and the menu key:
```
reg add "HKLM\SYSTEM\CurrentControlSet\Control\Keyboard Layout" /v "Scancode Map" /t REG_BINARY /f /d 00000000000000000400000000005be000005ce000005de000000000
```

To only disable the caps lock key, run this:
```
reg add "HKLM\SYSTEM\CurrentControlSet\Control\Keyboard Layout" /v "Scancode Map" /t REG_BINARY /f /d 00000000000000000200000000003a0000000000
```

To map the caps lock key to the Windows key and disable the left Windows key (Chromebook style):
```
reg add "HKLM\SYSTEM\CurrentControlSet\Control\Keyboard Layout" /v "Scancode Map" /t REG_BINARY /f /d 0000000000000000030000005be03a0000005be000000000
```

To clear all mappings, run this:
```
reg delete "HKLM\SYSTEM\CurrentControlSet\Control\Keyboard Layout" /v "Scancode Map" /f
```

Remember to reboot!


# Improve this page

Post [in the subreddit](http://www.reddit.com/r/stefansundin/) to suggest improvements to this page.
