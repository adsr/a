package main

import (
    "errors"
    "sync"
)

type Track struct {
    Name           string
    Sequencer      *Sequencer
    DefaultEvent   Event
    TrackLines     []*TrackLine
    BeatsPerBar    int
    NumBars        int
    BeatDivisor    uint64
    BeatChan       chan uint64
    EventChan      chan *Event
    CurDividedBeat uint64
    TrackLock      sync.Mutex
}

type TrackLine struct {
    Track        *Track
    Name         string
    DefaultParam int
    Events       []*Event
}

func NewTrack(parent *Sequencer, defaultEvent Event, beatsPerBar int, beatDivisor int, numBars int) *Track {
    track := new(Track)
    track.Name = "Track"
    track.Sequencer = parent
    track.DefaultEvent = defaultEvent
    track.TrackLines = make([]*TrackLine, 0, 1)
    track.BeatsPerBar = beatsPerBar
    track.NumBars = numBars
    track.resizeEvents()
    track.BeatDivisor = uint64(beatDivisor)
    track.BeatChan = make(chan uint64, 1)
    track.EventChan = make(chan *Event, 64)
    return track
}

func (track *Track) Start() {
    go func() {
        for {
            func() {
                // Wait for beat
                curBeat := <-track.BeatChan

                // Skip if dividedBeatCount does not align with BeatDivisor
                if curBeat%track.BeatDivisor != 0 {
                    return
                }

                // Acquire lock
                track.TrackLock.Lock()
                defer track.TrackLock.Unlock()

                // Send events
                for _, trackLine := range track.TrackLines {
                    event := trackLine.Events[curBeat%uint64(len(trackLine.Events))]
                    if event != nil {
                        track.EventChan <- event
                    }
                }

                // Increment beat
                track.CurDividedBeat += 1
            }()
        }
    }()
}

func (track *Track) SetBeatsPerBar(beatsPerBar int) {
    track.TrackLock.Lock()
    defer track.TrackLock.Unlock()
    track.BeatsPerBar = beatsPerBar
    track.resizeEvents()
}

func (track *Track) SetNumBars(numBars int) {
    track.TrackLock.Lock()
    defer track.TrackLock.Unlock()
    track.NumBars = numBars
    track.resizeEvents()
}

func (track *Track) SetBeatDivisor(beatDivisor uint64) error {
    track.TrackLock.Lock()
    defer track.TrackLock.Unlock()
    if beatDivisor < 1 {
        return errors.New("beatDivisor must be >= 1")
    }
    track.BeatDivisor = beatDivisor
    return nil
}

func (track *Track) AddTrackLine(name string, defaultParam int) *TrackLine {
    track.TrackLock.Lock()
    defer track.TrackLock.Unlock()
    trackLine := new(TrackLine)
    trackLine.Track = track
    trackLine.Name = name
    trackLine.DefaultParam = defaultParam
    trackLine.resizeEvents()
    return trackLine
}

func (track *Track) RemoveTrackLine(trackLineNum int) error {
    track.TrackLock.Lock()
    defer track.TrackLock.Unlock()
    if trackLineNum < 0 || trackLineNum >= len(track.TrackLines) {
        return errors.New("Invalid trackLineNum")
    }
    track.TrackLines = append(track.TrackLines[:trackLineNum], track.TrackLines[trackLineNum+1:]...)
    return nil
}

func (trackLine *TrackLine) SetEventAt(beatNum int, event *Event) error {
    trackLine.Track.TrackLock.Lock()
    defer trackLine.Track.TrackLock.Unlock()
    if beatNum < 0 || beatNum >= len(trackLine.Events) {
        return errors.New("Invalid beatNum")
    }
    trackLine.Events[beatNum] = event
    return nil
}

func (track *Track) resizeEvents() {
    for _, trackLine := range track.TrackLines {
        trackLine.resizeEvents()
    }
}

func (trackLine *TrackLine) resizeEvents() {
    numEvents := trackLine.Track.BeatsPerBar * trackLine.Track.NumBars
    newEvents := make([]*Event, 0, numEvents)
    copy(newEvents, trackLine.Events)
    trackLine.Events = newEvents
}
