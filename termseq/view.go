package main

import (
    "github.com/nsf/termbox-go"
)

// A view of a sequencer
type SequencerView struct {
    Sequencer *Sequencer
}

// Make a new sequencer view
func NewSequencerView(seq *Sequencer) *SequencerView {
    view := new(SequencerView)
    view.Sequencer = seq
    return view
}

// Draw sequencer to screen. Expects termbox.Init to have been called already.
func (view *SequencerView) Draw() {
    // Lock sequencer
    seq := view.Sequencer
    seq.SeqLock.Lock()
    defer seq.SeqLock.Unlock()

    // Get size
    //w, h := termbox.Size()

    // Clear
    termbox.Clear(termbox.ColorDefault, termbox.ColorDefault)

    // TODO Draw caption bar

    // TODO Draw global bar

    // TODO Draw each track
    //for _, track := range seq.Tracks {
    //}

    // Flush buffer to terminal
    termbox.Flush()
}
