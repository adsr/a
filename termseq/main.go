package main

import (
    "github.com/adsr/portmidi"
    "github.com/nsf/termbox-go"
    "time"
)

func main() {
    // Init portmidi
    if err := portmidi.Initialize(); err != nil {
        panic(err)
    }
    defer portmidi.Terminate()

    // Init termbox
    if err := termbox.Init(); err != nil {
        panic(err)
    }
    termbox.SetInputMode(termbox.InputEsc | termbox.InputMouse)
    termbox.HideCursor()
    defer termbox.Close()

    // Init sequencer and view
    seq := NewSequencer()
    seqView := NewSequencerView(seq)
    seq.Start()

    // Event poll/draw loop
    eventQueue := make(chan termbox.Event)
    go func() {
        for {
            // Stuff events in queue
            eventQueue <- termbox.PollEvent()
        }
    }()
    seqView.Draw()
    func() {
        keepGoing := true
        for {
            select {
            case event := <-eventQueue:
                // Process event
                keepGoing = processTerminalEvent(seq, event)
            case <-time.After(50 * time.Millisecond):
                // Timeout
            }
            // Draw
            seqView.Draw()
            if !keepGoing {
                return
            }
        }
    }()
}

// Process a terminal event
func processTerminalEvent(seq *Sequencer, ev termbox.Event) bool {
    return true
}
