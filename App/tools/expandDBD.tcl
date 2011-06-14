#!/usr/bin/tclsh

package require Tclx

set global_context [scancontext create]

set epicsversion 3.13
set quiet 0
set recordtypes 0
set seachpath {}

while {[llength $argv]} {
    switch -glob -- [lindex $argv 0] {
        "-3.14" { set epicsversion 3.14 }
        "-q"    { set quiet 1 }
        "-r"    { set recordtypes 1; set quiet 1 }
        "-I"    { lappend seachpath [lindex $argv 1]; set argv [lreplace $argv 0 1]; continue }
        "-I*"   { lappend seachpath [string range [lindex $argv 0] 2 end] }
        "-*"    { puts stderr "Unknown option [lindex $argv 0] ignored" }
        default { break }
    }
    set argv [lreplace $argv 0 0]
}

proc opendbd {name} {
    global seachpath
    foreach dir $seachpath {
        if ![catch {
            set file [open [file join $dir $name]]
        }] {
            return $file
        }
    }
    return -code error "file $name not found"
}

scanmatch $global_context {^[ \t]*(#|%|$)} {
    continue
} 

if {$recordtypes} {
    scanmatch $global_context {include[ \t]+"?(.*)Record.dbd"?} {
    puts $matchInfo(submatch0)
    continue
}

} else {

    scanmatch $global_context {(registrar|variable|function)[ \t]*\(} {
        global epicsversion
        if {$epicsversion == 3.14} {puts $matchInfo(line)}
    }

    scanmatch $global_context {
        puts $matchInfo(line)
    }
}

scanmatch $global_context {include[ \t]+"?([^"]*)"?} {
    global seachpath
    global FileName
    global quiet
    if [catch {
        includeFile $global_context $matchInfo(submatch0)
    } msg] {
        if {!$quiet} {
            puts stderr "ERROR: $msg in path \"$seachpath\" called from $FileName($matchInfo(handle)) line $matchInfo(linenum)"
            exit 1
        }
    }
    continue
}


proc includeFile {context name} {
    global global_context FileName
    set file [opendbd $name]
    set FileName($file) $name
    scanfile $context $file
    close $file
}   

foreach filename $argv {
    set file [open $filename]
    set FileName($file) $filename
    scanfile $global_context $file
    close $file
}

# $Header: /cvs/G/DRV/misc/App/tools/Attic/expandDBD.tcl,v 1.3 2011/06/14 16:00:55 zimoch Exp $
