#   -*- mode: shell-script; -*-
#The definitions here are used by all window managers that !include menu.h.
#This way, you can set your prefferences (like whether to use xterm/rxvt,
#how long do you want your menutitles, etc) for all window-managers.
#
#(This all assumes you're using menu-1.4 or higher).
#This file is part of the menu package (version 1.4 or higher).
#For more information, see /usr/share/doc/menu/html

#If you prefer long titles, change the definition below accordingly.
#(this will currently not give you many long titles, as most menuentries
#still don't provide long titles. In those cases, the defintion below
#defaults to the short title).
function title()=$title
#function title()=ifelse($longtitle,$longtitle,$title)

#if you don''t like to see the icons, (un)comment (out) the lines below:
function icon()=ifelse($icon32x32, $icon32x32, \
                  ifelse($icon16x16, $icon16x16, $icon))
#function icon()= ""

#if you prefer an rxvt as your default terminal-program, comment out
#the next lines, and uncomment the definition below
#function term()=\
#    "xterm -sb -sl 500 -j -ls -fn 7x14 -geometry 80x30"\
#    " -T \"" title() "\""  " -e " $command
function term()=\
    "x-terminal-emulator " ifnempty($visible,"-ut") \
        ifnempty($geometry,"-geometry ") $geometry \
        " -T \"" title() "\""  " -e " $command

#function term()=\
#    "rxvt " ifnempty($visible,"-ut") \
#        ifnempty($geometry,"-geometry ") $geometry \
#        " -T \"" title() "\"" ifnempty(icon()," -n " icon()) " -e " $command

# if you want your submenus to come before the commands themselves
# in the menus (in case of mixed menus):
#
# sort=ifelse($command, "1", "0" ) ":$title"



#the following is for the hints (or optimised tree structure):
# (For more info on these variables, see /usr/share/doc/menu/*)

#if you want menu to optimize the tree, set this to true:
hint_optimize=false
#for more info on the other variables, see /usr/share/doc/menu/
#hint_nentry=6
#hint_topnentry=4
#hint_mixedpenalty=15
#
#The variables below are only useful if you want to speedup
#the finding of the best tree.
#hint_minhintfreq=0.1
#hint_mlpenalty=2000
#hint_max_ntry=4
#hint_max_iter_hint=5
#hint_debug=false


forcetree
#Due to the existance of both /Apps/System and /System,
#menu gets confused. So, force /System in it's own section
  System
endforcetree
