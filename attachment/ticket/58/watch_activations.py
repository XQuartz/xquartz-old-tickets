#!/usr/bin/python
import signal
import sys
def signal_handler(signal, frame):
    sys.exit(0)
signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGQUIT, signal_handler)

from AppKit import *

class MyObserver(NSObject):
    def appActivated_(self, notification):
        app = notification.userInfo()["NSWorkspaceApplicationKey"]
        NSLog(u"activated %@ (%@)", app.localizedName(), app.bundleIdentifier())

ws = NSWorkspace.sharedWorkspace()
nc = ws.notificationCenter()
ob = MyObserver.new()
nc.addObserver_selector_name_object_(ob, "appActivated:", NSWorkspaceDidActivateApplicationNotification, ws)
NSRunLoop.currentRunLoop().run()
