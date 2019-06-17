package main

import (
    "container/list"
    "errors"
    "github.com/adsr/portmidi"
    "sync"
    "time"
)

// A sequencer of tracks
type Sequencer struct {
    Name            string
    Tracks          []*Track
    EventQueue      *list.List
    MidiEventFilter *MidiEventFilter
    BeatDur         time.Duration
    Paused          bool
    PlayChan        chan bool
    PauseChan       chan bool
    MidiOutputs     map[string]*portmidi.Stream
    CurBeat         uint64
    SeqLock         sync.Mutex
}

// Make a new sequencer
func NewSequencer() *Sequencer {
    seq := new(Sequencer)
    seq.Name = "Untitled"
    seq.Tracks = make([]*Track, 0, 8)
    seq.EventQueue = list.New()
    seq.MidiEventFilter = NewMidiEventFilter()
    seq.BeatDur = time.Duration(250) * time.Millisecond
    seq.Paused = true
    seq.PlayChan = make(chan bool, 1)
    seq.PauseChan = make(chan bool, 1)
    seq.MidiOutputs = make(map[string]*portmidi.Stream)
    return seq
}

// Start sequencer thread
func (seq *Sequencer) Start() {
    go func() {
        for {
            seq.CurBeat = 0

            // Wait for Play
            if seq.Paused {
                <-seq.PlayChan
                seq.Paused = false
            }

            // Play
            for {
                startTime := time.Now()

                func() {
                    seq.SeqLock.Lock()
                    defer seq.SeqLock.Unlock()

                    // Send beat
                    for _, track := range seq.Tracks {
                        track.BeatChan <- seq.CurBeat
                    }
                    seq.CurBeat += 1

                    // Queue events
                    for _, track := range seq.Tracks {
                        keepGoing := true
                        for {
                            select {
                            default:
                                keepGoing = false
                            case event := <-track.EventChan:
                                seq.queueEvent(track, event)
                            }
                            if !keepGoing {
                                break
                            }
                        }
                    }
                }()

                // Process events
                seq.processEvents()

                // Check for pause
                select {
                default:
                case <-seq.PauseChan:
                    seq.Paused = true
                }
                if seq.Paused {
                    break
                }

                // Sleep a bit, minus loop time
                time.Sleep(seq.BeatDur - time.Now().Sub(startTime))
            }
        }
    }()
}

// Play
func (seq *Sequencer) Play() {
    if seq.Paused {
        seq.PlayChan <- true
    }
}

// Pause
func (seq *Sequencer) Pause() {
    if !seq.Paused {
        seq.PauseChan <- true
    }
}

// Add track
func (seq *Sequencer) AddTrack(defaultEvent Event, beatsPerBar int, beatDivisor int, numBars int) (*Track, error) {
    seq.SeqLock.Lock()
    defer seq.SeqLock.Unlock()
    track := NewTrack(seq, defaultEvent, beatsPerBar, beatDivisor, numBars)
    seq.Tracks = append(seq.Tracks, track)
    err := seq.attemptOpenMidiOutput(defaultEvent.DeviceName)
    return track, err
}

// Remove track
func (seq *Sequencer) RemoveTrack(trackNum int) error {
    seq.SeqLock.Lock()
    defer seq.SeqLock.Unlock()
    if trackNum < 0 || trackNum >= len(seq.Tracks) {
        return errors.New("Invalid trackNum")
    }
    seq.Tracks = append(seq.Tracks[:trackNum], seq.Tracks[trackNum+1:]...)
    return nil
}

// Queue event
func (seq *Sequencer) queueEvent(track *Track, event *Event) {
    switch event.EventType {
    case EVENT_TYPE_TONE:
        event.MakeMidi()
        endEvent := event.Clone()
        endEvent.MakeNoteOff(track)
        seq.EventQueue.PushBack(endEvent)
    case EVENT_TYPE_PERC:
        event.MakeMidi()
    case EVENT_TYPE_CTL:
        event.MakeMidi()
    }
    seq.EventQueue.PushBack(event)
}

// Process queued events
func (seq *Sequencer) processEvents() {
    for e := seq.EventQueue.Front(); e != nil; e = e.Next() {
        event := e.Value.(*Event)
        if event.PlayAtBeat < seq.CurBeat {
            continue
        }
        seq.EventQueue.Remove(e)
        if len(event.MidiMsgs) > 0 || len(event.midiSysexStr) > 0 {
            if stream, ok := seq.MidiOutputs[event.DeviceName]; !ok {
                // TODO log error
                continue
            } else {
                if len(event.MidiMsgs) > 0 {
                    for _, m := range event.MidiMsgs {
                        // TODO log error
                        stream.WriteShort(int64(m.Status), int64(m.Data1), int64(m.Data2))
                    }
                }
                if len(event.midiSysexStr) > 0 {
                    // TODO log error
                    stream.WriteSysEx(portmidi.Timestamp(0), event.midiSysexStr)
                }
            }
        }
        if len(event.Code) > 0 {
            // TODO exec code
        }
    }
}

// Attempt to open midi output
func (seq *Sequencer) attemptOpenMidiOutput(deviceName string) error {
    numDevices := portmidi.CountDevices()
    for i := 0; i < numDevices; i++ {
        deviceId := portmidi.DeviceId(i)
        deviceInfo := portmidi.GetDeviceInfo(deviceId)
        if !deviceInfo.IsOutputAvailable || deviceInfo.Name != deviceName {
            continue
        }
        if !deviceInfo.IsOpened {
            if stream, err := portmidi.NewOutputStream(deviceId, 1024, 0); err != nil {
                return err
            } else {
                seq.MidiOutputs[deviceName] = stream
            }
        }
    }
    return nil
}
