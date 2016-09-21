
require 'gtk2'

#setup some helper functions to block so the script doesn't exit before we've had a chance to see the behavior
if !defined?(wait_until)
    def wait_until(announce=nil)
        priosave = Thread.current.priority
        Thread.current.priority = 0
        unless announce.nil? or yield
            puts(announce)
        end
        until yield
            sleep 0.25
        end
        Thread.current.priority = priosave
    end

    def wait_while(announce=nil)
        priosave = Thread.current.priority
        Thread.current.priority = 0
        unless announce.nil? or !yield
            puts(announce)
        end
        while yield
            sleep 0.25
        end
        Thread.current.priority = priosave
    end

end

if defined?(Gtk) && !defined?(Gtk.queue)
    module Gtk
        # Calling Gtk API in a thread other than the main thread may cause random segfaults
        def Gtk.queue &block
            GLib::Timeout.add(1) {
                begin
                    block.call
                rescue
                    puts "error in Gtk.queue: #{$!}"
                rescue SyntaxError
                    puts "error in Gtk.queue: #{$!}"
                rescue SystemExit
                    nil
                rescue SecurityError
                    puts "error in Gtk.queue: #{$!}"
                rescue ThreadError
                    puts "error in Gtk.queue: #{$!}"
                rescue SystemStackError
                    puts "error in Gtk.queue: #{$!}"
                rescue Exception
                    puts "error in Gtk.queue: #{$!}"
                rescue ScriptError
                    puts "error in Gtk.queue: #{$!}"
                rescue LoadError
                    puts "error in Gtk.queue: #{$!}"
                rescue NoMemoryError
                    puts "error in Gtk.queue: #{$!}"
                rescue
                    puts "error in Gtk.queue: #{$!}"
                end
                false # don't repeat timeout
            }
        end
    end
end


window_width       = 400
window_height      = 800
window_position    = [ 0, 0 ]

narost_exit     = false
window_resized  = true
start           = nil

window          = nil

require 'pp'

Gtk.queue {
    window = Gtk::Window.new
    window.title = "Test window"
    window.signal_connect('delete_event') { narost_exit = true}

    window.show_all

    window_width = [window_width,100].max
    window_height = [window_height,100].max
    window.resize(window_width, window_height)

    window_position[0] = [0, (Gdk.screen_width - window_width)].min
    window_position[1] = [0, (Gdk.screen_height - window_height)].min

    window.move(window_position[0], window_position[1])

    start = true
}

thread = Thread.new {
    # Create a second thread as our window event processor
    Gtk.main
}

# Wait for the window to open
wait_until { start }

# Wait until the window is closed
wait_while { !narost_exit }
